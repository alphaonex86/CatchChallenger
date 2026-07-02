"""Framing-level regression test (NOT a single handler): a DYNAMIC packet whose
header + query-number are complete but whose 4-byte dataSize field is TRUNCATED
(1-3 bytes on the wire) followed by silence used to WEDGE the whole server.

Root cause (general/base/ProtocolParsingInput.cpp): parseDataSize() stashes the
1-3 partial size bytes into header_cut and returns 0 ("need more"). Back in the
parseIncommingData() while(1) loop, the header_cut branch computed
    size = readFromSocket(...) + header_cut.size()
which is ALWAYS > 0 while a partial header is stashed. When no more bytes arrive
readFromSocket() returns 0, but size == header_cut.size() > 0, so the loop
re-parses the SAME stashed bytes, re-stashes them, and spins forever — pinning
the single-threaded epoll server at 100% CPU and starving every other client.
Any client could take the entire server down with 4 bytes.

The fix tests the FRESH read (freshRead<=0 -> keep header_cut, return to the
epoll loop) and, on slot reuse, clears header_cut in ProtocolParsingBase::reset()
/ resetForReconnect() so a client that legitimately disconnects mid-header cannot
leave stale prefix bytes for the next tenant of its slot.

Contract checked (all against ONE shared server, exercising slot reuse):
  * WEDGE: send [dyncode][qn][2 of 4 size bytes] and HOLD the socket open. A
    fresh full Session MUST still handshake promptly. If the server wedged, the
    single-threaded loop never services the new connection and A0 times out.
  * EOF variant: same truncated packet, then CLOSE. Server survives; new Session
    handshakes.
  * SLOT-REUSE hygiene: after a mid-header disconnect, open several fresh
    Sessions in a row to force reuse of the just-freed slot. Each MUST handshake
    cleanly (no stale header_cut byte corrupting the new stream).
  * The server stays ALIVE and un-crashed throughout (valgrind-clean; no leak of
    the per-connection header_cut on disconnect).

This is a raw-framing test, so it does NOT use the harness Session for the
attack — it drives bare sockets. 0xAA (addCharacter) is a convenient dynamic
client->server query code; the framer reaches parseDataSize() regardless of
login state, so no handshake is needed to trigger the bug.
"""

import time
import socket
import _protoharness as H

NAME = "00 framing truncated-size (wedge regression)"

_DYNCODE = 0xAA          # any dynamic (0xFE fixed-size marker) client->server code
_HANDSHAKE_BUDGET = 8.0  # a healthy Session handshakes in ~2.5s; wedged -> times out


def _alive_clean(server):
    if not server.alive():
        return False, "server.alive() == False"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % cr[:300]
    return True, ""


def _uniq(prefix):
    return ("%s%x" % (prefix, int(time.time() * 1000) & 0xFFFFFFFF)).encode()


def _fresh_session_ok(server, tag, budget=_HANDSHAKE_BUDGET):
    """Open a full handshake Session; return (ok, seconds, err). A wedged server
    makes this exceed `budget` (A0 gets no reply)."""
    t0 = time.time()
    try:
        s = H.Session(server, login=_uniq(tag), passh=_uniq(tag + b"p"),
                      pseudo="Fr")
        dt = time.time() - t0
        try:
            s.close()
        except Exception:
            pass
        if dt > budget:
            return (False, dt, "handshake took %.1fs > %.1fs budget (server slow/wedged)"
                    % (dt, budget))
        return (True, dt, "")
    except Exception as e:
        return (False, time.time() - t0, "%r" % e)


def _send_truncated(server, hold_open_s):
    """Open a bare socket, send [dyncode][qn][2 of 4 size bytes], optionally hold
    it open for hold_open_s, then return the socket (caller closes)."""
    sk = socket.create_connection(("127.0.0.1", server.port), timeout=5)
    sk.settimeout(0.5)
    # header, query-number, then only 2 of the 4 little-endian size bytes
    sk.sendall(bytes([_DYNCODE, 0x01, 0x02, 0x00]))
    if hold_open_s > 0:
        time.sleep(hold_open_s)
    return sk


def run(server):
    try:
        # baseline: server healthy before any abuse
        ok, dt, err = _fresh_session_ok(server, b"base")
        if not ok:
            return (False, "baseline Session failed (%.1fs): %s" % (dt, err))

        # (1) WEDGE case: truncated dynamic size, socket HELD OPEN. The old bug
        # spun here even without EOF. A fresh Session must still handshake.
        sk1 = _send_truncated(server, hold_open_s=0.5)
        ok, dt, err = _fresh_session_ok(server, b"held")
        if not ok:
            try:
                sk1.close()
            except Exception:
                pass
            return (False, "WEDGE: truncated-size (socket held open) starved the "
                           "event loop; fresh Session failed (%.1fs): %s" % (dt, err))
        try:
            sk1.close()
        except Exception:
            pass
        held_open_ok = True

        aok, why = _alive_clean(server)
        if not aok:
            return (False, "after held-open wedge probe: %s" % why)

        # (2) EOF variant: same truncated packet, then CLOSE immediately.
        sk2 = _send_truncated(server, hold_open_s=0.0)
        try:
            sk2.close()
        except Exception:
            pass
        time.sleep(0.3)  # let the server process the RDHUP / free the slot
        ok, dt, err = _fresh_session_ok(server, b"eof")
        if not ok:
            return (False, "EOF variant: truncated-size then close broke the "
                           "server; fresh Session failed (%.1fs): %s" % (dt, err))
        eof_ok = True

        aok, why = _alive_clean(server)
        if not aok:
            return (False, "after EOF variant: %s" % why)

        # (3) SLOT-REUSE hygiene: a mid-header disconnect must not leave stale
        # header_cut bytes for the next tenant of the freed slot. Fire several
        # truncated-then-close attacks each followed by a full Session, forcing
        # the slot to cycle; every Session must handshake cleanly.
        reuse_rounds = 0
        i = 0
        while i < 4:
            skN = _send_truncated(server, hold_open_s=0.0)
            try:
                skN.close()
            except Exception:
                pass
            time.sleep(0.2)
            ok, dt, err = _fresh_session_ok(server, b"reuse%d" % i)
            if not ok:
                return (False, "SLOT-REUSE round %d: fresh Session after a "
                               "mid-header disconnect failed (%.1fs): %s — stale "
                               "header_cut likely corrupted the reused slot"
                               % (i, dt, err))
            reuse_rounds += 1
            i += 1

        aok, why = _alive_clean(server)
        if not aok:
            return (False, "after slot-reuse rounds: %s" % why)

        return (True, "no-wedge (held-open=%s, eof=%s), slot-reuse clean x%d; "
                      "server alive+clean throughout (a 4-byte truncated dynamic "
                      "packet no longer starves the single-threaded loop)"
                % (held_open_ok, eof_ok, reuse_rounds))
    except Exception as e:
        return (False, "unexpected exception: %r" % e)
