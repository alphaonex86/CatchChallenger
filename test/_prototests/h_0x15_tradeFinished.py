"""Valgrind/protocol test for server handler 0x15 (tradeFinished).

Handler: server/base/ClientNetworkReadMessage.cpp:581 (Client::parseMessage),
which dispatches to Client::tradeFinished()
(server/base/ClientEvents/LocalClientHandlerTrade.cpp:85).

Wire form: FIXED-size MESSAGE, code 0x15, size 0
(packetFixedSize[0x15]=0, general/base/ProtocolParsingGeneral.cpp:86).
On the wire it is exactly one byte: [0x15]. No queryNumber (message,
code<0x80), no 4-byte length (fixed size, not 0xFE).

Handler logic traced (LocalClientHandlerTrade.cpp:85-164):
  void Client::tradeFinished():
    if(!tradeIsValidated)  -> errorOutput("Trade not valid")              -> KICK
    if(tradeIsFreezed)     -> errorOutput("Trade is freezed, unable...")  -> KICK
    tradeIsFreezed=true;
    Client &other = ClientList::list->rw(this->otherPlayerTrade);
    if(this freezed && other freezed) { ...do the swap, resetTheTrade()... }
    else { send 0x5A "I froze" to the other side }

  tradeIsValidated is initialised to false (Client.cpp:210) and
  otherPlayerTrade to PLAYER_INDEX_FOR_CONNECTED_MAX (Client.cpp:129).
  errorOutput() (Client.cpp:524) ALWAYS kicks (disconnectClient()): there is
  NO error-reply-without-kick path for this handler.

REACHABILITY (documented partial):
  Reaching the "trade is validated AND both sides frozen" branch needs a full
  two-player trade negotiation (trade request 0x53/0x14, accept, validate)
  that is not driveable from the 'test' maincode here without the matching
  trade-request handlers and a peer accept loop. So the VALID, state-mutating
  path (the actual item/cash/monster swap) is reachability-limited and is NOT
  exercised; this is flagged reachability_limited=True. What IS fully exercised
  and is the security-critical contract for this opcode:

  * SEMANTIC-INVALID: a well-formed 0x15 sent by an on-map player who has NO
    validated trade. tradeIsValidated==false -> the server MUST kick that
    client ("Trade not valid") and MUST NOT crash, hang, leak, or mutate any
    state (no phantom cash/items/monsters, otherPlayerTrade still MAX).
  * DOUBLE-FINISH: two 0x15 in a row from the same fresh session. The first
    already kicks; the second lands on a closed connection. Server must stay
    alive/clean.
  * MALFORMED FRAMING: 0x15 carrying spurious trailing bytes / truncated
    garbage. The fixed-size-0 framer must not be wedged; server stays alive.

Verification strategy:
  0x15 is a MESSAGE -> no 0x7F reply for the sender. The observable contract
  is the KICK (socket EOF) + server staying alive + clean, plus NO state
  corruption verified through a SEPARATE legitimate session and H.reload_state.
"""

import time
import socket
import _protoharness as H

NAME = "0x15 tradeFinished"


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


def _is_kicked(sess, timeout=1.5):
    """Return True iff the offending client's connection was closed by the
    server (a KICK). Detection: read the raw socket for EOF (recv -> b''),
    then confirm with a follow-up byte that the peer is really gone (broken
    pipe / further EOF). A still-open socket that keeps accepting our bytes
    and never EOFs == NOT kicked (contract violation for this handler)."""
    sk = sess.sock
    deadline = time.time() + timeout
    saw_eof = False
    sk.settimeout(0.3)
    while time.time() < deadline:
        try:
            d = sk.recv(4096)
            if d == b"":          # orderly shutdown from the server == kick
                saw_eof = True
                break
            # any bytes here are just the kick system-message / drain; keep reading
        except socket.timeout:
            # nothing pending yet; poke the peer to force a verdict
            try:
                sk.sendall(b"\x15")   # a 2nd finish byte onto a (maybe) dead peer
            except OSError:
                saw_eof = True        # broken pipe == peer gone == kicked
                break
        except OSError:
            saw_eof = True            # connection reset == kicked
            break
    if not saw_eof:
        # Final confirmation: try one more send + read; a live peer would still
        # be readable/writable, a kicked one yields EOF/error.
        try:
            sk.sendall(b"\x15")
            sk.settimeout(0.5)
            try:
                if sk.recv(4096) == b"":
                    saw_eof = True
            except socket.timeout:
                saw_eof = False
            except OSError:
                saw_eof = True
        except OSError:
            saw_eof = True
    return saw_eof


