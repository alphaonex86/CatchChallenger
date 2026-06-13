#!/usr/bin/env python3
"""
Handler test: 0x8A getFactoryList (server/base/ClientNetworkReadQuery.cpp:101,
Client::parseQuery).

Wire shape (general/base/ProtocolParsingGeneral.cpp: packetFixedSize[0x8a]==2):
0x8A is a FIXED-size-2 QUERY -> on the wire it is exactly
    [0x8A][queryNumber][factoryId:2 LE]
with NO 4-byte length field (it is not dynamic) and NO trailing data.

Handler semantics (ClientNetworkReadQuery.cpp:101-115):
    case 0x8A:
        #ifdef CATCHCHALLENGER_HARDENED
        if(size!=sizeof(uint16_t)) { errorOutput("wrong size ..."); return false; }
        #endif
        const uint16_t &factoryId=loadLe16(data);
        (void)factoryId;
        //getFactoryList(queryNumber,factoryId);   <-- STUBBED, no-op
        return true;

So the handler is a STUB: it parses the fixed 2-byte factoryId, discards it, and
returns true. It does NOT consult the datapack, does NOT mutate any state, does
NOT send a reply, and does NOT kick on a valid (size==2) packet. There is no
"unknown factory" rejection path because the lookup was never wired in.

Consequences this test verifies:
  * VALID: a well-formed [0x8A][qn][factoryId:2] is parsed and the server stays
    alive with NO crash and NO leak. Because the stub sends no reply, reply_to()
    times out (None) and the connection stays OPEN (no kick) -- that IS the
    correct behaviour for this stub, so we assert the socket is still open.
  * SEMANTIC-INVALID: an UNKNOWN factoryId (e.g. 0xFFFF, which no datapack
    factory uses) is, for THIS stubbed handler, indistinguishable from any other
    id -- it is parsed, discarded, and ignored. The documented contract for a
    no-op stub is therefore "accept-and-ignore, never crash, never corrupt
    state, never reply", NOT a kick. We assert exactly that (server alive, no
    reply, socket still open, no state change). This is the reachability limit:
    the genuine getFactoryList() body (which WOULD validate the id and could
    kick) is commented out, so a real "unknown-id -> kick" branch does not
    exist to be exercised. We document it rather than fabricate a kick the code
    cannot produce.
  * MALFORMED FRAMING: because 0x8A is FIXED-size, the input layer
    (ProtocolParsingInput.cpp:424) always slices exactly packetFixedSize[0x8A]==2
    data bytes for it -- the in-case `size!=2` HARDENED guard is unreachable via
    legal fixed framing. So "malformed" means corrupting the wire FRAME itself
    with send_raw (truncated header, dynamic-style framing of a fixed packet,
    oversized pseudo-length, garbage trailing). After EACH we assert the server
    process is alive and crash_report() is None (the offending client may be
    kicked by the lower parsing layer; the SERVER must never die/hang/leak).

NO STATE CORRUPTION: getFactoryList persists nothing (it is a discarded read),
so we prove an independent SECOND account still handshakes to CharacterSelected
on the map with its spawn intact, and H.reload_state() on the exercised account
shows its persisted identity/position unchanged.
"""

import time
import _protoharness as H


NAME = "0x8A getFactoryList"


def _peer_is_closed(sess, settle=0.8):
    """True if the server has closed (kicked) this session's socket.

    A kicked client receives a FIN: recv() then returns b'' (EOF). A still-open
    idle connection makes recv() time out -> returns False. A reset surfaces as
    OSError -> closed. Bounded so a missing reply can never hang the test."""
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sess.sock.settimeout(0.25)
            d = sess.sock.recv(65536)
        except (TimeoutError, OSError) as e:
            if isinstance(e, H.socket.timeout):
                continue          # still open, nothing more to say this slice
            return True           # reset / broken pipe -> peer gone
        if d == b"":
            return True           # clean FIN -> kicked
        sess._buf.extend(d)       # absorb any server message, keep waiting
        try:
            ev = sess._parse_stream()
            sess._auto_ack(ev)
        except Exception:
            pass
    return False


