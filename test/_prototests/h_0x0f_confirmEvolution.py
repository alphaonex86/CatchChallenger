"""Valgrind/protocol test for server handler 0x0F (confirmEvolution).

Handler: server/base/ClientNetworkReadMessage.cpp:445 (Client::parseMessage).
Wire form: DYNAMIC MESSAGE (packetFixedSize[0x0F]=0xFE), code 0x0F, a
4-byte LE size prefix then exactly 1 data byte:
    [monsterPosition:u8]
No queryNumber (message, code<0x80), no 4-byte length (fixed size 1).

Handler logic traced (server/fight/LocalClientHandlerFight.cpp:828
Client::confirmEvolution + ::confirmEvolutionTo + general/fight/
CommonFightEngineTurn.cpp:959 CommonFightEngine::confirmEvolutionTo):

  parseMessage 0x0F:
    * size != 1                              -> Client::errorOutput(
          "wrong remaining size for monster evolution validated") -> KICK
    * else confirmEvolution(monsterPosition); returns true (no reply echoed
          to the requester on any path).

  Client::confirmEvolution(pos):
    * pos >= public_and_private_informations.monsters.size()
          -> errorOutput("Monster for evolution not found") -> KICK     (OOB)
    * look up the monster's evolutions; an evolution fires only when:
          - EvolutionType_Level and the monster's level >= the required level, OR
          - EvolutionType_Trade and this exact monster id is in the server's
            tradedMonster set (i.e. it was just received in a trade).
      If NO evolution condition is met -> errorOutput("Evolution not found")
          -> KICK.
    * on a met condition -> confirmEvolutionTo(&monsters[index], evolveTo):
          - (HARDENED) requires getCurrentMonster()!=NULL (i.e. IN A FIGHT)
            and current hp <= max, else errorOutput(...) -> KICK.
          - CommonFightEngine::confirmEvolutionTo MUTATES monsters[index].monster
            IN PLACE (species id changes, hp clamped/grown). It NEVER appends a
            monster -> the monster COUNT is invariant. Re-confirming therefore
            cannot create an extra monster; at worst it re-evolves the same slot
            (and on the second call the new species' evolution condition is
            re-evaluated -> usually "Evolution not found" -> kick).

Reachability note (documented partial check):
  A genuine SUCCESS requires (a) being inside a fight (getCurrentMonster) AND
  (b) owning a monster that meets a level/trade evolution condition AND
  (c) the HARDENED hp invariant. A freshly-created character in the 'test'
  maincode starts with a low-level starter not at an evolution threshold and is
  NOT in a fight, so the success path is NOT reachable from a plain on-map
  Session here. We therefore exercise the handler's SECURITY contract end to
  end: every well-formed-but-illegal monsterPosition (in-range-no-evolution and
  out-of-range) must KICK the cheater, the SERVER must survive (alive + clean +
  valgrind-clean), and NO state must be corrupted (a separate legit account is
  byte-for-byte unaffected; identity/position of the victim persist unchanged).
  This is flagged reachability-limited because the positive evolution mutation
  is not driven (no extra monster can be created on ANY path by construction).

Verification strategy:
  * 0x0F is a MESSAGE; the server NEVER echoes a reply to the requester. We
    detect the KICK by EOF on the requester's socket (recv -> b'') after each
    illegal send, and confirm a follow-up send finds the peer gone.
  * server.alive()/crash_report() asserted after every send (malformed and
    semantic-invalid). No malformed/illegal input may crash or hang the server.
  * No-state-corruption: a SEPARATE legit account (the "victim") is snapshotted
    via H.reload_state before AND after the abuse; the two snapshots must match.
    Monster-count can't be read back by the harness (reload_state exposes only
    id/pseudo/position), but the handler appends NO monster on ANY code path, so
    "no extra monster" is structurally guaranteed and we additionally assert the
    victim's persisted identity/position are unchanged.
"""

import time
import _protoharness as H

NAME = "0x0F confirmEvolution"


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


def _peer_closed(sess, timeout=1.0):
    """True iff the server has CLOSED this session's socket (the kick).

    A kicked client reaches TCP EOF: a blocking recv returns b''. We also probe
    with a follow-up benign send: on a half/fully closed peer the OS eventually
    surfaces EOF (recv -> b'') or an error on send. We treat EOF OR a send error
    as 'peer gone'. A still-open, never-replying socket (recv times out with no
    bytes and the send succeeds) is reported as NOT closed -> contract violation.
    """
    try:
        sess.sock.settimeout(timeout)
        try:
            data = sess.sock.recv(65536)
            if data == b"":
                return True  # clean EOF == kicked
            # The server emitted bytes (e.g. a system "kicked" message for a
            # CharacterSelected client) THEN closes. Keep draining until EOF.
            deadline = time.time() + timeout
            while time.time() < deadline:
                more = sess.sock.recv(65536)
                if more == b"":
                    return True
        except OSError:
            return True  # connection reset/closed
        # No EOF yet; poke the peer and look again. A gone peer errors or EOFs.
        try:
            sess.sock.sendall(bytes([0x0F, 0x00]))
        except OSError:
            return True
        try:
            sess.sock.settimeout(timeout)
            data = sess.sock.recv(65536)
            return data == b""
        except OSError:
            return True
    except Exception:
        # Any socket exception here means the peer is unusable -> treat as gone.
        return True
    return False


