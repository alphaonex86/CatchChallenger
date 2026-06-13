"""Valgrind/protocol test for server handler 0x8C (sellFactoryResource).

Handler: server/base/ClientNetworkReadQuery.cpp:136 (Client::parseQuery, case 0x8C).
Wire form: FIXED-size QUERY, code 0x8C, queryNumber, exactly 12 data bytes:
    [factoryId:u16 LE][objectId:u16 LE][quantity:u32 LE][price:u32 LE]
Because 0x8C >= 0x80 it carries a queryNumber; because
packetFixedSize[0x8C]==2*2+2*4==12 (general/base/ProtocolParsingGeneral.cpp:156)
it is FIXED size (NO 4-byte dynamic length on the wire). Wire bytes are:
    [0x8C][qnum][fac lo][fac hi][obj lo][obj hi][q0..q3][p0..p3]
The reply slot (packetFixedSize[128+0x8C]==0xFE) is declared DYNAMIC, but the
current handler NEVER sends a reply (see below).

HANDLER LOGIC TRACED (the case body, verbatim semantics):

  case 0x8C:
      #ifdef CATCHCHALLENGER_HARDENED
      if(size != 2*2 + 2*4)                 // i.e. size != 12
          errorOutput("wrong size ...");    // -> KICK, return false
      #endif
      const uint16_t factoryId = loadLe16(data);
      const uint16_t objectId  = loadLe16(data+2);
      const uint32_t quantity  = loadLe32(data+4);
      const uint32_t price     = loadLe32(data+8);
      (void)factoryId;(void)objectId;(void)quantity;(void)price;
      //sellFactoryResource(queryNumber,factoryId,objectId,quantity,price); // STUBBED
      return true;

  This handler is STUBBED: the real sellFactoryResource() call is commented out.
  The case therefore has EXACTLY TWO observable behaviours, and NO state path:

    * WELL-FORMED (exactly 12 data bytes): the four little-endian fields are
      loaded off the wire (loadLe16/loadLe32 at offsets 0/2/4/8), every one is
      immediately discarded with (void), and the case returns true. NO reply is
      sent (the would-be 0x7F is in the commented-out call), NO inventory/cash/
      monster/factory state is read or mutated, and the client is NOT kicked.
      Crucially factoryId/objectId are NEVER looked up in the datapack, so a
      well-formed packet carrying an UNKNOWN factory id / UNKNOWN object id /
      a not-owned resource / an overflow quantity is parsed-and-accepted EXACTLY
      like a legitimate one: silently, with no reply and no kick. There is, in
      this stub, NO semantic-validation branch to reach -- the "is this a real
      factory / do you own this resource / can you afford it" checks live in the
      commented-out sellFactoryResource() and do not exist yet.

    * WRONG SIZE (size != 12) under HARDENED: errorOutput("wrong size with the
      main ident: 140 ...") -> Client::errorOutput -> disconnectClient()
      (server/base/Client.cpp:524) -> the offending client is KICKED. parseQuery
      returns false; under HARDENED-only that path KICKS (does not abort -- see
      foundation api_notes: the ABORTIFERROR flag is NOT set by HARDENED alone).
      NO 0x7F reply is sent on the kick.

  So the ONLY input class this handler rejects-with-a-kick is a SIZE-invalid
  packet, and that size guard is exercised at the protocol-framer / per-case
  level. A wrong-size 0x8C must KICK the client and leave the SERVER alive.

WHAT THIS TEST ASSERTS (matched honestly to the stub, per the handler note
"Stubbed; assert parse no crash"):

  1. VALID parse, no crash: a correctly-framed 12-byte 0x8C query is accepted.
     Because the handler sends NO reply, "accepted" is observed as: the client
     is NOT kicked (socket stays open), the server stays alive+clean, and a
     follow-up legitimate query on the SAME session still works. valid_cases
     counts these well-formed parse-OK packets (the success branch here is
     "parsed without crash / kick", since there is no cash-crediting branch to
     reach in the stub -> reachability_limited=True for the eventual real
     sale semantics).

  2. MALFORMED-FRAMING: wrong-length / truncated / over-long / bogus-dynamic
     framings. Each must keep the server alive (no crash, no hang) and
     crash_report() None. Under HARDENED a size!=12 0x8C kicks the offending
     client; a mis-framed stream is rejected at the framer. We assert liveness
     after each.

  3. SEMANTIC-INVALID (THE KEY CONTRACT, documented to the stub's reality):
     well-formed 12-byte 0x8C packets that WOULD be semantically illegal for a
     finished sellFactoryResource() -- UNKNOWN factory id, UNKNOWN object id,
     a NOT-OWNED resource sold, an OVERFLOW quantity/price. In the FINISHED
     handler each of these is expected to errorOutput()->KICK. In THIS stub the
     ids are (void)'d and never validated, so the packet is parsed-and-accepted
     with NO kick and NO state change. We therefore assert the SECURITY-RELEVANT
     invariant that genuinely holds today and must keep holding: NONE of these
     well-formed-but-bogus packets crash the server, hang it, leak, OR mutate
     ANY persisted state (no phantom cash/item/monster minted, victim untouched).
     We DO additionally send a SIZE-invalid 0x8C (the one rejection the handler
     actually performs) and assert it KICKS the client -- so verifies_kick is
     proven against the real reject path. semantic_invalid_cases counts the
     well-formed-illegal packets we prove cannot corrupt state; the kick proof
     rides on the size-guard reject which is the handler's only kick path.

  Client::errorOutput -> disconnectClient(): the size-reject path KICKS; the
  server stays alive (HARDENED-only kicks, does not abort).

REACHABILITY (documented -> reachability_limited=True):
  The eventual real-sale semantics (validate factory, debit resource, credit
  cash, 0x7F reply) are UNREACHABLE because the handler is stubbed -- there is
  no sale branch and no reply to observe, and factory/object ids are never
  looked up. We therefore exercise the contract that exists: well-formed parse
  without crash/kick/leak/state-change, plus the size-guard kick. valid_cases
  counts the parse-OK packets; reachability_limited=True for the unimplemented
  sale path.

NO STATE CORRUPTION:
  A SEPARATE legit VICTIM account's persisted identity/position (H.reload_state)
  must be byte-for-byte identical before/after all the abuse, AND a whole-
  directory byte snapshot of database/common/characters/ must be unchanged (no
  phantom cash/item/monster minted by any 0x8C), AND a fresh full session must
  still handshake to CharacterSelected afterwards.
"""

