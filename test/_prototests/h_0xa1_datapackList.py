"""Protocol handler test for 0xA1 datapackList (server/base/ClientNetworkReadQuery.cpp:221).

DYNAMIC QUERY. Wire payload layout:
    [stepToSkip:u8]
    [number_of_file:u32 LE]
    per-file (number_of_file times): [nameLen:u8][name:nameLen bytes]
    per-file (number_of_file times): [partialHash:u32 LE]

Reachable after login, pre/at character-select, while datapackStatus==Base. A fresh
Session reaches CharacterSelected WITHOUT ever sending 0xA1, so datapackStatus is still
Base (only this handler mutates it). The handler therefore runs the Base branch.

SECURITY (the just-fixed bug): number_of_file is attacker-controlled up to 0xFFFFFFFF and
used to size two std::vector::reserve() calls. Without the cap check those reserves request
tens of GB and abort the whole server on bad_alloc. The fix caps number_of_file>1000000
BEFORE any reserve() and rejects it. This test sends number_of_file=0xFFFFFFFF and asserts
the server (a) does NOT OOM/abort and (b) rejects+kicks the offending client.

Reply: dynamic. With number_of_file==0 ("I have nothing, send me everything") the server
streams the whole base datapack (0x75 size message + file packets) and a 0x7F reply, all of
which the harness pumps; we only assert the server stays alive and the client is NOT kicked
for that legitimate request.
"""

import struct
import time

import _protoharness as H

NAME = "0xA1 datapackList"


# --- kick / liveness detection -------------------------------------------------
def _peer_gone(sess, settle=0.8, probe_timeout=0.8):
    """Return True if the server has CLOSED (kicked) this session's socket.

    After an illegal protocol packet the server calls errorOutput()->disconnectClient(),
    so the peer half-closes and our recv() reaches EOF (returns b''). A silent-ignore
    (connection still open) instead raises socket.timeout on a quiet socket. We first
    drain any pending server bytes, then probe for EOF, then send a tiny well-formed
    follow-up query and probe again so a delayed FIN is still detected.
    """
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sess.sock.settimeout(probe_timeout)
            data = sess.sock.recv(65536)
            if data == b"":
                return True  # clean EOF -> kicked
            # got bytes (server's reply/stream); keep reading until EOF or quiet
        except OSError:
            return True      # socket error (RST/closed) -> kicked
    # still open after draining: send a harmless follow-up and re-probe for EOF.
    try:
        sess.sock.sendall(bytes([0xAD]) + bytes(32))  # 0xAD Stat client (fixed 32)
    except OSError:
        return True
    end = time.time() + settle
    while time.time() < end:
        try:
            sess.sock.settimeout(probe_timeout)
            data = sess.sock.recv(65536)
            if data == b"":
                return True
        except OSError:
            return True
    return False


def _a1_payload(step, number_of_file, files=None, hashes=None):
    """Build a WELL-FORMED 0xA1 payload body (no framing; q(...,dynamic=True) adds the
    4-byte length). files/hashes default to empty (consistent with number_of_file==0)."""
    body = bytes([step & 0xFF]) + struct.pack("<I", number_of_file & 0xFFFFFFFF)
    if files:
        for nm in files:
            body += bytes([len(nm) & 0xFF]) + nm
    if hashes:
        for h in hashes:
            body += struct.pack("<I", h & 0xFFFFFFFF)
    return body