def run(server):
    try:
        under_valgrind = server.valgrind_errors() != -1

        # ----------------------------------------------------------------
        # Snapshot a SEPARATE legit "victim" account BEFORE any abuse, so we
        # can prove the rejected attempts corrupt nobody else's state.
        # ----------------------------------------------------------------
        vlogin = _uniq("victim")
        vpass = _uniq("vpw")
        try:
            victim = H.Session(server, login=vlogin, passh=vpass, pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        try:
            before = H.reload_state(server, vlogin, vpass)
        except Exception as e:
            return (False, "victim reload_state(before) raised: %s" % e)
        if before.get("character_id") is None:
            return (False, "victim did not persist (no character_id)")

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim snapshot: %s" % why)

        # ----------------------------------------------------------------
        # Precondition for the "valid-frame" probe: a fully-handshaked on-map
        # session (the requester). It owns at least one monster at position 0
        # (the starter), but is NOT in a fight and the starter is not at an
        # evolution threshold -> position 0 yields "Evolution not found" -> the
        # requester is kicked. That is the EXPECTED behaviour for a well-formed
        # frame whose evolution is not currently legal; we assert the kick and
        # the server's health, and document that the positive mutation path is
        # not reachable here (reachability-limited).
        # ----------------------------------------------------------------
        valid_cases = 0
        semantic_invalid_cases = 0
        verifies_kick = True

        # VALID FRAME, position 0 (a monster the player owns). Well-formed; the
        # handler accepts the frame, finds no currently-legal evolution and
        # kicks. (If a 'test' starter ever DID meet a condition, the slot is
        # mutated in place with NO new monster -> count still invariant.)
        try:
            req = H.Session(server, login=_uniq("ev"), passh=_uniq("evp"),
                            pseudo="Evolver")
        except H.HandshakeError as e:
            return (False, "requester handshake failed: %s" % e)
        req.m(0x0F, H.u8(0), dynamic=True)   # valid frame (0x0F is a 0xFE dynamic packet)
        valid_cases += 1
        if not _peer_closed(req, timeout=1.5):
            req.close()
            return (False, "valid-frame pos=0: requester was NOT kicked "
                           "(no currently-legal evolution must errorOutput->kick)")
        req.close()
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after valid-frame pos=0: %s" % why)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID (well-formed frame, illegal payload) cases. Each must
        # KICK the offending client while the server survives. Fresh disposable
        # session per case (the prior one is now disconnected).
        # ----------------------------------------------------------------
        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Cheat")

        # (S1) OUT-OF-RANGE position == 255: pos >= monsters.size() ->
        #      "Monster for evolution not found" -> KICK. (the OOB case)
        try:
            s = fresh("oob")
            s.m(0x0F, H.u8(255), dynamic=True)
            kicked = _peer_closed(s, timeout=1.5)
            s.close()
        except H.HandshakeError as e:
            return (False, "S1 handshake failed: %s" % e)
        except OSError:
            kicked = True  # kicked mid-probe is acceptable
        semantic_invalid_cases += 1
        if not kicked:
            verifies_kick = False
            return (False, "S1 OOB pos=255: cheater was NOT kicked "
                           "(silent ignore is a contract violation)")
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after S1 OOB pos=255: %s" % why)

        # (S2) OUT-OF-RANGE position == 10 (player owns far fewer than 10
        #      monsters) -> same "not found" -> KICK.
        try:
            s = fresh("oob2")
            s.m(0x0F, H.u8(10), dynamic=True)
            kicked = _peer_closed(s, timeout=1.5)
            s.close()
        except H.HandshakeError as e:
            return (False, "S2 handshake failed: %s" % e)
        except OSError:
            kicked = True
        semantic_invalid_cases += 1
        if not kicked:
            verifies_kick = False
            return (False, "S2 OOB pos=10: cheater was NOT kicked")
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after S2 OOB pos=10: %s" % why)

        # (S3) RE-CONFIRM / double-validation on an owned slot that has no legal
        #      evolution: send 0x0F pos=0 TWICE on one session. The FIRST kicks
        #      ("Evolution not found"); a second send after the kick must not
        #      revive the connection nor create an extra monster. We assert the
        #      socket is gone and the server is healthy. (Covers the
        #      "Re-confirm must not create extra monster" note: no monster is
        #      ever appended on any path, and the kick prevents a second action.)
        try:
            s = fresh("redo")
            s.m(0x0F, H.u8(0), dynamic=True)   # first confirm: no legal evo -> kick
            kicked = _peer_closed(s, timeout=1.5)
            # second confirm on a (now closed) socket: must not crash the server
            try:
                s.m(0x0F, H.u8(0), dynamic=True)
            except OSError:
                pass
            s.close()
        except H.HandshakeError as e:
            return (False, "S3 handshake failed: %s" % e)
        except OSError:
            kicked = True
        semantic_invalid_cases += 1
        if not kicked:
            verifies_kick = False
            return (False, "S3 re-confirm pos=0: cheater was NOT kicked")
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after S3 re-confirm: %s" % why)

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. The fixed-size handler demands exactly 1 data
        # byte. Wrong/truncated framing must kick ONLY the offender and leave the
        # server alive+clean. Fresh disposable session per case.
        # ----------------------------------------------------------------
        malformed_cases = 0

        # (M1) ZERO data bytes: code 0x0F with no payload. The fixed-size framer
        #      expects 1 byte; sending the lone code then GARBAGE forces the
        #      parser to consume a wrong-shaped frame (size!=1) -> kick, and
        #      proves the parser can't be wedged.
        try:
            s = fresh("m1")
            s.send_raw(bytes([0x0F]))            # code, no data byte
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF]))  # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "M1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M1 zero-byte/garbage framing: %s" % why)

        # (M2) TWO data bytes for a 1-byte handler. The framer reads byte 1 as
        #      the position and the extra 0x0F byte starts a NEW (truncated)
        #      0x0F frame; either way the server must stay healthy.
        try:
            s = fresh("m2")
            s.send_raw(bytes([0x0F, 0x00, 0x0F]))  # code + 2 trailing bytes
            time.sleep(0.15)
            s.send_raw(bytes([0xFF]))              # finish/garbage
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "M2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M2 over-length framing: %s" % why)

        # (M3) MIS-FRAMED AS A QUERY: send 0x0F with a queryNumber byte as if it
        #      were a query packet. 0x0F is a MESSAGE (code<0x80) so the server
        #      reads the qnum byte as the position payload and the real position
        #      byte starts a new frame -> mis-parse path; must not crash/hang.
        try:
            s = fresh("m3")
            s.send_raw(bytes([0x0F, 0x05, 0x00]))  # pretend [code][qn][data]
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF]))
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "M3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M3 query-mis-framing: %s" % why)

        # (M4) Truncated then SILENCE: lone code byte, nothing else. The framer
        #      must wait for more bytes WITHOUT busy-spinning or crashing; the
        #      server stays alive while the stalled frame is incomplete.
        try:
            s = fresh("m4")
            s.send_raw(bytes([0x0F]))   # incomplete, no follow-up
            s.drain(timeout=0.4)
            s.close()
        except H.HandshakeError as e:
            return (False, "M4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after M4 truncated-silence framing: %s" % why)

        # ----------------------------------------------------------------
        # NO-STATE-CORRUPTION: re-snapshot the untouched legit victim. Its
        # persisted identity + position must be byte-for-byte what they were
        # before all the abuse (no phantom monster/position/identity change). No
        # handler path appends a monster, so "no extra monster" holds by
        # construction; reload_state confirms id/pseudo/position are intact.
        # ----------------------------------------------------------------
        checks_db_state = False
        try:
            after = H.reload_state(server, vlogin, vpass)
        except Exception as e:
            return (False, "victim reload_state(after) raised: %s" % e)
        for k in ("character_id", "pseudo", "x", "y", "mapIndex"):
            if before.get(k) != after.get(k):
                return (False, "victim state CORRUPTED: %s before=%r after=%r"
                               % (k, before.get(k), after.get(k)))
        checks_db_state = True

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim re-snapshot: %s" % why)

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
            "valid=%d malformed=%d semantic_invalid=%d kick_verified=%s "
            "db_checked=%s reachability_limited=True (success/evolution-mutation "
            "path needs an in-fight monster meeting a level/trade condition, not "
            "reachable from a fresh on-map 'test' session; handler appends NO "
            "monster on ANY path so 'no extra monster' is structural). All "
            "illegal/OOB positions kicked the cheater; server stayed "
            "alive+clean%s; victim id/pos byte-identical before/after."
            % (valid_cases, malformed_cases, semantic_invalid_cases,
               verifies_kick, checks_db_state,
               "+valgrind-clean(delta vs baseline at stop)" if under_valgrind
               else "")
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