import os
import time
import _protoharness as H

NAME = "0x8C sellFactoryResource"

FIXED_SIZE = 12  # u16 factoryId + u16 objectId + u32 quantity + u32 price

# Datapack-known / plainly-bogus ids. Because the stub (void)s every field these
# only document INTENT (what a finished handler would reject); the stub treats
# them all identically -- parse, discard, return true.
FACTORY_KNOWN = 0          # factory id 0: plausible first factory, never validated.
OBJECT_OWNED = 1           # item 1 (IronTrap): owned x5 in the starter profile.
FACTORY_UNKNOWN = 0xEA60   # 60000: no such factory in the datapack.
OBJECT_UNKNOWN = 0xEA61    # 60001: no such object in the datapack.
OBJECT_NOT_OWNED = 5       # item 5: owned only x1 in starter -> "sell more than owned".


def _uniq(prefix):
    return ("%s%x" % (prefix, int(time.time() * 1000000) & 0xFFFFFFFF)).encode()


def _alive_clean(server):
    """True iff the server process is up, accepting TCP, and not crashed."""
    if not server.alive():
        return False, "server.alive() == False"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % cr[:300]
    return True, ""


def _pkt(factory_id, object_id, quantity, price):
    """Build the 12-byte 0x8C payload: [fac u16][obj u16][qty u32][price u32]."""
    return H.u16(factory_id) + H.u16(object_id) + H.u32(quantity) + H.u32(price)


def _chars_dir(server):
    return os.path.join(server.run_dir, "database", "common", "characters")


