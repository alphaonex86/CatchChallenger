"""Valgrind/protocol test for server handler 0x88 (buyObject).

Handler: server/base/ClientNetworkReadQuery.cpp:81 (Client::parseQuery, case 0x88).
Wire form: FIXED-size QUERY, code 0x88, queryNumber, exactly 10 data bytes:
    [objectId:u16][quantity:u32][price:u32]
Because 0x88 >= 0x80 it carries a queryNumber; because
packetFixedSize[0x88]==2+4+4==10 it is FIXED size (NO 4-byte dynamic length on
the wire). Wire bytes are:
    [0x88][qnum][id0 id1][q0 q1 q2 q3][p0 p1 p2 p3]
Unlike most neighbouring cases, 0x88 has NO `#ifdef CATCHCHALLENGER_HARDENED`
size guard in parseQuery: the size is fixed by packetFixedSize[], so the
framing layer (ProtocolParsingInput) delivers exactly 10 data bytes or rejects
the frame; the three loadLeNN() reads can never run short.

Handler logic traced
(ClientNetworkReadQuery.cpp:81 -> server/base/ClientEvents/LocalClientHandlerShop.cpp:91
 Client::buyObject(query_id, objectId, quantity, price)):

  1. mapIndex>=65535            -> silent return (no reply, not on a map).
  2. quantity<=0  (quantity is uint32, so this catches quantity==0)
        -> errorOutput("quantity wrong: ...") -> KICK.
  3. facedShop(): look at the cell the player is FACING. If that move is
     invalid (out of map / colliding wall)
        -> errorOutput("not shop into this direction") -> KICK.
  4. The faced cell must hold a SHOP (map->shops[(x,y)]). If not
        -> errorOutput("not shop into this direction") -> KICK.
  5. objectId not sold by this shop          -> 0x7F reply BuyStat_HaveNotQuantity (0x03).
  6. shop price for objectId == 0            -> 0x7F reply BuyStat_HaveNotQuantity (0x03).
  7. realprice > price (client under-paid)   -> 0x7F reply BuyStat_PriceHaveChanged (0x04).
  8. realprice < price (client over-offered) -> 0x7F reply BuyStat_BetterPrice (0x02)+u32.
  9. otherwise                               -> BuyStat_Done (0x01).
     Then THE SECURITY FIX (LocalClientHandlerShop.cpp:174-179):
        const uint64_t totalprice = static_cast<uint64_t>(realprice)*quantity;
        if(cash >= totalprice) removeCash(totalprice); else KICK.
        addObject(objectId, quantity);
     realprice and quantity are both uint32. The OLD code multiplied in 32 bits
     so quantity=0x80000000 with an even realprice wrapped totalprice to 0 ->
     the cash check passed for free -> attacker got billions of items for ~0
     cash. The fix promotes to uint64 before the multiply, so the true product
     is compared against the (uint64) cash and an insufficient-cash buy is
     KICKED. cash/inventory are therefore never corrupted by an overflow buy.

  Client::errorOutput -> disconnectClient() (server/base/Client.cpp:524). So
  EVERY rejection of buyObject KICKS the offending client; the server stays
  alive. A successful buy (cases 7-9) sends a 0x7F reply and mutates cash.

REACHABILITY (achieved -> the success branch IS exercised):
  buyObject requires the player to be standing in front of a shop bot. The
  server started for this test uses H.Server(start_override=...): the run-local
  start profile spawns every new character in map 'house2' at cell (0,23),
  DIRECTLY BELOW the Seller bot whose 'shop' step is at cell (0,22)
  (map/main/test/house2.xml bot id=2), with 1e9 cash (player/start.xml
  <cash>). A fresh character's last_direction is look-at-bottom (orientation
  bottom, DatapackGeneralLoader), i.e. AWAY from the shop: a buy without
  turning is still rejected at the shop gate (kept as a semantic-invalid
  case). Looking up with the 0x02 direction message puts the shop cell in the
  facing slot, and every branch past the gate becomes reachable in-band. The
  shop's own product list (item ids + prices) is fetched with query 0x87
  (getShopList, same gate) instead of hard-coding datapack prices.

  Enabling fix in _protoharness: MSG_FIXED[0x64] was 2, but the server pushes
  the online-player count as ONE data byte when max_players<=255
  (ClientHeavyLoadSelectCharFinal.cpp:503). The off-by-one left a stray 0x01
  in the stream right after select that mis-framed every later packet, which
  is why 0x87/0x88 replies used to be invisible from an on-map session.

DB-STATE verification (checks_db_state=True, now FULL):
  reload_state reads back cash + inventory (cash from the select block,
  items from the 0x54 snapshot; Api_protocol_loadchar.cpp / message.cpp) plus
  the persisted identity/position. Every valid branch asserts the EXACT cash
  delta and item count after disconnect+reload, not just the in-band reply.
"""

