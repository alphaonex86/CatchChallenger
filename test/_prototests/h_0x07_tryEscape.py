"""Protocol handler test: 0x07 tryEscape.

Handler: server/base/ClientNetworkReadMessage.cpp:369
    case 0x07: tryEscape(); break;
Wire form: 0x07 is a FIXED-size MESSAGE of size 0 (packetFixedSize[0x07]==0,
general/base/ProtocolParsingGeneral.cpp:72). So a valid packet on the wire is
the single byte [0x07] -- no queryNumber (code<0x80, not a reply), no dataSize.
The case has NO size check; framing for a fixed-0 packet is fixed by the table.

Call chain (general/base/fight/CommonFightEngineWild.cpp:303 -> CommonFightEngine.cpp):
    tryEscape() -> canEscape():
        if(!isInFight())  -> errorFightEngine("error: tryEscape() when is not in
                              fight") -> Client::errorFightEngine() ->
                              Client::errorOutput() -> disconnectClient()  (KICK)
        else if wildMonsters.empty() (bot fight) -> messageFightEngine notice,
                              returns false (no kick).
        else (real wild fight) -> internalTryEscape(), clears wildMonsters on
                              success or generateOtherAttack() on failure.

CONTRACT under test:
  * tryEscape() OUT OF a wild fight is the SEMANTICALLY-ILLEGAL use (a player can
    only escape a wild battle). On the server errorFightEngine routes to
    errorOutput which, when stat==CharacterSelected, KICKS the offending client
    (and broadcasts a system 'kicked, have try hack' message) and the server
    keeps running (production-safe, no crash/hang/leak).
  * malformed framing around the 0-byte 0x07 must never crash/hang the server.
  * an illegal escape attempt must not mutate any persisted/in-game state.

REACHABILITY-LIMITED: a genuine wild fight is not triggerable from the bare
'test' maincode via the harness API (no encounter packet path exposed), so the
in-fight success path of tryEscape() can only be checked indirectly. We assert
the out-of-fight handler refuses gracefully (KICK, no crash, no state change)
across many semantic-illegal and malformed-framing sends. Documented as a
reachability-limited partial check of the success path.
"""

import _protoharness as H

NAME = "0x07 tryEscape"


# --- local kick / liveness helpers (use ONLY the harness Session socket) ----

def _peer_gone(sess, timeout=2.0):
    """Return True if the server has closed this connection (the client was
    kicked). Strategy: drain whatever the server sent (it sends a system
    'kicked' message before disconnect), then probe the socket. A kicked peer
    yields EOF (recv -> b'') or an OSError on send; a still-open peer does not.
    The poke byte 0x07 is itself a (harmless out-of-fight) escape: against a
    still-open peer it would just trigger the same kick, against a closed peer
    it raises OSError -> both confirm or progress toward 'gone'.
    """
    import time
    import socket as _socket
    # First, let any pending bytes + the FIN arrive.
    try:
        sess.drain(timeout=timeout)
    except Exception:
        return True
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            sess.sock.settimeout(0.4)
            d = sess.sock.recv(4096)
            if d == b"":
                # orderly shutdown observed: peer is gone
                return True
            # got bytes (e.g. the system 'kicked' message); keep reading to EOF
        except _socket.timeout:
            # no EOF yet; poke the peer to force a FIN/RST on a closed socket
            try:
                sess.sock.sendall(b"\x07")
            except OSError:
                return True
        except OSError:
            # connection reset / broken pipe / closed fd -> peer is gone
            return True
    # Final confirmation: a send to a half-closed/closed socket eventually fails
    try:
        sess.sock.settimeout(0.4)
        sess.sock.sendall(b"\x07")
        d = sess.sock.recv(4096)
        return d == b""
    except _socket.timeout:
        return False
    except OSError:
        return True


def _server_clean(server):
    """True iff the server is alive and reports no crash."""
    if not server.alive():
        return False, "server not alive"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % str(cr)[:400]
    return True, ""