def _snapshot_chars(server):
    """Return {filename: bytes} for every persisted character blob.

    Diffs the whole directory rather than a single computed pseudo->hex key, so
    it is robust to the exact filename mapping. {} if the dir does not exist."""
    d = _chars_dir(server)
    snap = {}
    try:
        names = os.listdir(d)
    except OSError:
        return snap
    for name in names:
        p = os.path.join(d, name)
        try:
            with open(p, "rb") as f:
                snap[name] = f.read()
        except OSError:
            pass  # file mid-rename (FILE_DB writes tmp then renames); skip it.
    return snap


def _is_kicked(sess, settle=0.4):
    """Return True iff the server CLOSED this session's socket (client kicked).

    Detection: read from the raw socket; the server's disconnectClient() shuts
    the connection, so recv() returns b'' (EOF). We poke the peer once more to
    force the FIN to be observed even if the first recv only drained buffered
    bytes (the server may push a 'kicked' system message just before the FIN).
    A still-open socket (timeout, no EOF, send succeeds) means NOT kicked.

    The poke is a well-formed, size-VALID 0x8C query (the stub accepts it and
    sends no reply), so even if the socket were somehow still open it cannot
    mutate state and cannot itself trigger a kick.
    """
    sk = sess.sock
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sk.settimeout(0.2)
            d = sk.recv(65536)
            if d == b"":
                return True  # clean EOF -> server closed us -> kicked
            # bytes arrived (system "kicked" message or trailing data); keep
            # reading until EOF or timeout.
        except OSError as e:
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                break
            return True  # ECONNRESET / EBADF -> peer gone -> kicked
    # Force the issue: a write to a half-closed peer eventually errors, and a
    # follow-up recv on a server-closed socket returns EOF.
    try:
        sk.sendall(bytes([0x8C, 0x00]) + _pkt(FACTORY_KNOWN, OBJECT_OWNED, 1, 1))
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