def run(server):
    try:
        # ----------------------------------------------------------------
        # A SEPARATE, untouched legitimate session: its persisted state is the
        # baseline we assert is NOT corrupted by any rejected 0x15 from cheaters.
        # ----------------------------------------------------------------
        vlogin = _uniq("vic")
        vpassw = _uniq("vpw")
        try:
            victim = H.Session(server, login=vlogin, passh=vpassw, pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)
        victim_start = (victim.x, victim.y, victim.mapIndex)
        # Persist the victim's clean baseline (disconnect-save) and read it back.
        victim.close()
        time.sleep(0.35)
        try:
            base_snap = H.reload_state(server, vlogin, vpassw)
        except Exception as e:
            return (False, "baseline reload_state raised: %s" % e)
        if base_snap.get("character_id") is None:
            return (False, "victim did not persist (no character id)")

        valid_cases = 0
        semantic_invalid_cases = 0
        malformed_cases = 0
        verifies_kick = True
        reachability_limited = True  # validated-trade swap path not driveable here

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID #1 (the key contract): a well-formed 0x15 from an
        # on-map session that has NO validated trade. tradeIsValidated==false
        # -> server kicks ("Trade not valid"). This is also the only form of
        # 0x15 reachable from a single session, so it doubles as the "send the
        # well-formed packet and assert graceful refusal" reachability-limited
        # valid-shape check.
        # ----------------------------------------------------------------
        try:
            cheat = H.Session(server, login=_uniq("c1"), passh=_uniq("p1"),
                              pseudo="Cheat1")
        except H.HandshakeError as e:
            return (False, "cheat1 handshake failed: %s" % e)
        cheat.m(0x15)  # well-formed: fixed size 0, just the opcode byte
        valid_cases += 1                 # well-formed packet shape exercised
        semantic_invalid_cases += 1      # semantically illegal (no trade)
        kicked = _is_kicked(cheat)
        cheat.close()
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x15 with no validated trade: %s" % why)
        if not kicked:
            verifies_kick = False
            return (False, "0x15 with no validated trade did NOT kick the "
                           "client (server silently ignored a contract-violating "
                           "tradeFinished -> CONTRACT VIOLATION)")

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID #2: DOUBLE-FINISH. Send two 0x15 back-to-back from a
        # fresh session. The first kicks; the second is delivered against (or
        # after) a closed connection. Per the handler notes this must NOT crash.
        # ----------------------------------------------------------------
        try:
            cheat2 = H.Session(server, login=_uniq("c2"), passh=_uniq("p2"),
                               pseudo="Cheat2")
        except H.HandshakeError as e:
            return (False, "cheat2 handshake failed: %s" % e)
        try:
            cheat2.send_raw(b"\x15\x15")  # two finishes in one write
        except OSError:
            pass  # kicked mid-write is acceptable
        semantic_invalid_cases += 1
        kicked2 = _is_kicked(cheat2)
        cheat2.close()
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after double 0x15: %s" % why)
        if not kicked2:
            verifies_kick = False
            return (False, "double 0x15 did NOT kick the client")

        # ----------------------------------------------------------------
        # MALFORMED FRAMING cases. 0x15 is fixed-size-0; spurious trailing bytes
        # or truncated garbage must not wedge the framer. Each uses a disposable
        # session; after each we assert server alive + clean.
        # ----------------------------------------------------------------
        def fresh():
            return H.Session(server, login=_uniq("bad"), passh=_uniq("bp"),
                             pseudo="Bad")

        # (M1) 0x15 followed immediately by a stray data byte. The fixed-0 framer
        # consumes only the opcode; the 0xFF that follows is an unknown/blocked
        # code -> the offending client is kicked, server survives.
        try:
            s = fresh()
            s.send_raw(bytes([0x15, 0xFF]))
            time.sleep(0.1)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x15+stray byte: %s" % why)

        # (M2) 0x15 with a bogus 4-byte length prefix as if it were a dynamic
        # packet (it is NOT dynamic). The server frames it as fixed-0, so the
        # length bytes are interpreted as following packets -> garbage codes ->
        # kick. Must not crash/hang.
        try:
            s = fresh()
            s.send_raw(bytes([0x15]) + H.u32(0xFFFFFFFF))
            time.sleep(0.1)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x15+bogus length: %s" % why)

        # (M3) A burst of 0x15 opcodes in one write (rapid repeated finish). The
        # first kicks; the rest land on a torn-down connection. No crash/hang.
        try:
            s = fresh()
            s.send_raw(bytes([0x15]) * 8)
            time.sleep(0.1)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x15 burst: %s" % why)

        # (M4) 0x15 then a truncated dynamic header (a half-written 4-byte len of
        # an unrelated dynamic code) to probe partial-frame handling. Server must
        # not wedge waiting forever; the offending client is kicked, server lives.
        try:
            s = fresh()
            s.send_raw(bytes([0x15, 0x03, 0x02, 0x00]))  # 0x15 + start of dyn 0x03 chat, truncated len
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF]))              # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x15+truncated dynamic: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: re-read the untouched victim's persisted state via
        # a SEPARATE legitimate path. None of the rejected 0x15 attempts (from
        # other accounts) may have changed it. Identity + position must be
        # byte-for-byte the baseline (cash/items/monsters are not in the reload
        # header per the foundation's documented limit; the swap path was never
        # reached so no cash/item/monster could move anyway).
        # ----------------------------------------------------------------
        try:
            after_snap = H.reload_state(server, vlogin, vpassw)
        except Exception as e:
            return (False, "post-abuse reload_state raised: %s" % e)
        checks_db = (after_snap.get("character_id") is not None)
        verifies_no_state_corruption = True
        for k in ("character_id", "pseudo", "x", "y", "mapIndex"):
            if after_snap.get(k) != base_snap.get(k):
                verifies_no_state_corruption = False
                return (False, "victim state changed by a rejected 0x15: %s "
                               "baseline=%r after=%r"
                               % (k, base_snap.get(k), after_snap.get(k)))

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after corruption re-check: %s" % why)

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session still reaches
        # CharacterSelected after all the abuse.
        # ----------------------------------------------------------------
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.drain(timeout=0.2)
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid(shape)=%d semantic_invalid=%d malformed=%d kick_verified=%s "
            "no_corruption=%s db_checked=%s reachability_limited=%s "
            "victim_start=%s victim_persisted=(%s,%s,%s); 0x15 with no validated "
            "trade kicked the cheater, double/burst/malformed-framed 0x15 left "
            "the server alive+clean, victim state unchanged. NOTE: the validated "
            "two-player swap branch is not driveable from the 'test' maincode "
            "(no trade-request/accept loop here) -> reachability-limited."
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               verifies_kick, verifies_no_state_corruption, checks_db,
               reachability_limited, victim_start,
               after_snap.get("x"), after_snap.get("y"), after_snap.get("mapIndex"))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
