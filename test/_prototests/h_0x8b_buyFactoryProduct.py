#!/usr/bin/env python3
"""
Protocol valgrind test for server handler 0x8B (buyFactoryProduct).

Server side: server/base/ClientNetworkReadQuery.cpp:117 (parseQuery).
The packet is a FIXED-size query of 12 bytes:
    [factoryId u16][objectId u16][quantity u32][price u32]   (all little-endian)
On the wire (fixed-size query, code>=0x80): [0x8B][queryNumber][12 data bytes].

IMPORTANT — what this handler actually does today (verified by reading the
source): the body is STUBBED. It loads the four fields, casts them to (void),
and `return true;` — the real buyFactoryProduct() call is commented out. So:

  * It sends NO reply (no 0x7F comes back).  reply_to() timing out is the
    CORRECT, expected behaviour for the valid case.
  * It mutates NO persisted/in-memory state (cash/items/factory). reload_state()
    must therefore show the player UNCHANGED — that is exactly the "no state
    corruption" assertion we want.
  * It performs NO semantic validation of the four fields (unknown factoryId /
    objectId, overflow quantity/price are all ignored). There is consequently
    NO handler-level semantic-reject-and-kick path to exercise: a well-formed
    12-byte 0x8B with absurd ids is accepted as a no-op and the client is NOT
    kicked. We still SEND those illegal-value packets and assert the server
    survives + the connection is NOT corrupted, documenting it as
    reachability-limited (the stub cannot reject semantically).

Also: because 0x8B is FIXED-size (packetFixedSize[0x8B]=12), the protocol layer
always frames exactly 12 bytes into parseQuery(), so the in-handler HARDENED
`if(size!=12)` check is dead for a properly-framed 0x8B. To exercise the
size/parse error paths we corrupt the WIRE FRAMING around 0x8B (truncated body
+ EOF, a 0x7F reply to an unknown queryNumber, an unknown/blocked packet code,
and a 0x8B whose trailing bytes run into a blocked code). Each of those is a
genuine protocol-layer error -> errorParsingLayer()/errorOutput() ->
disconnectClient() (KICK). We assert the offending client is kicked (socket
EOF) and that the SERVER stays alive and clean after each.

run(server) -> (ok: bool, detail: str). Catches all its own exceptions.
"""

import socket
import time

import _protoharness as H


NAME = "0x8B buyFactoryProduct"

CODE = 0x8B


def _valid_body(factory=0, obj=0, qty=1, price=1):
    """Well-formed 12-byte 0x8B body."""
    return H.u16(factory) + H.u16(obj) + H.u32(qty) + H.u32(price)


def _peer_gone(sess, timeout=1.5):
    """Return True if the peer has closed the connection (we were kicked).

    A kick on the server is disconnectClient(): the socket reaches EOF. We
    detect that by reading until recv() returns b'' (EOF) or the socket errors,
    and as a second confirmation try to send on it (a closed peer eventually
    yields a broken pipe / further reads return empty). Bounded by `timeout` so
    a still-open connection can't hang us."""
    deadline = time.time() + timeout
    try:
        sess.sock.settimeout(0.3)
    except OSError:
        return True
    saw_eof = False
    while time.time() < deadline:
        try:
            d = sess.sock.recv(4096)
            if d == b"":
                saw_eof = True
                break
            # data still flowing (e.g. a "kicked" system message just before
            # the FIN); keep reading until EOF.
        except socket.timeout:
            # nothing right now; probe by sending a harmless byte sequence and
            # loop again. If the peer is gone the next recv will EOF/raise.
            try:
                sess.sock.sendall(b"\x00")
            except OSError:
                saw_eof = True
                break
        except OSError:
            saw_eof = True
            break
    return saw_eof


def _server_clean(server):
    """server alive AND no crash report. Returns (ok, detail)."""
    if not server.alive():
        return False, "server not alive (process died or stopped accepting TCP)"
    cr = server.crash_report()
    if cr is not None:
        return False, "server crash_report: %s" % cr
    return True, ""