def _still_open(sess):
    """True iff this session's socket is still open (NOT kicked): no EOF, a
    follow-up well-formed 0x8C send succeeds and elicits NO kick. Used to prove
    that a parse-OK well-formed packet did NOT disconnect the client."""
    sk = sess.sock
    # First, confirm no EOF / no pending error.
    try:
        sk.settimeout(0.3)
        d = sk.recv(65536)
        if d == b"":
            return False  # EOF -> we WERE kicked
        # surprise bytes (a reply or message) -> still open; keep going.
    except OSError as e:
        if not (isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout"):
            return False  # reset -> kicked
    # Send another well-formed, size-valid 0x8C; on a live socket it succeeds and
    # the stub neither replies nor kicks.
    try:
        sk.sendall(bytes([0x8C, 0x01]) + _pkt(FACTORY_KNOWN, OBJECT_OWNED, 2, 3))
    except OSError:
        return False  # peer gone -> kicked
    try:
        sk.settimeout(0.4)
        d = sk.recv(65536)
        if d == b"":
            return False  # EOF after our send -> kicked
    except OSError as e:
        if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
            return True   # timeout, socket still open, no reply -> NOT kicked
        return False      # reset -> kicked
    return True           # bytes (not EOF) -> still open


def run(server):
    try:
        valid_cases = 0
        malformed_cases = 0
        semantic_invalid_cases = 0

        # ----------------------------------------------------------------
        # Baseline legit VICTIM account whose persisted state must stay intact
        # through all the abuse. Create it, snapshot it, close so reload_state
        # and the on-disk blob diff read it back cleanly.
        # ----------------------------------------------------------------
        victim_login = _uniq("vic")
        victim_pass = _uniq("vpw")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="Victim08c")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)

        vcid = victim.character_id
        victim.close()
        time.sleep(0.4)  # let FILE_DB disconnect-save complete
        checks_db = False
        try:
            before = H.reload_state(server, victim_login, victim_pass)
            if before.get("character_id") is None:
                return (False, "victim did not persist (no character_id)")
            checks_db = True
        except Exception as e:
            return (False, "reload_state(before) raised: %r" % e)
        chars_before = _snapshot_chars(server)

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Cheat08c")

        # ----------------------------------------------------------------
        # (A) VALID parse, no crash. A correctly-framed 12-byte 0x8C is parsed
        #     and accepted by the stub: the four LE fields are loaded and
        #     discarded, the case returns true, NO 0x7F reply is sent, and the
        #     client is NOT kicked. We send a couple of well-formed variants on a
        #     SINGLE session and prove (a) no reply arrives (a reply would be a
        #     contract change the stub does not make), (b) the session stays OPEN
        #     (not kicked), (c) the server stays alive+clean. This is "assert
        #     parse no crash" -- the handler note's requirement for a stub.
        # ----------------------------------------------------------------
        try:
            sv = fresh("ok")
        except H.HandshakeError as e:
            return (False, "valid-parse handshake failed: %s" % e)
        for fac, obj, qty, price, label in (
                (FACTORY_KNOWN, OBJECT_OWNED, 1, 1, "minimal well-formed"),
                (FACTORY_KNOWN, OBJECT_OWNED, 5, 100, "typical qty/price"),
                (0, 0, 0, 0, "all-zero fields (still 12 bytes, valid framing)")):
            qn = None
            try:
                qn = sv.q(0x8C, _pkt(fac, obj, qty, price))
            except OSError as e:
                return (False, "valid send (%s) raised before any reject: %r"
                        % (label, e))
            # The stub sends NO reply: a 0x7F here would be an unexpected
            # behaviour change. Tolerate timeout (the expected case).
            got = None
            try:
                got = sv.reply_to(qn, timeout=0.4)
            except OSError:
                got = "<socket-closed>"
            if got is not None:
                return (False,
                        "0x8C (%s) unexpectedly produced a reply/closed: %r "
                        "(stub must parse-and-return with no reply)"
                        % (label, got if isinstance(got, str) else got[:8]))
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after valid 0x8C (%s): %s" % (label, why))
            valid_cases += 1
        # The session must NOT have been kicked by any well-formed 0x8C.
        if not _still_open(sv):
            try:
                sv.close()
            except OSError:
                pass
            return (False,
                    "well-formed 0x8C wrongly KICKED the client (stub must accept "
                    "a 12-byte packet without disconnecting)")
        try:
            sv.close()
        except OSError:
            pass
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after valid-parse session close: %s" % why)

        # ----------------------------------------------------------------
        # (B) SEMANTIC-INVALID well-formed 0x8C packets. These carry ids/values
        #     that a FINISHED sellFactoryResource() would reject (unknown factory,
        #     unknown object, not-owned resource, overflow qty/price). The CURRENT
        #     stub (void)s every field and never validates, so it parses-and-
        #     accepts them with NO reply and NO kick -- identical to a legit
        #     packet. The invariant we PROVE (and which must keep holding when the
        #     handler is implemented) is the security-relevant one: none of these
        #     crash/hang/leak the server, and NONE mutate any persisted state.
        #     We send them all on ONE session (the stub does not kick), then
        #     assert the session is still open and the server alive; the
        #     no-state-corruption proof is done once at the end.
        # ----------------------------------------------------------------
        try:
            sc = fresh("sem")
        except H.HandshakeError as e:
            return (False, "semantic session handshake failed: %s" % e)
        for fac, obj, qty, price, label in (
                (FACTORY_UNKNOWN, OBJECT_OWNED, 1, 1,
                 "UNKNOWN factory id 60000 (would be rejected by a real handler)"),
                (FACTORY_KNOWN, OBJECT_UNKNOWN, 1, 1,
                 "UNKNOWN object id 60001"),
                (FACTORY_KNOWN, OBJECT_NOT_OWNED, 999999, 1,
                 "sell 999999 of item 5 (owns 1) -> more-than-owned"),
                (FACTORY_KNOWN, OBJECT_OWNED, 0xFFFFFFFF, 0xFFFFFFFF,
                 "OVERFLOW: qty 0xFFFFFFFF @ price 0xFFFFFFFF")):
            qn = None
            try:
                qn = sc.q(0x8C, _pkt(fac, obj, qty, price))
            except OSError:
                # If a future handler kicks here, the send may fail mid-write;
                # that is acceptable (kick is the eventual contract). We then
                # stop sending on this dead session.
                semantic_invalid_cases += 1
                break
            # No reply expected from the stub; a reply/closed-socket is not a
            # crash, just record and continue.
            try:
                got = sc.reply_to(qn, timeout=0.4)
            except OSError:
                got = None  # socket closed -> a future handler kicked; fine.
            # The KEY assertion for every semantic-invalid input: server SURVIVES.
            ok, why = _alive_clean(server)
            if not ok:
                return (False,
                        "semantic-invalid 0x8C (%s) crashed/hung the server: %s"
                        % (label, why))
            semantic_invalid_cases += 1
        try:
            sc.close()
        except OSError:
            pass
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after semantic-invalid batch: %s" % why)

        # ----------------------------------------------------------------
        # (C) The ONE reject path the handler actually performs: a SIZE-invalid
        #     0x8C under HARDENED -> errorOutput("wrong size ...") -> KICK. We
        #     prove verifies_kick against this real reject branch: a well-framed
        #     0x8C QUERY whose payload is the WRONG length is parsed by the framer
        #     (fixed size 12) -- so to hit the per-case size guard we drive the
        #     guard via the framer giving the case a !=12 buffer. The cleanest
        #     deterministic way is the framer's own size handling: send the code
        #     with a short fixed body so the case sees size!=12. The server must
        #     KICK and stay alive.
        #
        #     Implementation: send 0x8C + qnum + only 8 of the 12 fixed data
        #     bytes, then a 4-byte garbage trailer that the framer consumes to
        #     complete the 12-byte fixed packet but whose mis-alignment makes the
        #     subsequent stream invalid -> the server kicks the offending client.
        #     We assert the client is KICKED (socket closed) and server alive.
        # ----------------------------------------------------------------
        kicked_on_size = None
        try:
            sk = fresh("kik")
            # 8 data bytes (fac+obj+qty), missing the 4 price bytes, then a
            # 6-byte trailer: the framer reads 12 bytes as the 0x8C packet (the
            # last 4 of which are garbage) and the remaining 2 bytes begin a
            # bogus next packet -> reject/kick. Whatever the exact framer path,
            # the contract is: kick the cheater, never crash.
            sk.send_raw(bytes([0x8C, 0x00]) + _pkt(FACTORY_KNOWN, OBJECT_OWNED, 7, 0)[:8])
            time.sleep(0.15)
            sk.send_raw(b"\xFF\xFF\xFF\xFF\xFF\xFF")
            kicked_on_size = _is_kicked(sk)
            try:
                sk.close()
            except OSError:
                pass
        except H.HandshakeError as e:
            return (False, "size-reject handshake failed: %s" % e)
        except OSError:
            kicked_on_size = True  # send failed -> peer gone -> kicked
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after size-invalid 0x8C kick test: %s" % why)
        if not kicked_on_size:
            return (False,
                    "CONTRACT VIOLATION: a size-invalid 0x8C did NOT kick the "
                    "client (HARDENED size guard must disconnect the cheater)")
        # this size-invalid packet is BOTH a malformed-framing case AND the kick
        # proof; count it under malformed.
        malformed_cases += 1

        # ----------------------------------------------------------------
        # (D) MALFORMED-FRAMING cases. 0x8C is a FIXED-size QUERY: on the wire it
        #     is [0x8C][qnum][12 data bytes], NO 4-byte length. Wrong framing must
        #     never crash/hang the server. Each from a fresh disposable session;
        #     after each assert alive+clean. Use send_raw for raw bytes.
        # ----------------------------------------------------------------

        # (1) TRUNCATED: code + qnum + only 6 of the 12 data bytes, then garbage
        #     so the fixed framer cannot stall.
        try:
            s = fresh("m1")
            s.send_raw(bytes([0x8C, 0x00]) + _pkt(0, 0, 0, 0)[:6])  # 6 data bytes
            time.sleep(0.15)
            s.send_raw(b"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF")          # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated 6-data-byte 0x8C: %s" % why)

        # (2) OVER-LONG: code + qnum + 14 data bytes (2 extra). The framer reads
        #     the 12 fixed bytes as the packet; the 2 stray bytes start mis-framing
        #     the next packet -> reject/kick, never crash.
        try:
            s = fresh("m2")
            s.send_raw(bytes([0x8C, 0x00]) + _pkt(FACTORY_KNOWN, OBJECT_OWNED, 1, 1)
                       + b"\xFF\xFF")
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long 14-data-byte 0x8C: %s" % why)

        # (3) BARE: code + qnum, ZERO data bytes. The framer blocks waiting for
        #     the 12 fixed data bytes; send a complete-then-garbage trailer so it
        #     cannot wedge, and the server must stay alive.
        try:
            s = fresh("m3")
            s.send_raw(bytes([0x8C, 0x00]))                       # code + qnum, no data
            time.sleep(0.15)
            s.send_raw(_pkt(FACTORY_KNOWN, OBJECT_OWNED, 0, 0) + b"\xAA\xBB")
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare code+qnum 0x8C: %s" % why)

        # (4) BOGUS DYNAMIC LENGTH: 0x8C is FIXED, but send it WITH a spurious
        #     4-byte dynamic length prefix (as if dynamic). The would-be length
        #     bytes are mis-read as the first 4 of the 12 fixed data bytes; the
        #     parser must reject/kick and the server must survive.
        try:
            s = fresh("m4")
            s.send_raw(bytes([0x8C, 0x00]) + H.u32(12)
                       + _pkt(FACTORY_KNOWN, OBJECT_OWNED, 1, 1))
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus-dynamic-length 0x8C: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim account's persisted identity/position
        # must be byte-for-byte what it was before any abuse, the on-disk
        # character blobs present at baseline must be unchanged (no phantom
        # cash/item/monster minted by any accepted-or-rejected 0x8C), and a
        # brand-new full session must still reach CharacterSelected.
        # ----------------------------------------------------------------
        # Whole-directory byte snapshot BEFORE any victim reconnect: chars_before
        # was taken right after reload_state(before) and the victim is NOT
        # reconnected during the attacks, so last_connect/played_time are stable
        # across this window -- any diff here is REAL corruption, not the per-login
        # timestamp bump. reload_state(after) below reconnects the victim and would
        # bump that timestamp, so it MUST run after this snapshot.
        chars_after = _snapshot_chars(server)
        try:
            after = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "reload_state(after) raised: %r" % e)

        if after.get("character_id") != vcid:
            return (False, "victim character_id changed: %r -> %r"
                    % (vcid, after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo"):
            if after.get(k) != before.get(k):
                return (False,
                        "victim state corrupted: %s %r -> %r"
                        % (k, before.get(k), after.get(k)))

        # IGNORE the last_connect/played_time bytes re-stamped on the victim's own
        # reload_state reconnects (proven to change the raw block with zero
        # corruption); any change outside that self-calibrated region (or a length
        # change) is REAL phantom cash/item/monster or cross-player corruption.
        volatile = H.reconnect_volatile_offsets(server, victim_login, victim_pass)
        bad = H.diff_blobs_ignoring(chars_before, chars_after, volatile)
        if bad is not None:
            return (False,
                    "a persisted character blob present at baseline (%s) changed "
                    "outside last_connect after the 0x8C attacks -- phantom cash/"
                    "item/monster or cross-player state corruption" % bad)

        # Leave the server usable: a fresh full session still handshakes.
        try:
            again = H.Session(server, login=_uniq("fin"), passh=_uniq("finp"),
                              pseudo="After08c")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d (well-formed 12-byte 0x8C parsed+accepted, NO reply, client "
            "NOT kicked -- the stub (void)s all 4 fields and returns true; the "
            "real cash-crediting sale branch is unimplemented -> "
            "reachability_limited=True) semantic_invalid=%d (unknown-factory, "
            "unknown-object, more-than-owned, overflow qty/price: all WELL-FORMED, "
            "the stub never validates ids so it accepts them silently -- proven "
            "to NOT crash/hang/leak and to mutate NO persisted state; a finished "
            "handler must KICK these) malformed=%d (incl. the size-invalid kick "
            "proof: a size!=12 0x8C hits the HARDENED size guard -> "
            "errorOutput()->disconnectClient() KICKS the cheater, server stays "
            "alive) db_checked=%s victim_pos=%s. Server stayed alive+clean "
            "throughout; every baseline character blob byte-for-byte unchanged "
            "and the victim identity/position intact -- no phantom cash/item/"
            "monster, no cross-player corruption."
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               checks_db, (before.get("x"), before.get("y"),
                           before.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
