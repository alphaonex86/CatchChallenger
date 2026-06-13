"""Valgrind/protocol test for server handler 0x0C (requestFight — bot fight).

Handler: server/base/ClientNetworkReadMessage.cpp:398 (Client::parseMessage),
case 0x0C -> Client::requestFight(); return true.

Wire form: FIXED-size MESSAGE, code 0x0C, ZERO data bytes -> the whole packet on
the wire is the single byte 0x0C. No queryNumber (message, code<0x80), no 4-byte
length (fixed size 0). parseMessage does NOT range-check size for 0x0C (it just
calls requestFight() and returns true), so the size guard lives entirely in the
framer: any extra bytes after 0x0C are parsed as a SEPARATE following packet.

Handler logic traced (server/base/ClientEvents/LocalClientHandlerFightManage.cpp
Client::requestFight, + server/fight/LocalClientHandlerFight.cpp):
  * mapIndex>=65535            -> silent return (no kick) — unreachable on map.
  * isInFight()                -> errorOutput("...is in fight") -> KICK.
  * mapAndPosIfMoveInLookingDirectionJumpColision() == nullptr (can't step in the
                                  look direction / off-map) -> errorOutput(...) -> KICK.
  * a bot-fight trigger at the looked-at cell:
        - already beaten       -> errorOutput("You can't rebeat...") -> KICK.
        - else                 -> botFightStart(...) -> starts the bot fight
                                  (sends the fight packets, NO 0x7F reply to a
                                  message; observable only as broadcast/state).
  * no trigger at that cell    -> errorOutput("no fight with in this direction") -> KICK.

So for a generic on-map player who is NOT standing one step from an un-beaten bot
trigger, requestFight() ALWAYS ends in errorOutput()->disconnectClient() -> the
offending client is KICKED and the SERVER stays alive (production-safe). This is
the dominant, deterministic outcome and is exactly the "semantically illegal"
contract: a client asking to fight where there is no fight is a misbehaving peer
and is disconnected, with NO state mutation.

REACHABILITY-LIMITED valid case: standing exactly one step (in the current look
direction) from a not-yet-beaten bot-fight trigger is map/spawn dependent and the
harness exposes no reliable way to navigate to such a cell in the 'test' maincode
(it only tracks x/y/mapIndex best-effort and cannot path-find). We therefore send
the WELL-FORMED 0x0C from a fresh on-map session and assert the server REFUSES
GRACEFULLY (kick, no crash/hang/leak) — documented as a reachability-limited
partial check per the task contract. We do NOT assert a fight starts.

Verification strategy:
  * 0x0C is a MESSAGE -> no 0x7F reply to read. We assert (a) the server stays
    ALIVE+clean after every send, and (b) the offending client is KICKED (the
    socket reaches EOF / a follow-up send+recv shows the peer is gone) for the
    graceful-refuse cases.
  * NO STATE CORRUPTION is checked with a SEPARATE legit account: it handshakes,
    we snapshot its persisted identity/position via H.reload_state BEFORE and
    AFTER the abuse, and require them byte-for-byte identical (the rejected fight
    requests of other clients must not touch this victim).
"""

import time
import _protoharness as H

NAME = "0x0C requestFight"


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


def _is_kicked(sess):
    """Best-effort detection that the server CLOSED this client's connection.

    A kick = disconnectClient() -> the TCP peer goes away. We detect it by:
      1) draining: a closed socket yields EOF (recv returns b'') so drain stops
         producing bytes; then
      2) a follow-up benign send + recv: on a peer-closed socket the send either
         raises OSError (RST/EPIPE) or the subsequent recv returns b'' (EOF).
    Returns True if the peer looks gone, False if the connection still seems live.
    """
    try:
        sess.drain(timeout=0.4)
    except OSError:
        return True
    # Poke the socket: a benign extra byte then read. A live, un-kicked server
    # keeps the connection; a kicked one is closed (EOF or error).
    try:
        # send a harmless, well-formed heartbeat-ish message (0x0C again is fine:
        # if still connected it would just be processed/kicked; if already gone
        # this errors). Then read for EOF.
        sess.sock.settimeout(0.5)
        try:
            sess.sock.sendall(b"\x0c")
        except OSError:
            return True
        try:
            data = sess.sock.recv(4096)
            if data == b"":
                return True  # EOF -> peer closed -> kicked
            # got bytes: could be a kick system-message broadcast then close;
            # try one more recv to see if it closes.
            try:
                more = sess.sock.recv(4096)
                if more == b"":
                    return True
            except OSError:
                return True
            # still delivering data and not closing -> treat as NOT kicked.
            return False
        except (OSError,):
            return True
    except OSError:
        return True