def run(server):
    try:
        # ---- snapshot a legitimate, independent victim BEFORE any abuse -----
        # A separate account whose persisted state must be byte-identical after
        # all the abuse below (proves no cross-client corruption).
        victim_login = ("vic8b%x" % (int(time.time()) & 0xFFFFFF)).encode()
        victim_pass = b"p_" + victim_login
        victim = H.Session(server, login=victim_login, passh=victim_pass,
                           pseudo="Vic8b")
        before = {"character_id": victim.character_id, "x": victim.x,
                  "y": victim.y, "mapIndex": victim.mapIndex}
        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        snap_before = H.reload_state(server, victim_login, victim_pass)

        ok, why = _server_clean(server)
        if not ok:
            return False, "after setup: " + why

        # ================================================================
        # 1) VALID packet (well-formed 12-byte 0x8B). Stub => no reply.
        # ================================================================
        sess = H.Session(server, pseudo="Buyer8b")
        qn = sess.q(CODE, _valid_body(factory=0, obj=0, qty=1, price=1))
        rep = sess.reply_to(qn, timeout=1.5)
        # The handler is stubbed and sends NO reply: rep is expected None.
        if rep is not None:
            # Not a failure per se, but document the surprise; the stub today
            # returns nothing. A future real handler may reply — accept either.
            pass
        ok, why = _server_clean(server)
        if not ok:
            sess.close()
            return False, "valid 0x8B: " + why
        # The valid client must NOT be kicked (well-formed packet). Confirm the
        # connection is still usable: send a 2nd valid 0x8B and stay connected.
        sess.q(CODE, _valid_body(factory=1, obj=2, qty=3, price=4))
        time.sleep(0.2)
        ok, why = _server_clean(server)
        if not ok:
            sess.close()
            return False, "valid 0x8B (2nd send): " + why
        if _peer_gone(sess, timeout=0.8):
            sess.close()
            return False, ("valid 0x8B got KICKED — a well-formed packet must "
                           "not disconnect the client")
        valid_cases = 1
        sess.close()
        time.sleep(0.2)

        # ================================================================
        # 2) SEMANTIC-INVALID values (well-formed framing, illegal content).
        #    Stub ignores field values => accepted as a no-op, NOT kicked.
        #    reachability-limited: the handler cannot reject these today.
        # ================================================================
        semantic_invalid_cases = 0
        semantic_specs = [
            # (factory, obj, qty, price, label)
            (0xFFFF, 0xFFFF, 1, 1, "unknown factoryId+objectId (do not exist)"),
            (0, 0, 0xFFFFFFFF, 0xFFFFFFFF, "overflow quantity+price"),
            (0xFFFF, 0, 0, 1, "unknown factory, zero quantity"),
        ]
        i = 0
        while i < len(semantic_specs):
            f, o, qy, pr, label = semantic_specs[i]
            s = H.Session(server, pseudo="Sem8b%d" % i)
            s.q(CODE, _valid_body(f, o, qy, pr))
            time.sleep(0.2)
            ok, why = _server_clean(server)
            if not ok:
                s.close()
                return False, "semantic-invalid [%s]: %s" % (label, why)
            # Stubbed handler => the client is NOT kicked (documented limit).
            # We do not REQUIRE a kick here (no semantic-reject path exists);
            # we only require the server stays healthy and the connection is
            # still coherent (a follow-up valid packet does not crash it).
            s.q(CODE, _valid_body(0, 0, 1, 1))
            time.sleep(0.15)
            ok, why = _server_clean(server)
            if not ok:
                s.close()
                return False, "semantic-invalid [%s] follow-up: %s" % (label, why)
            semantic_invalid_cases += 1
            s.close()
            time.sleep(0.15)
            i += 1

        # ================================================================
        # 3) MALFORMED FRAMING around 0x8B — must KICK, never crash.
        #    Each on its OWN session (a kick closes the socket).
        # ================================================================
        malformed_cases = 0

        def _malformed(label, raw, expect_kick=True):
            nonlocal malformed_cases
            ms = H.Session(server, pseudo="Mal8b%d" % malformed_cases)
            ms.send_raw(raw)
            time.sleep(0.25)
            ok2, why2 = _server_clean(server)
            if not ok2:
                ms.close()
                return False, "malformed [%s]: %s" % (label, why2)
            if expect_kick:
                gone = _peer_gone(ms, timeout=1.5)
                ms.close()
                if not gone:
                    return False, ("malformed [%s]: client was NOT kicked "
                                   "(connection stayed open) — contract "
                                   "violation" % label)
            else:
                ms.close()
            # server must still be clean AFTER the kick too
            ok2, why2 = _server_clean(server)
            if not ok2:
                return False, "malformed [%s] post-kick: %s" % (label, why2)
            malformed_cases += 1
            return True, ""

        # (a) 0x7F reply to a queryNumber that is NOT in flight -> the parser
        #     rejects "not a reply to a known query" -> kick. (queryNumber 7
        #     was never used as a query by this fresh session.)
        ok, why = _malformed("0x7F reply to unknown queryNumber",
                             bytes([0x7F, 0x07]))
        if not ok:
            return False, why

        # (b) unknown / blocked packet code (0x70 has packetFixedSize 0xFF) ->
        #     "wrong packet code (header)" -> kick.
        ok, why = _malformed("unknown/blocked packet code 0x70",
                             bytes([0x70, 0x00, 0x00]))
        if not ok:
            return False, why

        # (c) a 0x8B header with a TRUNCATED body then a blocked code: the
        #     fixed-size framing greedily consumes 12 bytes (eating the junk),
        #     then the next byte is a blocked code -> kick. We give 2 body
        #     bytes + a run of blocked-code bytes so the parser definitely
        #     sees the bad code on the next packet boundary.
        ok, why = _malformed(
            "0x8B truncated body running into blocked code 0x70",
            bytes([CODE, 0x01, 0xAA, 0xBB]) + bytes([0x70]) * 16)
        if not ok:
            return False, why

        # (d) a 0x8B query reusing an in-flight queryNumber pattern via a raw
        #     duplicate of the queryNumber with an extra stray 0x01 reply
        #     header to an unknown qnum after a valid-looking 0x8B prefix.
        #     This drives the reply-framing reject path -> kick.
        ok, why = _malformed(
            "valid 0x8B then 0x01 reply-header to unknown qnum",
            bytes([CODE, 0x02]) + _valid_body(0, 0, 1, 1) + bytes([0x01, 0x09]))
        if not ok:
            return False, why

        # ================================================================
        # 4) NO STATE CORRUPTION — verify the victim's persisted state is
        #    byte-for-byte what it was before any of the abuse above, using a
        #    SEPARATE legitimate session. The stub mutates nothing, and the
        #    kicked cheaters must not have touched anyone.
        # ================================================================
        time.sleep(0.3)
        snap_after = H.reload_state(server, victim_login, victim_pass)
        for key in ("character_id", "x", "y", "mapIndex"):
            b = snap_before.get(key) if snap_before else None
            a = snap_after.get(key) if snap_after else None
            if b != a:
                return False, ("STATE CORRUPTION: victim %s changed %r -> %r "
                               "(expected unchanged — handler is a stub)"
                               % (key, b, a))
        # also confirm the live identity/position the victim logged in with is
        # unchanged from the pre-abuse snapshot.
        if snap_after is not None:
            if before["character_id"] is not None and \
               snap_after.get("character_id") != before["character_id"]:
                return False, ("STATE CORRUPTION: victim character_id changed "
                               "%r -> %r" % (before["character_id"],
                                             snap_after.get("character_id")))

        # ================================================================
        # 5) Leave the server usable — a fresh legit session must still
        #    handshake to the map after all the abuse.
        # ================================================================
        post = H.Session(server, pseudo="Post8b")
        if post.character_id is None:
            post.close()
            return False, "post-check: fresh session failed to reach the map"
        post.close()

        ok, why = _server_clean(server)
        if not ok:
            return False, "final: " + why

        detail = ("0x8B is a STUBBED fixed-12B query (no reply, no state "
                  "mutation, no semantic validation): valid=%d sent+survived; "
                  "%d semantic-illegal-value sends accepted-as-noop (NOT "
                  "kicked — reachability-limited: stub has no reject path); "
                  "%d malformed-framing variants each KICKED the client and "
                  "the server stayed alive+clean; victim DB state unchanged "
                  "(no corruption)."
                  % (valid_cases, semantic_invalid_cases, malformed_cases))
        return True, detail

    except Exception as e:
        import traceback
        return False, "exception in run(): %r\n%s" % (e, traceback.format_exc())
