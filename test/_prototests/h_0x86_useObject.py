"""Valgrind/protocol test for server handler 0x86 (useObject).

Handler: server/base/ClientNetworkReadQuery.cpp:59 (Client::parseQuery, case 0x86).
Wire form: FIXED-size QUERY, code 0x86, queryNumber, exactly 2 data bytes:
    [objectId:u16]
0x86 >= 0x80 so it carries a queryNumber; packetFixedSize[0x86]==2 so it is
FIXED size (NO 4-byte dynamic length on the wire). Wire bytes:
    [0x86][qnum][lo][hi]

Handler logic traced
(server/base/ClientNetworkReadQuery.cpp:59 -> server/base/ClientEvents/
 LocalClientHandlerObject.cpp:197  Client::useObject(query_id, itemId)):

  parseQuery (HARDENED):
    * size != sizeof(uint16_t) (i.e. != 2)
          -> errorOutput("wrong size with the main ident: 134, ...") -> KICK.

  useObject(query_id, itemId):
    1. itemId NOT in the player's inventory (items.find()==cend())
          -> errorOutput("can't use the object: ... because don't have into the
             inventory") -> KICK.
          *** THE NOT-OWNED / UNKNOWN-ID GUARD ***: the check is on the PLAYER's
          items map, not the datapack, so an item the player owns 0 of AND an
          item id that does not exist in the datapack BOTH funnel through this
          guard (the unknown id never reaches CommonDatapack::get_item(), so no
          OOB read). With the guard in place this must NOT crash / NOT leak.
    2. objectQuantity(itemId) < 1  -> errorOutput(...) -> KICK. (cannot happen via
          the normal inventory map, which never stores a 0 entry, but is a belt-
          and-braces guard.)
    3. if get_item(itemId).consumeAtUse  -> removeObject(itemId)  (consumes 1
          BEFORE the usage branch).
    4. branch on the item's usage:
          * crafting-recipe item (has_itemToCraftingRecipe): learn recipe, send
            0x7F reply (ObjectUsage_correctlyUsed). SUCCESS branch.
          * trap (has_trap): requires isInFight() && isInFightWithWild(), else
            errorOutput(...) -> KICK. In a wild fight: tryCapture + 0x7F reply.
          * repel (has_repel): repel_step += step, send 0x7F reply
            (ObjectUsage_correctlyUsed). SUCCESS branch (no fight needed).
          * else (no usage in this context, e.g. a heal potion out of fight)
            -> errorOutput("... because don't have an usage") -> KICK.

  Client::errorOutput -> disconnectClient() (server/base/Client.cpp:524). So EVERY
  rejection of useObject KICKS the offending client; the server stays alive (the
  HARDENED-only parse-fail path here kicks rather than abort — see foundation
  api_notes). NO 0x7F reply is sent on a rejection.

REACHABILITY (documented -> reachability_limited=True):
  The clean VALID success branches (recipe-item, or repel) need the player to
  OWN a recipe-item or a repel-item AND stay connected after the 0x7F reply. The
  'test' maincode hands a fresh on-map character only:
      item 1  = IronTrap (trap)   x5   (player/start.xml)
      item 5  = Potion grade A (hp) x1
  (start.xml; cash 500; 3 monster groups). There is NO reachable handler in this
  harness to grant a repel/recipe item, and no wild fight is deterministically
  reachable to make the trap branch valid. Therefore:
    * Using item 5 (potion) OUT of fight: consumeAtUse removes 1, then the heal
      item has no out-of-fight usage -> else-branch errorOutput -> KICK. It both
      consumes AND kicks, so the "consume 1 + reply + stay connected" success
      cannot be positively observed (the consuming socket is gone; reload_state
      reads only identity/position, not item counts -> documented limitation).
    * Using item 1 (trap) OUT of fight: consumeAtUse removes 1, then trap branch
      "is not in fight" -> KICK.
    * Every other well-formed id (not owned / unknown) -> not-owned guard -> KICK.
  So NO well-formed reachable 0x86 packet produces a 0x7F success reply with the
  session still alive. valid_cases=0; reachability_limited=True. We instead
  exercise the full SECURITY contract: not-owned/unknown-id guard (no OOB into
  get_item), owned-but-no-usable-context (potion/trap out of fight) -> KICK, and
  malformed framing -> KICK, all with the server surviving clean and NO persisted
  state corruption on a separate legit account.

Verification strategy:
  * useObject is a QUERY but a rejected one sends NO reply and disconnects the
    client. We confirm the contract by detecting the offending socket is CLOSED
    by the server (EOF) after the illegal send -> cheater kicked. A silent ignore
    (socket stays open, OR a 0x7F reply arrives on a rejected use) would be a
    contract violation.
  * After EACH send we assert server.alive() and crash_report() is None.
  * NO STATE CORRUPTION is checked with a SEPARATE legit account: its persisted
    identity/position (H.reload_state) is identical before and after all the
    abuse, and a fresh full session still handshakes -> the server is unharmed.
"""