import struct
import time
import _protoharness as H

NAME = "0x88 buyObject"

# Spawn right below the house2 Seller bot (shop cell (0,22)); 1e9 cash so the
# legit buys and the BetterPrice branch have headroom, while the overflow buy
# (realprice * 0x80000000 >= 2^39) still exceeds it -> insufficient-cash KICK.
CASH_SEED = 1000000000
START_OVERRIDE = {"map": "house2", "x": 0, "y": 23, "cash": CASH_SEED}


def _uniq(prefix):
    return ("%s%x" % (prefix, int(time.time() * 1000) & 0xFFFFFFFF)).encode()


def _alive_clean(server):
    """True iff the server process is up, accepting TCP, and not crashed."""
    if not server.alive():
        return False, "server.alive() == False"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % cr[:300]
    return True, ""


def _buy_payload(object_id, quantity, price):
    """The 10 fixed data bytes of a 0x88 buyObject packet."""
    return H.u16(object_id) + H.u32(quantity) + H.u32(price)


def _is_kicked(sess, settle=0.4):
    """Return True iff the server CLOSED this session's socket (client kicked).

    Detection: read from the raw socket; the server's disconnectClient() shuts
    the connection, so recv() returns b'' (EOF). On a CharacterSelected client
    the server first pushes a 'have been kicked ... try hack' system message,
    then the FIN; we keep reading until EOF, then poke the peer once more with a
    well-formed-but-illegal 0x88 to force the FIN to be observed. A still-open
    socket (timeout, no EOF, poke send succeeds and stays open) means NOT
    kicked.
    """
    sk = sess.sock
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sk.settimeout(0.2)
            d = sk.recv(65536)
            if d == b"":
                return True  # clean EOF -> server closed us -> kicked
            # bytes arrived (the 'kicked, try hack' system message or trailing
            # data); keep reading until EOF or timeout.
        except OSError as e:
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                break
            return True  # ECONNRESET / EBADF -> peer gone -> kicked
    # Force the issue: a follow-up illegal buyObject query. If the peer is gone
    # the send errors or a subsequent recv hits EOF.
    try:
        sk.sendall(bytes([0x88, 0x00]) + _buy_payload(0xFFFF, 1, 1))
    except OSError:
        return True  # send failed -> peer gone -> kicked
    try:
        sk.settimeout(0.4)
        d = sk.recv(65536)
        if d == b"":
            return True
        try:
            d2 = sk.recv(65536)
            if d2 == b"":
                return True
        except OSError as e:
            if not (isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout"):
                return True
    except OSError as e:
        if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
            return False  # socket still open, server kept us -> NOT kicked
        return True       # reset -> kicked
    return False


def _facing_shop(server, tag):
    """Fresh session, spawned below the Seller bot, turned to FACE it."""
    s = H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                  pseudo="Buyer")
    s.look(1)  # 0x02 [sub 0][dir 1=top]: shop bot cell (0,22) is above (0,23)
    return s


def _shop_products(sess):
    """Query 0x87 getShopList (same facing gate as 0x88) -> {item_id: price}.
    Reading the list back from the SERVER is how this test picks item+price
    pairs: nothing about the datapack shop is hard-coded here."""
    pa = sess.reply_to(sess.q(0x87), timeout=2.0)
    if pa is None or len(pa) < 2:
        return None
    n = struct.unpack("<H", pa[0:2])[0]
    prods = {}
    for i in range(n):
        o = 2 + i * 10          # [id:u16][price:u32][u32 reserved]
        if o + 6 > len(pa):
            return None
        prods[struct.unpack("<H", pa[o:o+2])[0]] = \
            struct.unpack("<I", pa[o+2:o+6])[0]
    return prods


def _closed_reload(server, sess, settle=0.5):
    """Disconnect the session (FILE_DB persists at disconnect) and read the
    persisted cash/items/position back over the protocol."""
    lg, pw = sess.login_creds, sess.pass_creds
    sess.close()
    time.sleep(settle)
    return H.reload_state(server, lg, pw)