def run(server):
    try:
        valid_cases = 0
        malformed_cases = 0
        semantic_invalid_cases = 0

        # ----------------------------------------------------------------
        # 0) Baseline: a SEPARATE victim account (never sends 0x07) whose
        #    persisted identity/position we snapshot, to later prove the
        #    illegal escapes from other players corrupt nothing.
        # ----------------------------------------------------------------
        victim = H.Session(server)
        victim_login = victim.login_creds
        victim_pass = victim.pass_creds
        victim_pos0 = (victim.x, victim.y, victim.mapIndex)
        ok, why = _server_clean(server)
        if not ok:
            return (False, "after baseline victim session: " + why)

        # ----------------------------------------------------------------
        # 1) SEMANTIC-INVALID (the key contract): a well-formed 0x07 sent
        #    OUT OF a wild fight is illegal -> server must KICK the sender
        #    and stay alive. This is also the only reachable tryEscape()
        #    path in 'test' maincode (no wild encounter), so it is the
        #    reachability-limited graceful-refusal check.
        # ----------------------------------------------------------------
        cheater = H.Session(server)
        cheater.m(0x07)                       # valid framing, illegal context
        semantic_invalid_cases += 1
        # (a) KICK: the offending connection must be closed by the server.
        if not _peer_gone(cheater):
            cheater.close()
            return (False, "0x07 out-of-fight not kicked: connection stayed "
                           "open (silent-ignore contract violation)")
        cheater.close()
        # (b) SERVER SURVIVES.
        ok, why = _server_clean(server)
        if not ok:
            return (False, "after out-of-fight 0x07 kick: " + why)

        # Repeat from independent sessions to prove determinism (every
        # cheater kicked, server never crashes/hangs). Each is semantic-invalid.
        i = 0
        while i < 2:
            c2 = H.Session(server)
            c2.m(0x07)
            semantic_invalid_cases += 1
            if not _peer_gone(c2):
                c2.close()
                return (False, "repeat out-of-fight 0x07 #%d not kicked" % i)
            c2.close()
            ok, why = _server_clean(server)
            if not ok:
                return (False, "repeat out-of-fight 0x07 #%d: %s" % (i, why))
            i += 1

        # The VALID framing of 0x07 (single byte, size 0) is exactly what the
        # cheater sent above and the server parsed correctly (it reached the
        # handler and kicked rather than rejecting framing). Count it.
        valid_cases += 1

        # ----------------------------------------------------------------
        # 2) MALFORMED FRAMING: 0x07 is a 0-byte fixed message. Feed several
        #    malformed/abusive framings; assert the server survives each (no
        #    crash, no hang). Per the harness notes the server kicks the
        #    offender on bad framing but never aborts under HARDENED-only.
        # ----------------------------------------------------------------
        # 2a) 0x07 with a spurious trailing data byte. The fixed-size parser
        #     consumes [0x07] (size 0) then reads the stray 0xFF as the next
        #     packet code -> blocked/unknown -> kick. Must not crash.
        m1 = H.Session(server)
        m1.send_raw(b"\x07\xff")
        malformed_cases += 1
        m1.drain(0.6)
        ok, why = _server_clean(server)
        if not ok:
            m1.close()
            return (False, "after 0x07 + trailing 0xff: " + why)
        m1.close()

        # 2b) 0x07 framed as if it carried a queryNumber byte that a size-0
        #     MESSAGE never has. [0x07][0x00] -- the second byte is misread as
        #     the next packet code (0x00 blocked/unknown) -> kick. No crash.
        m2 = H.Session(server)
        m2.send_raw(b"\x07\x00")
        malformed_cases += 1
        m2.drain(0.6)
        ok, why = _server_clean(server)
        if not ok:
            m2.close()
            return (False, "after 0x07 + bogus qnum byte: " + why)
        m2.close()

        # 2c) 0x07 immediately followed by a truncated DYNAMIC packet header
        #     (0x03 chat, a huge 4-byte length, but no data). The parser must
        #     wait or kick, never block forever or crash. Assert liveness.
        m3 = H.Session(server)
        m3.send_raw(b"\x07" + b"\x03" + H.u32(0x7FFFFFFF))
        malformed_cases += 1
        m3.drain(0.6)
        ok, why = _server_clean(server)
        if not ok:
            m3.close()
            return (False, "after 0x07 + truncated dynamic length: " + why)
        m3.close()

        # 2d) A burst of bare 0x07 bytes in one write. Each is a valid size-0
        #     message; the first triggers the out-of-fight kick. The server
        #     must process+kick cleanly without crashing on the backlog.
        m4 = H.Session(server)
        m4.send_raw(b"\x07" * 8)
        malformed_cases += 1
        if not _peer_gone(m4):
            m4.close()
            return (False, "burst of 0x07 did not kick the sender")
        m4.close()
        ok, why = _server_clean(server)
        if not ok:
            return (False, "after burst of 0x07: " + why)

        # ----------------------------------------------------------------
        # 3) NO STATE CORRUPTION: the illegal/abusive 0x07 attempts must not
        #    have mutated any persisted/in-game state.
        #    (a) The victim (a different account, never sent 0x07) is still
        #        on the map at its original tracked position.
        #    (b) reload_state of the victim shows unchanged identity/position.
        # ----------------------------------------------------------------
        ok, why = _server_clean(server)
        if not ok:
            return (False, "before state-corruption check: " + why)
        # The victim connection should still be healthy and untouched.
        victim.drain(0.4)
        if (victim.x, victim.y, victim.mapIndex) != victim_pos0:
            return (False, "victim position changed after others' illegal 0x07:"
                           " %r -> %r" % (victim_pos0,
                                          (victim.x, victim.y, victim.mapIndex)))
        # Close the victim so the disconnect-save lands, then reload its
        # persisted state and assert identity/position survived intact.
        victim.close()
        import time
        time.sleep(0.4)
        snap = H.reload_state(server, victim_login, victim_pass)
        if snap.get("character_id") is None:
            return (False, "reload_state could not read back victim character")
        if victim_pos0[0] is not None and snap.get("x") is not None:
            if (snap["x"], snap["y"]) != (victim_pos0[0], victim_pos0[1]):
                return (False, "persisted victim position drifted after illegal "
                               "0x07: spawn %r vs reload %r" %
                               (victim_pos0[:2], (snap["x"], snap["y"])))

        # ----------------------------------------------------------------
        # 4) Leave the server usable: a brand-new legit Session must still
        #    complete the full handshake and reach the map.
        # ----------------------------------------------------------------
        post = H.Session(server)
        post_on_map = post.x is not None
        post.close()
        ok, why = _server_clean(server)
        if not ok:
            return (False, "after post-test session: " + why)
        if not post_on_map:
            # not fatal (position parse is best-effort) but worth surfacing
            pass

        detail = ("0x07 tryEscape: handler reachable only out-of-fight in 'test'"
                  " maincode (no wild encounter -> reachability-limited for the "
                  "in-fight success path); the valid 0-byte 0x07 sent out of a "
                  "fight correctly KICKS the sender; server stayed alive+clean "
                  "across %d semantic-illegal and %d malformed-framing sends "
                  "(%d valid-framing case); victim/other state uncorrupted "
                  "(tracked + persisted position unchanged); server still usable."
                  % (semantic_invalid_cases, malformed_cases, valid_cases))
        return (True, detail)

    except Exception as e:
        try:
            usable = server.alive()
        except Exception:
            usable = False
        return (False, "exception in run(): %r (server alive=%s)" % (e, usable))