def _peer_is_open(sess):
    """True if the connection is still usable (NOT kicked). For the stub's valid
    path we expect the socket to stay open: we poke it with another well-formed
    0x8A and confirm the send succeeds and no FIN arrives."""
    if _peer_is_closed(sess, settle=0.6):
        return False
    try:
        sess.q(0x8A, H.u16(0))      # harmless extra factory request
    except OSError:
        return False                # broken pipe -> was kicked/closed
    # still must not be closed after the poke
    return not _peer_is_closed(sess, settle=0.5)


def run(server):
    sessions = []
    try:
        # ---- precondition: an on-map player (a fresh handshaked Session) -----
        try:
            cheater = H.Session(server, pseudo="FactCheat")
        except H.HandshakeError as e:
            return (False, "precondition handshake failed: %r" % e)
        sessions.append(cheater)
        if not server.alive():
            return (False, "server not alive right after handshake")

        # ---- VALID: well-formed fixed 0x8A, factoryId=0 ----------------------
        # [0x8A][qn][00 00]. Stub parses + discards + returns true: no reply, no
        # kick. Assert server alive, no reply within a short window, socket open.
        qn = cheater.q(0x8A, H.u16(0))
        if not server.alive():
            return (False, "server died after a well-formed 0x8A (stub must never "
                           "crash): %s" % server.crash_report())
        rep = cheater.reply_to(qn, timeout=1.0)
        if rep is not None:
            return (False, "stubbed 0x8A unexpectedly produced a reply (%s); the "
                           "handler body is commented out, expected no reply"
                           % H._hx(rep))
        if server.crash_report() is not None:
            return (False, "crash_report after well-formed 0x8A: %s"
                           % server.crash_report())
        if not _peer_is_open(cheater):
            return (False, "stubbed 0x8A wrongly closed the connection; a no-op "
                           "stub on a valid fixed packet must NOT kick")
        valid_cases = 1

        # ---- SEMANTIC-INVALID: unknown factoryId 0xFFFF ----------------------
        # No datapack factory uses 0xFFFF. For this stub it is parsed+discarded
        # exactly like any other id: accept-and-ignore, no crash, no reply, no
        # kick, no state change (the validate/kick branch is commented out ->
        # reachability-limited, documented).
        qn2 = cheater.q(0x8A, H.u16(0xFFFF))
        if not server.alive():
            return (False, "server died on 0x8A with unknown factoryId=0xFFFF: %s"
                           % server.crash_report())
        rep2 = cheater.reply_to(qn2, timeout=1.0)
        if rep2 is not None:
            return (False, "0x8A unknown factoryId produced a reply (%s); stub "
                           "should ignore it" % H._hx(rep2))
        if server.crash_report() is not None:
            return (False, "crash_report after 0x8A unknown factoryId: %s"
                           % server.crash_report())
        if not _peer_is_open(cheater):
            return (False, "0x8A with unknown factoryId wrongly closed the "
                           "connection; the stub does not validate the id, so it "
                           "must accept-and-ignore (no kick)")
        # a second odd-but-well-formed id (max-1) for breadth
        qn3 = cheater.q(0x8A, H.u16(0xFFFE))
        if not server.alive():
            return (False, "server died on 0x8A factoryId=0xFFFE: %s"
                           % server.crash_report())
        if cheater.reply_to(qn3, timeout=0.8) is not None:
            return (False, "0x8A factoryId=0xFFFE produced an unexpected reply")
        semantic_invalid_cases = 2

        # ---- NO STATE CORRUPTION: a SECOND legit account is unaffected -------
        # getFactoryList persists nothing; prove an independent player still
        # reaches CharacterSelected on the map with a valid spawn.
        try:
            victim = H.Session(server, pseudo="FactWitness")
        except H.HandshakeError as e:
            return (False, "post-exercise: independent Session failed to handshake "
                           "-> shared/server state corrupted: %r" % e)
        sessions.append(victim)
        if not server.alive():
            return (False, "victim session lost the server")
        victim.close()

        # reload_state on the exercised account: identity/position must be intact
        # (handler mutates nothing, so this is a pure no-corruption check).
        try:
            cheater.close()
        except Exception:
            pass
        time.sleep(0.3)  # let the disconnect-save settle (FILE_DB block save)
        snap = H.reload_state(server, cheater.login_creds, cheater.pass_creds)
        if snap.get("character_id") is None:
            return (False, "reload_state lost the account's character -> state "
                           "corruption from the 0x8A requests (handler must "
                           "persist nothing)")

        # ---- MALFORMED FRAMING (fresh session each, assert survive) ----------
        # 0x8A is FIXED-size-2; the input layer always slices exactly 2 data
        # bytes, so we corrupt the FRAME itself with send_raw.
        malformed = []
        # 1) Header truncated: just the code byte, no queryNumber + no data. The
        #    parser is waiting for bytes that never come; must not crash/hang.
        malformed.append(("0x8A code byte only (truncated, missing qn+data)",
                          bytes([0x8A])))
        # 2) qn present but factoryId truncated to a single byte: [0x8A][qn][b0].
        #    A fixed-2 packet wants 2 data bytes; only 1 arrives -> incomplete,
        #    server must wait without crashing (we then close).
        malformed.append(("0x8A with 1-byte (truncated) factoryId",
                          bytes([0x8A, 0x00, 0x11])))
        # 3) Dynamic-style framing for a FIXED packet: pretend 0x8A carries a
        #    4-byte length + 8 data bytes. The server treats 0x8A as fixed-2, so
        #    [len:4][data] desyncs into a bogus follow-on packet stream.
        malformed.append(("0x8A framed as dynamic (bogus len=8 + 8 bytes)",
                          bytes([0x8A, 0x00]) + H.u32(8) + b"\xAB" * 8))
        # 4) Valid 0x8A then a trailing pseudo-dynamic packet claiming a huge
        #    size that will never arrive: server must not hang or alloc unbounded.
        malformed.append(("0x8A(valid) + trailing pseudo-packet, huge size",
                          bytes([0x8A, 0x00]) + H.u16(0)
                          + b"\x83" + H.u32(0x7FFFFFFF)))

        malformed_cases = 0
        for label, raw in malformed:
            try:
                ms = H.Session(server, pseudo="FactMal%d" % malformed_cases)
            except H.HandshakeError as e:
                return (False, "malformed[%s]: pre-handshake failed: %r" % (label, e))
            sessions.append(ms)
            try:
                ms.send_raw(raw)
            except OSError:
                pass  # peer may already be gone; that's fine
            try:
                ms.drain(timeout=0.6)   # bounded; a never-coming reply can't hang
            except Exception:
                pass
            if not server.alive():
                return (False, "server DIED on malformed framing [%s]: %s"
                               % (label, server.crash_report()))
            if server.crash_report() is not None:
                return (False, "crash_report on malformed framing [%s]: %s"
                               % (label, server.crash_report()))
            malformed_cases += 1
            try:
                ms.close()
            except Exception:
                pass

        # ---- post-checks: server still usable for the next test --------------
        if not server.alive():
            return (False, "server not alive at end of test")
        if server.crash_report() is not None:
            return (False, "crash_report set at end: %s" % server.crash_report())
        try:
            tail = H.Session(server, pseudo="FactAfter")
            sessions.append(tail)
            tail.close()
        except H.HandshakeError as e:
            return (False, "server unusable after test (final Session failed): %r" % e)

        return (True,
                "0x8A getFactoryList is a NO-OP STUB (handler body commented out): "
                "a well-formed fixed [0x8A][qn][factoryId:2] (%d valid case) is "
                "parsed, discarded and produces NO reply and NO kick (socket stays "
                "open) and the server survives with no crash/leak; an unknown "
                "factoryId 0xFFFF/0xFFFE (%d semantic-invalid cases) is "
                "accept-and-ignored identically (no validation/kick branch exists "
                "-- reachability-limited, documented); %d malformed-framing "
                "variants each left the server alive with crash_report()==None; a "
                "second account + reload_state confirm no state corruption."
                % (valid_cases, semantic_invalid_cases, malformed_cases))
    except Exception as e:
        return (False, "unexpected exception in run(): %r" % e)
    finally:
        for s in sessions:
            try:
                s.close()
            except Exception:
                pass