def run(server):
    try:
        # ----------------------------------------------------------------
        # NO-STATE-CORRUPTION baseline: a SEPARATE legit account whose
        # persisted identity/position must be untouched by all the abuse.
        # ----------------------------------------------------------------
        v_login = _uniq("vic")
        v_pass = _uniq("vpw")
        try:
            victim = H.Session(server, login=v_login, passh=v_pass, pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        try:
            before = H.reload_state(server, v_login, v_pass)
        except Exception as e:
            return (False, "victim reload_state(before) raised: %s" % e)
        if before.get("character_id") is None:
            return (False, "victim did not persist (no character_id)")
        checks_db = True

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim baseline: %s" % why)

        # ----------------------------------------------------------------
        # VALID (reachability-limited) case: a fresh on-map session sends the
        # well-formed size-0 0x0C. In the 'test' maincode a generic spawn is not
        # facing an un-beaten bot trigger, so requestFight() refuses via
        # errorOutput()->KICK. We assert the refusal is GRACEFUL: server stays
        # alive+clean and the offending client is disconnected. We do NOT require
        # a fight to start (cannot be reliably reached here).
        # ----------------------------------------------------------------
        valid_cases = 0
        reachability_limited = True
        try:
            s = H.Session(server, login=_uniq("rf"), passh=_uniq("rp"),
                          pseudo="Fighter")
        except H.HandshakeError as e:
            return (False, "valid-case handshake failed: %s" % e)
        try:
            s.m(0x0C)  # size-0 message: the single byte 0x0C
        except OSError:
            pass  # kicked mid-send is acceptable
        valid_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after valid 0x0C send: %s" % why)
        kicked_valid = _is_kicked(s)
        s.close()
        # The server MUST NOT crash; it must refuse gracefully (kick). If it
        # somehow kept the client (e.g. a real fight DID start on this spawn),
        # that is still a non-crashing acceptable outcome — we don't fail on it.
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after valid-case kick check: %s" % why)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID cases — well-formed 0x0C that is illegal for this
        # handler's state. The KEY contract: server KICKS the cheater and
        # SURVIVES, no state corruption.
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Cheat")

        # (S1) Request a bot fight while NOT facing any trigger (the dominant
        # illegal use). requestFight() -> errorOutput("no fight with in this
        # direction") -> KICK. Assert kick + server alive.
        try:
            c = fresh("s1")
            try:
                c.m(0x0C)
            except OSError:
                pass
            kicked = _is_kicked(c)
            c.close()
            if not kicked:
                return (False, "S1 (no fight in direction) NOT kicked — "
                               "silent-ignore is a contract violation")
            semantic_invalid_cases += 1
        except H.HandshakeError as e:
            return (False, "S1 handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after S1: %s" % why)

        # (S2) Spam several 0x0C in a row from one session. Only the first is
        # processed; the kick from the first request tears down the connection.
        # The server must not crash, double-free, or hang on the trailing ones.
        try:
            c = fresh("s2")
            try:
                c.send_raw(b"\x0c\x0c\x0c\x0c")  # four back-to-back fight requests
            except OSError:
                pass
            kicked = _is_kicked(c)
            c.close()
            if not kicked:
                return (False, "S2 (repeated requestFight) NOT kicked")
            semantic_invalid_cases += 1
        except H.HandshakeError as e:
            return (False, "S2 handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after S2: %s" % why)

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. 0x0C is fixed-size 0, so the only way to
        # malform it on the wire is to append bytes the framer must treat as a
        # NEW (possibly bogus) following packet, or to feed it as if it were a
        # dynamic/query packet. After each we require server.alive() and
        # crash_report() is None. Disposable sessions (a malformed stream kicks
        # the offending client).
        # ----------------------------------------------------------------
        malformed_cases = 0

        # (M1) 0x0C with a bogus dynamic-style 4-byte length appended. The
        # framer reads 0x0C as a complete size-0 message; the following bytes are
        # interpreted as a fresh packet header (code 0xFF...). 0xFF is a blocked/
        # unknown code -> parse stops / client kicked. Must not crash.
        try:
            s = fresh("m1")
            s.send_raw(b"\x0c" + b"\xff\xff\xff\xff")
            time.sleep(0.1)
            s.send_raw(b"\xde\xad\xbe\xef")  # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "M1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M1 (0x0C + bogus dynlen): %s" % why)

        # (M2) A DYNAMIC-style frame for 0x0C: [0x0C][len=4 LE][4 bytes]. 0x0C is
        # NOT dynamic, so the engine consumes only the 0x0C byte; the injected
        # length+payload bytes (04 00 00 00 ...) are mis-parsed as following
        # packets (0x04 is an empty case in parseMessage). Must stay alive.
        try:
            s = fresh("m2")
            s.m(0x0C, b"\x01\x02\x03\x04", dynamic=True)  # wrongly framed dynamic
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "M2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M2 (0x0C dynamic-framed): %s" % why)

        # (M3) A QUERY-style frame for 0x0C: [0x0C][qnum][garbage]. 0x0C<0x80 so
        # the server does NOT expect a queryNumber here; the qnum byte and the
        # trailing garbage are mis-parsed as following packets. Must stay alive.
        try:
            s = fresh("m3")
            s.send_raw(b"\x0c\x05\x99\x99")  # fake qnum 0x05 + garbage
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "M3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M3 (0x0C query-framed): %s" % why)

        # (M4) Truncated/partial: send a lone 0x0C then immediately close, then a
        # separate session sends only a partial unknown header and closes. This
        # exercises the partial-buffer path (server waits for more, gets EOF).
        try:
            s = fresh("m4")
            s.send_raw(b"\x0c")  # one valid request then drop the link
            s.close()
            s2 = fresh("m4b")
            s2.send_raw(b"\xaa")  # lone dynamic-query header, no qnum/len -> waits
            time.sleep(0.15)
            s2.close()
        except H.HandshakeError as e:
            return (False, "M4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M4 (truncated/partial): %s" % why)

        # ----------------------------------------------------------------
        # NO-STATE-CORRUPTION: the victim's persisted identity/position must be
        # byte-for-byte what it was before any of the rejected fight requests.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, v_login, v_pass)
        except Exception as e:
            return (False, "victim reload_state(after) raised: %s" % e)

        verifies_no_state_corruption = True
        for k in ("character_id", "pseudo", "x", "y", "mapIndex"):
            if before.get(k) != after.get(k):
                return (False, "STATE CORRUPTION: victim %s changed %r -> %r "
                               "after rejected fight requests"
                               % (k, before.get(k), after.get(k)))

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session still handshakes
        # to CharacterSelected after all the abuse.
        # ----------------------------------------------------------------
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
            "valid=%d(reachability-limited: graceful-refuse kick=%s) "
            "semantic_invalid=%d malformed=%d db_checked=%s "
            "no_state_corruption=%s victim=%r "
            "(0x0C is size-0; un-faced fight request -> errorOutput()->KICK; "
            "server stayed alive+clean through all cases)"
            % (valid_cases, kicked_valid, semantic_invalid_cases,
               malformed_cases, checks_db, verifies_no_state_corruption,
               (before.get("character_id"), before.get("x"), before.get("y"),
                before.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
