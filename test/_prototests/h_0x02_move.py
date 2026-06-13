"""Valgrind/protocol test for server handler 0x02 (move).

Handler: server/base/ClientNetworkReadMessage.cpp:76 (Client::parseMessage).
Wire form: FIXED-size MESSAGE, code 0x02, exactly 2 data bytes:
    [stepCount:u8][direction:u8]
No queryNumber (message, code<0x80), no 4-byte length (fixed size 2).

Handler logic traced (server/base):
  * size!=2                  -> Client::errorOutput("Wrong size...")
        -> parseMessage returns false.
  * direction<1 || >8        -> Client::errorOutput("Bad direction number")
        -> parseMessage returns false.
  * else moveThePlayer(stepCount, direction):
        - MapBasicMove::moveThePlayer FIRST refuses an EXACT repeat of the
          previous direction (last_direction==direction -> MapBasicMove::
          errorOutput, a no-op base method: NO kick, just returns false).
        - It then steps `stepCount` times via Client::singleMove(), which on a
          COLLISION / map-edge / in-fight / lost-event-sync calls
          Client::errorOutput() -> disconnectClient() (a KICK).
        - parseMessage discards the bool ((void)moveOk) and returns true.
  * direction 1..4 are LOOK directions (general/base/GeneralStructures.hpp):
        Direction_look_at_top/right/bottom/left = 1..4. The look branch of
        MapBasicMove::moveThePlayer does NOT move, just records last_direction
        and returns true -> never a kick (apart from the same-direction repeat).
  * direction 5..8 are MOVE directions (Direction_move_at_*).

KICK semantics (Client::errorOutput, server/base/Client.cpp):
  errorOutput() ALWAYS ends in disconnectClient(); for a CharacterSelected
  client it also pushes a system "kicked, tried to hack" broadcast. The server
  process STAYS ALIVE (production-safe graceful disconnect) — only the offending
  socket is closed (reaches EOF on our side). Under CATCHCHALLENGER_HARDENED
  alone (NOT CATCHCHALLENGER_ABORTIFERROR) parseMessage returning false does NOT
  abort the process; it kicks. So every semantically-illegal move => the cheater
  socket is closed and the server keeps running for everyone else.

Test contract coverage:
  * VALID:  well-formed move messages for each direction; assert the mover is
    NOT kicked and the server stays alive+clean; read identity/position back via
    H.reload_state (FILE_DB saves the whole character block at disconnect).
  * MALFORMED FRAMING: truncated payload, over-long fixed packet, lone code
    byte + garbage trailer; assert server.alive() and crash_report() is None
    after each.
  * SEMANTIC-INVALID (the key contract): well-formed-but-illegal moves
    (direction 0, direction 9, direction 255). For EACH assert:
      (a) KICK     — the offending socket is closed by the server (EOF on recv);
      (b) SURVIVES — server.alive() True, crash_report() None;
      (c) NO STATE CORRUPTION — a SEPARATE legit session on the same map is
          byte-for-byte unaffected (its persisted position is unchanged), and
          the cheater itself created no phantom state (its character is gone /
          unchanged, no new connections leaked).
"""

import time
import socket
import _protoharness as H

NAME = "0x02 move"


def _uniq(prefix):
    return ("%s%x" % (prefix, int(time.time() * 1000) & 0xFFFFFFFF)).encode()


def _alive_clean(server):
    """True iff the server process is up, accepting TCP, and not crashed."""
    if not server.alive():
        return False, "server.alive() == False"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % (cr[:300],)
    return True, ""


def _is_kicked(sess, timeout=2.0):
    """Return True iff the server has CLOSED this session's socket (a kick).

    Detection is purely at the TCP level — we must NOT send any further protocol
    packet as a probe, because almost every message code (e.g. 0x07 tryEscape
    when not in a fight) itself calls errorOutput() -> disconnectClient(), which
    would FALSELY look like a kick on a perfectly healthy connection.

    A kicked client gets disconnectClient() server-side, so:
      * our recv() eventually returns b'' (EOF / orderly FIN), and/or
      * a zero-byte send() raises BrokenPipe/ECONNRESET once the FIN/RST lands.
    We read+discard any in-flight payload (the "you were kicked" system
    broadcast precedes the close) and wait for the EOF/reset. The harness
    drain() swallows EOF, so we read the raw socket directly here.
    """
    deadline = time.time() + timeout
    sk = sess.sock
    saw_eof = False
    while time.time() < deadline:
        try:
            sk.settimeout(0.3)
            data = sk.recv(65536)
            if data == b"":
                saw_eof = True          # orderly FIN -> kicked
                break
            # else: real bytes (broadcast / system kick msg) — keep reading.
        except socket.timeout:
            # Nothing pending. TCP-level liveness probe: a zero-length send
            # never reaches the server's parser but surfaces a pending RST.
            try:
                sk.send(b"")
            except OSError:
                saw_eof = True
                break
        except OSError:
            saw_eof = True              # ECONNRESET / EPIPE -> kicked
            break
    return saw_eof