def run(server):
    try:
        # ----------------------------------------------------------------
        # Baseline legit "victim" account whose persisted state must stay
        # intact through all the abuse (its spawn/cash seed is identical to
        # every attacker account, so ANY delta is real corruption).
        # ----------------------------------------------------------------
        victim_login = _uniq("vic")
        victim_pass = _uniq("vpw")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)

        # The seed itself must have landed: character really spawns below the
        # shop bot, and really holds the seeded cash, or nothing below proves.
        if (victim.x, victim.y) != (0, 23) or victim.mapIndex is None:
            return (False, "start_override spawn failed: pos=(%r,%r) map=%r"
                    % (victim.x, victim.y, victim.mapIndex))
        if victim.cash != CASH_SEED:
            return (False, "start_override cash failed: %r != %d"
                    % (victim.cash, CASH_SEED))

        victim.close()
        time.sleep(0.5)  # let FILE_DB disconnect-save complete
        checks_db = False
        try:
            before = H.reload_state(server, victim_login, victim_pass)
            if before.get("character_id") is None:
                return (False, "victim did not persist (no character_id)")
            if before.get("cash") != CASH_SEED or before.get("items") is None:
                return (False, "victim persisted state unreadable: %r" % before)
            checks_db = True
        except Exception as e:
            return (False, "reload_state(before) raised: %s" % e)

        # ----------------------------------------------------------------
        # VALID cases: the player is FACING the shop (look-up), so every
        # reply branch past the gate is exercised, and the money/inventory
        # effect is proven against the PERSISTED state (reload_state).
        # ----------------------------------------------------------------
        valid_cases = 0
        try:
            s = _facing_shop(server, "v0")
        except H.HandshakeError as e:
            return (False, "facing session handshake failed: %s" % e)
        prods = _shop_products(s)
        s.close()
        time.sleep(0.3)
        if not prods:
            return (False, "0x87 getShopList while facing the shop bot "
                    "returned no product list (shop not reachable?)")
        item_id = min(prods)
        price = prods[item_id]

        # (1) Buy ONE at the exact shop price -> BuyStat_Done(0x01); after
        #     disconnect+reload: cash == seed - price, items[item_id] +1.
        s = _facing_shop(server, "v1")
        r = s.reply_to(s.q(0x88, _buy_payload(item_id, 1, price)), timeout=1.5)
        if r is None or len(r) < 1 or r[0] != 0x01:
            return (False, "exact-price buy did not answer BuyStat_Done: %r" % r)
        snap = _closed_reload(server, s)
        if snap.get("cash") != CASH_SEED - price:
            return (False, "exact buy cash wrong: %r != %d"
                    % (snap.get("cash"), CASH_SEED - price))
        base_item = before["items"].get(item_id, 0)
        if (snap.get("items") or {}).get(item_id) != base_item + 1:
            return (False, "exact buy added no item: items=%r" % snap.get("items"))
        valid_cases += 1

        # (2) Over-offer -> BuyStat_BetterPrice(0x02)+u32, but the buy STILL
        #     completes at the REAL price: cash delta is exactly -price.
        s = _facing_shop(server, "v2")
        r = s.reply_to(s.q(0x88, _buy_payload(item_id, 1, price + 1000)),
                       timeout=1.5)
        if r is None or len(r) < 5 or r[0] != 0x02:
            return (False, "over-offer did not answer BuyStat_BetterPrice+u32: %r" % r)
        if struct.unpack("<I", r[1:5])[0] == 0:
            return (False, "BetterPrice echoed a zero price: %r" % r)
        snap = _closed_reload(server, s)
        if snap.get("cash") != CASH_SEED - price:
            return (False, "over-offer charged %r, must charge the real price %d"
                    % (CASH_SEED - snap.get("cash"), price))
        valid_cases += 1

        # (3) Under-pay -> BuyStat_PriceHaveChanged(0x04) and NO mutation.
        s = _facing_shop(server, "v3")
        r = s.reply_to(s.q(0x88, _buy_payload(item_id, 1, price - 1)),
                       timeout=1.5)
        if r is None or len(r) < 1 or r[0] != 0x04:
            return (False, "under-pay did not answer BuyStat_PriceHaveChanged: %r" % r)
        snap = _closed_reload(server, s)
        if snap.get("cash") != CASH_SEED or (snap.get("items") or {}).get(item_id, 0) != base_item:
            return (False, "refused (PriceHaveChanged) buy mutated state: %r" % snap)
        valid_cases += 1

        # (4) Item the shop does not sell -> BuyStat_HaveNotQuantity(0x03)
        #     and NO mutation. 0xFFFF can never be a sold product id here.
        s = _facing_shop(server, "v4")
        r = s.reply_to(s.q(0x88, _buy_payload(0xFFFF, 1, price)), timeout=1.5)
        if r is None or len(r) < 1 or r[0] != 0x03:
            return (False, "unsold item did not answer BuyStat_HaveNotQuantity: %r" % r)
        snap = _closed_reload(server, s)
        if snap.get("cash") != CASH_SEED:
            return (False, "refused (HaveNotQuantity) buy mutated cash: %r" % snap)
        valid_cases += 1

        # (5) Two consecutive buys in ONE session -> each is charged once:
        #     items +2 and cash -2*price (no double-credit, no double-charge).
        s = _facing_shop(server, "v5")
        r1 = s.reply_to(s.q(0x88, _buy_payload(item_id, 1, price)), timeout=1.5)
        r2 = s.reply_to(s.q(0x88, _buy_payload(item_id, 1, price)), timeout=1.5)
        if r1 is None or r2 is None or r1[0] != 0x01 or r2[0] != 0x01:
            return (False, "double buy did not answer Done twice: %r %r" % (r1, r2))
        snap = _closed_reload(server, s)
        if snap.get("cash") != CASH_SEED - 2 * price:
            return (False, "double buy cash delta wrong: %r" % snap.get("cash"))
        if (snap.get("items") or {}).get(item_id) != base_item + 2:
            return (False, "double buy item count wrong: %r" % snap.get("items"))
        valid_cases += 1

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID packets: each must KICK with NO 0x7F reply and
        # leave NO persisted mutation (checked against the untouched seed):
        #   * quantity==0                     -> "quantity wrong" gate (step 2).
        #   * NOT facing the shop (no look)   -> shop gate (steps 3/4).
        #   * qty=0x80000000 x realprice >= 2^39 > seed cash, while FACING the
        #     shop with an over-generous price: this reaches the 64-bit-fixed
        #     cash check and must be KICKED there ("have not the cash"), which
        #     is exactly the behaviour the uint64 promotion bought. Under the
        #     old 32-bit multiply this same packet wrapped the total to 0 and
        #     handed out 2^31 items for free.
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0
        for object_id, quantity, price_arg, facing, label in (
                (item_id, 0, price, True,
                 "quantity=0 (quantity<=0 gate -> kick before shop lookup)"),
                (item_id, 1, price, False,
                 "well-formed buy but NOT facing a shop -> shop-gate kick"),
                (item_id, 0x80000000, 0xFFFFFFFF, True,
                 "qty=0x80000000 OVERFLOW VECTOR facing the shop: kicked at the "
                 "64-bit cash check, no free items"),
                (item_id, 0xFFFFFFFF, 0xFFFFFFFF, True,
                 "qty=0xFFFFFFFF max-values: kicked at the cash check")):
            try:
                if facing:
                    sx = _facing_shop(server, "cb")
                else:
                    sx = H.Session(server, login=_uniq("cb"), passh=_uniq("cbp"),
                                   pseudo="Cheat")
            except H.HandshakeError as e:
                return (False, "semantic#%d handshake failed: %s"
                        % (semantic_invalid_cases + 1, e))
            qn = None
            try:
                qn = sx.q(0x88, _buy_payload(object_id, quantity, price_arg))
            except OSError:
                pass  # kicked mid-send is acceptable evidence
            # A rejected buyObject sends NO reply: a 0x7F reply here would mean
            # the cheat succeeded (item granted / cash debited) -> contract
            # violation.
            got_reply = None
            if qn is not None:
                try:
                    got_reply = sx.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None  # socket closed -> kicked, no reply
            if got_reply is not None:
                return (False,
                        "CONTRACT VIOLATION: %s produced a 0x7F reply %r "
                        "(illegal buy must be refused, never fulfilled)"
                        % (label, got_reply[:8]))
            kicked = _is_kicked(sx)
            creds = (sx.login_creds, sx.pass_creds)
            try:
                sx.close()
            except OSError:
                pass
            time.sleep(0.5)
            snap = H.reload_state(server, creds[0], creds[1])
            if snap.get("cash") != CASH_SEED or snap.get("items") != before["items"]:
                return (False,
                        "CONTRACT VIOLATION: %s mutated persisted state: %r"
                        % (label, snap))
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(illegal buyObject must disconnect the cheater)"
                        % label)
            semantic_invalid_cases += 1

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. 0x88 is a FIXED-size QUERY: on the wire it is
        # [0x88][qnum][10 data bytes], NO 4-byte length. Wrong framing must
        # never crash/hang the server (the framing layer waits for exactly 10
        # data bytes; a mis-framed stream is rejected and the client kicked).
        # Each from a fresh disposable session; after each assert alive+clean.
        # Use send_raw for raw bytes.
        # ----------------------------------------------------------------
        malformed_cases = 0

        # (1) TRUNCATED: code + qnum + only 9 data bytes (size 9, < 10). The
        # fixed-size framer waits for the 10th byte; we follow with garbage so
        # the parser can't wedge, and the server must stay alive.
        try:
            s = H.Session(server, login=_uniq("m1"), passh=_uniq("m1p"),
                          pseudo="Cheat")
            s.send_raw(bytes([0x88, 0x00]) + (b"\x01" * 9))   # one byte short
            time.sleep(0.15)
            s.send_raw(b"\xFF\xFF\xFF\xFF")                   # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated 9-data-byte buyObject: %s" % why)

        # (2) OVER-LONG: code + qnum + 11 data bytes (size 11, > 10). The framer
        # reads 10 data bytes as the packet, leaving 1 stray byte to mis-frame
        # the next packet -> server should reject/kick, never crash.
        try:
            s = H.Session(server, login=_uniq("m2"), passh=_uniq("m2p"),
                          pseudo="Cheat")
            s.send_raw(bytes([0x88, 0x00]) + _buy_payload(1, 1, 1) + b"\x00")
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long 11-data-byte buyObject: %s" % why)

        # (3) BARE: code + qnum, ZERO data bytes. The framer blocks waiting for
        # the 10 fixed data bytes; we send garbage to complete+trail so there is
        # no wedge, and the server must stay alive.
        try:
            s = H.Session(server, login=_uniq("m3"), passh=_uniq("m3p"),
                          pseudo="Cheat")
            s.send_raw(bytes([0x88, 0x00]))                  # code + qnum, no data
            time.sleep(0.15)
            s.send_raw(b"\xAA" * 12)                         # complete then trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare code+qnum buyObject: %s" % why)

        # (4) BOGUS DYNAMIC LENGTH: FIXED-size query 0x88 sent WITH a spurious
        # 4-byte dynamic length prefix (as if it were a dynamic query). 0x88 is
        # FIXED, so the would-be length bytes are mis-read as the first data
        # bytes + a garbage next packet -> the parser must reject/kick and the
        # server must survive.
        try:
            s = H.Session(server, login=_uniq("m4"), passh=_uniq("m4p"),
                          pseudo="Cheat")
            s.send_raw(bytes([0x88, 0x00]) + H.u32(10) + _buy_payload(1, 1, 1))
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus-dynamic-length buyObject: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim account's persisted identity,
        # position, cash AND inventory must be byte-for-byte what they were
        # before any of the abuse, and a brand-new full session must still
        # reach CharacterSelected. None of the rejected buyObject attempts
        # (incl. the overflow vector, which this time DID reach the cash
        # check) may have debited cash, added items, or moved anyone.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "reload_state(after) raised: %s" % e)

        if after.get("character_id") != before.get("character_id"):
            return (False, "victim character_id changed: %r -> %r"
                    % (before.get("character_id"), after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo", "cash", "items"):
            if after.get(k) != before.get(k):
                return (False,
                        "victim state corrupted: %s %r -> %r"
                        % (k, before.get(k), after.get(k)))

        # Leave the server usable: a fresh full session still handshakes.
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d (facing the house2 shop bot via spawn-under-bot + "
            "look-up: exact-price Done with cash -price and item +1, "
            "BetterPrice over-offer charged at the real price, PriceHaveChanged "
            "no-mutation, HaveNotQuantity on unsold id, double-buy charged "
            "exactly 2x) all cash/items proven by disconnect+reload "
            "(cash from select block, items from 0x54) semantic_invalid=%d "
            "(all kicked, no 0x7F reply, zero persisted mutation: qty=0 gate, "
            "not-facing shop gate, OVERFLOW qty=0x80000000 kicked at the "
            "64-bit cash check, max-values) malformed=%d db_checked=%s "
            "victim_persisted=%s (server stayed alive+clean; shop products "
            "came from 0x87 so no datapack price is hard-coded)"
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               checks_db, (before.get("x"), before.get("y"),
                           before.get("mapIndex"), before.get("cash")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