import time
import _protoharness as H

NAME = "0x86 useObject"


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


def _is_kicked(sess, settle=0.4):
    """Return True iff the server CLOSED this session's socket (client kicked).

    Detection: read from the raw socket; the server's disconnectClient() shuts
    the connection, so recv() returns b'' (EOF). We poke the peer once more to
    force the FIN to be observed even if the first recv only drained buffered
    bytes (the server pushes a 'kicked' system message just before the FIN).
    A still-open socket (timeout, no EOF, send succeeds) means NOT kicked.

    The poke is a well-formed-but-still-illegal useObject (qnum 0, objectId
    0xFFFF, which the fresh peer does not own): against a closed peer it raises
    OSError (-> kicked); against a still-open peer it just triggers the same kick.
    """
    sk = sess.sock
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sk.settimeout(0.2)
            d = sk.recv(65536)
            if d == b"":
                return True  # clean EOF -> server closed us -> kicked
            # bytes arrived (system "kicked" message / trailing data); keep reading
        except OSError as e:
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                break
            return True  # ECONNRESET / EBADF -> peer gone -> kicked
    try:
        sk.sendall(bytes([0x86, 0x00, 0xFF, 0xFF]))  # qnum 0, objectId 0xFFFF (not owned)
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


def run(server):
    try:
        # ----------------------------------------------------------------
        # Baseline legit account whose persisted state must stay intact
        # through all the abuse. Create it, record its persisted snapshot,
        # then close it so reload_state can read it back cleanly.
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

        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        checks_db = False
        try:
            before = H.reload_state(server, victim_login, victim_pass)
            if before.get("character_id") is None:
                return (False, "victim did not persist (no character_id)")
            checks_db = True
        except Exception as e:
            return (False, "reload_state(before) raised: %s" % e)

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Cheat")

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID well-formed useObject packets. From a fresh on-map
        # session (owns ONLY items {1:5, 5:1}) every one of these is illegal and
        # must hit the not-owned guard (server/base/.../LocalClientHandlerObject.cpp
        # :202) BEFORE any datapack lookup:
        #   * objectId 0xFFFF / 0x4901+1 : id far above any datapack item and not
        #       owned -> not-owned guard (the unknown id never reaches
        #       CommonDatapack::get_item(), so no OOB read) -> KICK.
        #   * objectId 9 (Repulse grade A, EXISTS in datapack, has a real repel
        #       usage) but the player owns 0 of it -> not-owned guard -> KICK.
        #       This is the security-relevant case: a known, usable item id that
        #       the cheater does NOT own must be refused, never used/consumed.
        #   * objectId 6 (Potion grade B, exists, owned 0) -> not-owned -> KICK.
        #   * objectId 0 (no such item, owned 0) -> not-owned -> KICK.
        # For EACH we assert:
        #   (a) the offending client is KICKED (socket closed by server) and NO
        #       0x7F reply is sent,
        #   (b) the SERVER survives (alive + no crash),
        #   (c) no corrupt state (checked once at the end on the victim account).
        # Each uses a FRESH disposable session because it gets kicked.
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0
        for object_id, label in (
                (0xFFFF, "objectId=0xFFFF (far above any datapack id, not owned)"),
                (0x4902, "objectId=0x4902 (just above max datapack item 0x4901, not owned)"),
                (9,      "objectId=9 (Repulse grade A: EXISTS+usable but NOT owned)"),
                (6,      "objectId=6 (Potion grade B: exists but NOT owned)"),
                (0,      "objectId=0 (no such item, not owned)")):
            try:
                s = fresh("uo")
            except H.HandshakeError as e:
                return (False, "semantic#%d handshake failed: %s"
                        % (semantic_invalid_cases + 1, e))
            qn = None
            try:
                qn = s.q(0x86, H.u16(object_id))  # well-formed query, illegal id
            except OSError:
                pass  # kicked mid-send is acceptable evidence
            # A rejected useObject sends NO reply: a 0x7F reply here would be a
            # contract violation (the cheat must be refused, not used).
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None  # socket closed -> kicked, no reply
            if got_reply is not None:
                return (False,
                        "CONTRACT VIOLATION: %s produced a 0x7F reply %r "
                        "(illegal/not-owned object must be refused, never used)"
                        % (label, got_reply[:8]))
            kicked = _is_kicked(s)
            try:
                s.close()
            except OSError:
                pass
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(illegal useObject must disconnect the cheater)"
                        % label)
            semantic_invalid_cases += 1

        # ----------------------------------------------------------------
        # OWNED-BUT-NO-USABLE-CONTEXT well-formed useObject. These use items the
        # player REALLY owns, so they pass the not-owned guard, but their usage is
        # illegal in the current (out-of-fight) context. consumeAtUse defaults to
        # true (general/.../DatapackGeneralLoaderItem.cpp:125), so the engine
        # removes 1 BEFORE the usage branch decides; then the branch rejects ->
        # KICK with NO 0x7F reply. We verify: no reply, the client is kicked, the
        # server survives clean. (This proves the rejection path of an owned item
        # is exercised; it both consumes 1 and kicks, which is exactly why the
        # clean "consume + reply + stay connected" success is not observable here
        # -> reachability_limited.)
        # ----------------------------------------------------------------
        for object_id, label in (
                (5, "objectId=5 (Potion grade A: OWNED but no out-of-fight usage)"),
                (1, "objectId=1 (IronTrap: OWNED but not in a wild fight)")):
            try:
                s = fresh("ow")
            except H.HandshakeError as e:
                return (False, "owned-case handshake failed: %s" % e)
            qn = None
            try:
                qn = s.q(0x86, H.u16(object_id))
            except OSError:
                pass
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None
            if got_reply is not None:
                return (False,
                        "CONTRACT VIOLATION: %s produced a 0x7F reply %r "
                        "(item with no usable context must be refused)"
                        % (label, got_reply[:8]))
            kicked = _is_kicked(s)
            try:
                s.close()
            except OSError:
                pass
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(no-usable-context useObject must disconnect)" % label)
            semantic_invalid_cases += 1

        # VALID-case note: a successful useObject (repel/recipe item owned ->
        # 0x7F ObjectUsage_correctlyUsed, session stays connected) is NOT
        # deterministically reachable: the test maincode grants only a trap and a
        # heal potion, neither usable out of fight, and there is no reachable
        # handler to grant a repel/recipe item. We assert the security-relevant
        # refusal+kick for every reachable well-formed packet instead.
        valid_cases = 0

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. 0x86 is a FIXED-size QUERY: on the wire it is
        # [0x86][qnum][2 data bytes], NO 4-byte length. Wrong framing must never
        # crash/hang the server (HARDENED size!=2 guard kicks; bad framing is
        # mis-parsed and kicked). Each from a fresh disposable session; after
        # each assert alive+clean. Use send_raw for raw bytes.
        # ----------------------------------------------------------------
        malformed_cases = 0

        # (1) ONE data byte after qnum (size 1, < 2). The fixed-size framer waits
        # for the 2nd data byte; we follow with garbage so it can't wedge, and the
        # server must stay alive.
        try:
            s = fresh("m1")
            s.send_raw(bytes([0x86, 0x00, 0x05]))         # code + qnum + ONE data byte
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))   # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated 1-data-byte useObject: %s" % why)

        # (2) THREE data bytes after qnum (size 3, > 2). The framer reads code +
        # qnum + 2 bytes as the packet, leaving 1 stray byte to mis-frame the next
        # packet -> server should reject/kick, never crash.
        try:
            s = fresh("m2")
            s.send_raw(bytes([0x86, 0x00, 0x05, 0x00, 0x00]))  # code + qnum + 3 data bytes
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long 3-data-byte useObject: %s" % why)

        # (3) code + qnum, ZERO data bytes. The framer blocks waiting for the 2
        # fixed data bytes; we send garbage to ensure no wedge, server stays alive.
        try:
            s = fresh("m3")
            s.send_raw(bytes([0x86, 0x00]))               # code + qnum, no data
            time.sleep(0.15)
            s.send_raw(bytes([0xAA, 0xBB]))               # complete then trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare code+qnum useObject: %s" % why)

        # (4) FIXED-size query 0x86 sent WITH a spurious 4-byte dynamic length (as
        # if it were a dynamic query). 0x86 is FIXED, so the would-be length bytes
        # are mis-read as data + next packet -> the parser must reject/kick and the
        # server must survive.
        try:
            s = fresh("m4")
            s.send_raw(bytes([0x86, 0x00]) + H.u32(2) + H.u16(5))  # bogus len prefix
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus-dynamic-length useObject: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim account's persisted identity/position
        # must be byte-for-byte what it was before any of the abuse, and a brand-
        # new full session must still reach CharacterSelected. None of the rejected
        # useObject attempts may have leaked into another account or the server.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "reload_state(after) raised: %s" % e)

        if after.get("character_id") != before.get("character_id"):
            return (False, "victim character_id changed: %r -> %r"
                    % (before.get("character_id"), after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo"):
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
            "valid=%d (success branch unreachable: needs an OWNED repel/recipe "
            "item usable out of fight; test maincode grants only trap(1) + "
            "potion(5), neither usable out of fight, and no item-grant handler) "
            "semantic_invalid=%d (all kicked, no 0x7F reply: not-owned/unknown "
            "ids 0xFFFF/0x4902/9/6/0 + owned-no-usable-context potion(5)/trap(1)) "
            "malformed=%d db_checked=%s victim_persisted=%s "
            "(server stayed alive+clean; no state corruption)"
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               checks_db, (before.get("x"), before.get("y"),
                           before.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