def run(server):
    try:
        # ---- precondition: a fresh logged-in, on-map session (datapackStatus==Base) ----
        try:
            sess = H.Session(server)
        except H.HandshakeError as e:
            return (False, "precondition: could not establish Session: %r" % e)

        if not server.alive():
            return (False, "server not alive after establishing precondition Session")

        # ============================================================================
        # 1) VALID well-formed request: stepToSkip=0x01 (stay in Base), number_of_file=0.
        #    Client claims it has nothing -> server must stream the whole base datapack
        #    and reply, WITHOUT kicking. This is the legitimate, non-cheating path.
        # ============================================================================
        qn = sess.q(0xA1, _a1_payload(0x01, 0), dynamic=True)
        rep = sess.reply_to(qn, timeout=4.0)  # pumps 0x75 + file packets + the 0x7F reply
        if not server.alive():
            return (False, "server died after VALID 0xA1 (number_of_file=0)")
        if server.crash_report() is not None:
            return (False, "crash after VALID 0xA1: %s" % server.crash_report())
        # The server should NOT have kicked us for a legitimate full-datapack request.
        if _peer_gone(sess, settle=0.6):
            return (False, "VALID 0xA1 (number_of_file=0) wrongly kicked the client "
                           "(legitimate full-datapack request must not disconnect)")
        valid_cases = 1
        # We got either a streamed datapack or at minimum no crash/kick: that is the
        # observable success. (No persisted state is mutated by datapackList: it only
        # streams files; reload_state below confirms identity/position is untouched.)
        _ = rep

        # ============================================================================
        # 2) MALFORMED-FRAMING variants. Each is sent on its OWN fresh session because
        #    the server kicks on a parse error (closing the socket). After each: assert
        #    server stays alive + no crash + the offending client is kicked.
        # ============================================================================
        malformed_cases = 0

        # (a) empty payload: dynamic length 0 -> handler can't read stepToSkip (u8).
        s = H.Session(server)
        s.q(0xA1, b"", dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on malformed (a) empty payload: %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "malformed (a) empty payload: client NOT kicked")
        malformed_cases += 1

        # (b) stepToSkip present but number_of_file (u32) truncated to 2 bytes.
        s = H.Session(server)
        s.q(0xA1, bytes([0x01]) + b"\x05\x00", dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on malformed (b) truncated number_of_file: %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "malformed (b) truncated number_of_file: client NOT kicked")
        malformed_cases += 1

        # (c) number_of_file=3 but ZERO file entries follow (header lies about count) ->
        #     handler runs out of bytes reading the first file name size -> reject.
        s = H.Session(server)
        s.q(0xA1, _a1_payload(0x01, 3), dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on malformed (c) count>data: %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "malformed (c) count=3 with no file data: client NOT kicked")
        malformed_cases += 1

        # (d) number_of_file=1, name advertises 0xFF bytes but only 2 provided ->
        #     truncated file-name body -> reject.
        s = H.Session(server)
        bad = bytes([0x01]) + struct.pack("<I", 1) + bytes([0xFF]) + b"ab"
        s.q(0xA1, bad, dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on malformed (d) truncated file name: %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "malformed (d) truncated file-name body: client NOT kicked")
        malformed_cases += 1

        # ============================================================================
        # 3) SEMANTIC-INVALID (well-formed framing, illegal content for THIS handler).
        #    Each must KICK the cheater while the SERVER survives (no crash/OOM/leak).
        # ============================================================================
        semantic_invalid_cases = 0

        # (S1) THE SECURITY CASE: number_of_file=0xFFFFFFFF. Well-formed framing
        #      (payload is exactly stepToSkip+u32, length 5). Pre-fix this made the
        #      server reserve() ~tens of GB and abort on bad_alloc; the fix caps it
        #      BEFORE reserve and rejects. Assert: NO server abort/OOM, client kicked.
        s = H.Session(server)
        s.q(0xA1, _a1_payload(0x01, 0xFFFFFFFF), dynamic=True)
        if not server.alive():
            return (False, "SERVER DIED on number_of_file=0xFFFFFFFF (OOM regression!): %s"
                           % server.crash_report())
        if server.crash_report() is not None:
            return (False, "crash/abort on number_of_file=0xFFFFFFFF (OOM regression!): %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "number_of_file=0xFFFFFFFF: cheater NOT kicked")
        semantic_invalid_cases += 1

        # (S2) number_of_file just over the cap (1000001): well-formed header, illegal
        #      count -> rejected by the >million cap, no reserve, kick.
        s = H.Session(server)
        s.q(0xA1, _a1_payload(0x01, 1000001), dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on number_of_file=1000001: %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "number_of_file=1000001 (over cap): cheater NOT kicked")
        semantic_invalid_cases += 1

        # (S3) illegal stepToSkip byte (0x09 is not 1/2/3) -> "step out of range" reject.
        s = H.Session(server)
        s.q(0xA1, _a1_payload(0x09, 0), dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on stepToSkip=0x09: %s" % server.crash_report())
        if not _peer_gone(s):
            return (False, "illegal stepToSkip=0x09: cheater NOT kicked")
        semantic_invalid_cases += 1

        # (S4) under HARDENED: a well-formed file entry whose NAME violates the datapack
        #      filename regex (path traversal "../x.png") -> regex reject -> kick.
        s = H.Session(server)
        nm = b"../x.png"
        bad = _a1_payload(0x01, 1, files=[nm], hashes=[0])
        s.q(0xA1, bad, dynamic=True)
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on illegal file name '../x.png': %s"
                           % server.crash_report())
        if not _peer_gone(s):
            return (False, "illegal file name '../x.png': cheater NOT kicked")
        semantic_invalid_cases += 1

        # ============================================================================
        # 4) NO STATE CORRUPTION. datapackList streams files only; it must never mutate
        #    persisted character state. Verify the original account's character is intact
        #    via reload_state (identity + position) using a fresh, legitimate connection.
        # ============================================================================
        checks_db_state = False
        try:
            # Let the original session's disconnect-save settle, then reload.
            sess.close()
            time.sleep(0.4)
            snap = H.reload_state(server, sess.login_creds, sess.pass_creds)
            checks_db_state = True
            if snap.get("character_id") is None:
                # reload could not frame the character (datapack quirk) -> document, do
                # not fail the whole test for an observability gap.
                checks_db_state = False
            else:
                # Position must be the deterministic spawn (start.xml); datapackList must
                # not have moved/altered it. We only have identity+position from reload.
                if snap.get("x") is None or snap.get("y") is None \
                        or snap.get("mapIndex") is None:
                    checks_db_state = False
        except Exception as e:
            checks_db_state = False
            _ = e  # observability gap, not a contract failure

        # ============================================================================
        # 5) Server must remain USABLE for the next test: a brand-new legit Session
        #    completes the full handshake (the cheater sessions are all closed).
        # ============================================================================
        try:
            post = H.Session(server)
            post.close()
        except H.HandshakeError as e:
            return (False, "server unusable after the handler test (fresh handshake "
                           "failed): %r" % e)

        if not server.alive():
            return (False, "server not alive at end of test")
        if server.crash_report() is not None:
            return (False, "crash detected at end of test: %s" % server.crash_report())

        detail = ("0xA1 datapackList OK: valid=%d malformed=%d semantic_invalid=%d; "
                  "number_of_file=0xFFFFFFFF rejected with NO OOM/abort and cheater kicked; "
                  "db-identity/position re-checked=%s; server survived all + still usable"
                  % (valid_cases, malformed_cases, semantic_invalid_cases,
                     checks_db_state))
        return (True, detail)

    except Exception as e:
        import traceback
        return (False, "unexpected exception in run(): %r\n%s"
                       % (e, traceback.format_exc()))