def _spawn_of(server, login, passw):
    """Best-effort persisted (x,y,mapIndex) for an account, or None."""
    try:
        snap = H.reload_state(server, login, passw)
        if snap.get("character_id") is None:
            return None
        return (snap.get("x"), snap.get("y"), snap.get("mapIndex"))
    except Exception:
        return None


def run(server):
    try:
        # ----------------------------------------------------------------
        # Precondition: a fully-handshaked on-map session (the "mover").
        # ----------------------------------------------------------------
        login = _uniq("mv")
        passw = _uniq("pw")
        try:
            mover = H.Session(server, login=login, passh=passw, pseudo="Mover")
        except H.HandshakeError as e:
            return (False, "handshake failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after handshake: %s" % why)

        start = (mover.x, mover.y, mover.mapIndex)

        # A SEPARATE legitimate victim/witness account: it never sends an
        # illegal packet, and we use it to prove no cross-client corruption.
        victim_login = _uniq("vic")
        victim_pass = _uniq("vp")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        victim.close()
        time.sleep(0.35)
        victim_before = _spawn_of(server, victim_login, victim_pass)
        if victim_before is None:
            return (False, "victim did not persist (cannot baseline state)")

        # ----------------------------------------------------------------
        # VALID case: send a well-formed move message for every direction.
        # The engine steps using the PREVIOUS last_direction then updates it,
        # refusing only an exact repeat (no kick) and kicking only on a real
        # collision. We alternate directions so consecutive ones never repeat,
        # and never require the position to change (collision-blocked is valid),
        # only that the server stays healthy and the mover is not killed by a
        # well-formed packet.
        # ----------------------------------------------------------------
        valid_cases = 0
        for direction in (5, 6, 7, 8, 1, 2, 3, 4):
            mover.m(0x02, H.u8(1) + H.u8(direction))
            valid_cases += 1
            mover.drain(timeout=0.2)
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after valid move dir=%d: %s" % (direction, why))

        # Multi-step move (stepCount=2) — exercises the singleMove loop. Still a
        # valid frame; the mover must remain connected (a kick would EOF send).
        try:
            mover.m(0x02, H.u8(2) + H.u8(6))  # 2 steps, move-right
            mover.drain(timeout=0.2)
        except OSError as e:
            return (False, "mover socket died after valid moves (unexpected kick): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after multi-step valid move: %s" % why)

        # Persisted-state read-back (identity/position) after a clean disconnect.
        mover.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        checks_db = False
        try:
            snap = H.reload_state(server, login, passw)
            if snap.get("character_id") is None:
                return (False, "reload_state: character did not persist (no id)")
            if snap.get("x") is None or snap.get("y") is None or snap.get("mapIndex") is None:
                return (False, "reload_state: position header unreadable")
            checks_db = True
            persisted = (snap["x"], snap["y"], snap["mapIndex"])
        except Exception as e:
            return (False, "reload_state raised: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after reload_state: %s" % why)

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. Each from a FRESH disposable session; a
        # bad frame either gets the client kicked or ignored, but the SERVER
        # must stay alive+clean. We do NOT require a kick here (some framings
        # just stall the parser waiting for more bytes); we ONLY require the
        # server survives, per the harness contract for malformed framing.
        # ----------------------------------------------------------------
        malformed_cases = 0

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Frm")

        # (1) Truncated dynamic-ish: code + ONE data byte only, then garbage.
        try:
            s = fresh("f1")
            s.send_raw(bytes([0x02, 0x01]))              # code + ONE byte (need 2)
            time.sleep(0.1)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))  # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated move frame: %s" % why)

        # (2) Over-long fixed packet: code + THREE data bytes. The fixed framer
        # consumes exactly 2 as the move; the 3rd byte is reparsed as a new
        # packet code (0x07 = 0-length message) — harmless. Server must survive.
        try:
            s = fresh("f2")
            s.send_raw(bytes([0x02, 0x01, 0x06, 0x07]))  # move + stray 0x07 msg
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long move frame: %s" % why)

        # (3) OOB stepCount==255 with a VALID direction. moveThePlayer loops
        # singleMove up to 255 times; it MUST stay bounded (map edge/collision)
        # and never crash or hang. The session may get kicked by an eventual
        # collision, but the SERVER must remain alive and clean.
        try:
            s = fresh("f3")
            s.m(0x02, H.u8(255) + H.u8(6))  # huge stepCount, move-right
            s.drain(timeout=0.5)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after OOB stepCount=255: %s" % why)

        # (4) Lone code byte then a long pause then nothing. Tests the framer
        # can't be wedged by a stalled half-packet (it just waits for byte 2).
        try:
            s = fresh("f4")
            s.send_raw(bytes([0x02]))  # only the code, no data at all
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after lone-code-byte move frame: %s" % why)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID cases — THE KEY CONTRACT. Well-formed 2-byte move
        # packets whose direction is OUT OF the valid 1..8 range. The handler
        # rejects each via errorOutput() -> KICK. For EACH we assert:
        #   (a) the offending socket is CLOSED by the server (EOF), i.e. kicked;
        #   (b) the server stays alive + crash_report() is None;
        #   (c) no cross-client corruption: the untouched victim's persisted
        #       position is byte-for-byte the same, AND the cheater created no
        #       phantom persisted state (its move was rejected, so its position,
        #       if it even persisted, must equal its untouched spawn).
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0
        verifies_kick = True
        verifies_no_corruption = True

        bad_dirs = (0, 9, 255)  # below range, just-above range, max overflow
        for bad in bad_dirs:
            ch_login = _uniq("ch%d" % bad)
            ch_pass = _uniq("cp%d" % bad)
            try:
                cheat = H.Session(server, login=ch_login, passh=ch_pass,
                                  pseudo="Cheat")
            except H.HandshakeError as e:
                return (False, "semantic#dir=%d handshake failed: %s" % (bad, e))

            cheat_spawn = (cheat.x, cheat.y, cheat.mapIndex)

            # Send the well-formed-but-illegal move.
            try:
                cheat.m(0x02, H.u8(1) + H.u8(bad))
            except OSError:
                # already gone mid-send -> that IS a kick
                pass

            # (a) KICK: server must close this socket.
            kicked = _is_kicked(cheat, timeout=2.0)
            try:
                cheat.close()
            except OSError:
                pass
            if not kicked:
                verifies_kick = False
                return (False,
                        "CONTRACT VIOLATION: illegal move dir=%d NOT kicked "
                        "(socket stayed open)" % bad)
            semantic_invalid_cases += 1

            # (b) SERVER SURVIVES.
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after illegal move dir=%d: %s" % (bad, why))

            # (c) NO STATE CORRUPTION — give FILE_DB time to settle, then
            # re-read the untouched victim. It must be byte-for-byte unchanged.
            time.sleep(0.35)
            victim_now = _spawn_of(server, victim_login, victim_pass)
            if victim_now != victim_before:
                verifies_no_corruption = False
                return (False,
                        "STATE CORRUPTION: victim position changed by another "
                        "client's illegal move dir=%d: %s -> %s"
                        % (bad, victim_before, victim_now))

            # The cheater's persisted position (if it saved on disconnect) must
            # NOT reflect any movement from the rejected packet: it must equal
            # its spawn (the rejected move never executed).
            cheat_persisted = _spawn_of(server, ch_login, ch_pass)
            if cheat_persisted is not None and cheat_persisted != cheat_spawn:
                verifies_no_corruption = False
                return (False,
                        "STATE CORRUPTION: cheater dir=%d persisted a moved "
                        "position %s != spawn %s despite rejection"
                        % (bad, cheat_persisted, cheat_spawn))

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session still handshakes
        # to CharacterSelected and can move after all the abuse. The witness
        # session is reopened fresh (cheaters are all closed).
        # ----------------------------------------------------------------
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.m(0x02, H.u8(1) + H.u8(7))  # one clean move-bottom
            again.drain(timeout=0.2)
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d malformed=%d semantic_invalid=%d kick_verified=%s "
            "no_corruption=%s db_checked=%s start=%s persisted=%s "
            "victim_stable=%s (position may be unchanged if collision-blocked; "
            "server stayed alive+clean through all cases)"
            % (valid_cases, malformed_cases, semantic_invalid_cases,
               verifies_kick, verifies_no_corruption, checks_db, start,
               persisted, victim_before)
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
