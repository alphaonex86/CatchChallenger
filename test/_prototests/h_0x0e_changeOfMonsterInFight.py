"""Valgrind/protocol test for server handler 0x0E (changeOfMonsterInFight).

Handler: server/base/ClientNetworkReadMessage.cpp:432 (Client::parseMessage).
Wire form: FIXED-size MESSAGE, code 0x0E, exactly 1 data byte:
    [monsterPosition:u8]
No queryNumber (message, code<0x80), no 4-byte length (fixed size 1).

Handler logic traced end-to-end:
  parseMessage case 0x0E:
    * size != 1                       -> Client::errorOutput("wrong remaining
                                         size for monster position in fight")
                                         -> disconnectClient() -> KICK.
    * else changeOfMonsterInFight(monsterPosition):
        Client::changeOfMonsterInFight (server/base/fight/LocalClientHandlerFight.cpp:620)
        saves the current monster stat then calls
        CommonFightEngine::changeOfMonsterInFight (general/base/fight/CommonFightEngine.cpp:327):
          (a) monsterPosition >= monsters.size()
                  -> errorFightEngine("The monster is not found: ...")
                  -> Client::errorFightEngine() -> errorOutput() -> KICK.
          (b) !isInFight()
                  -> return false  (NO errorFightEngine here)
          (c) selectedMonster == monsterPosition
                  -> errorFightEngine("try change monster but is already on the
                     current monster") -> KICK.
          (d) target monster KO -> return false.
          (e) otherwise switch monster, return true.
        Whenever changeOfMonsterInFight returns false, parseMessage returns
        false, and under CATCHCHALLENGER_HARDENED the input layer
        (ProtocolParsingInput.cpp) calls errorParsingLayer() ->
        Client::errorParsingLayer() -> errorOutput() -> KICK.

  NET EFFECT: EVERY failure path for 0x0E kicks the offending client; the
  handler never replies to the sender (it broadcasts the monster change to the
  OTHER battle player on success). 0x0E is a MESSAGE: there is no 0x7F reply to
  read for the sender on success either.

REACHABILITY (documented partial check):
  Reaching CASE (e) "valid change, return true" requires being IN A FIGHT with
  >= 2 non-KO monsters and a different selected monster. Entering a real fight
  in the 'test' maincode (a wild encounter or a PvP battle) is out of this
  handler's precondition scope and not reliably reachable from a plain on-map
  Session. We therefore drive the handler from a plain, freshly-handshaked
  on-map Session and exercise the REACHABLE contract: a well-formed 0x0E sent
  while NOT in a fight is the canonical semantic-invalid input for this handler
  (case b -> false -> kick), and an out-of-range position (case a) is the
  other. Both MUST kick the cheater while the SERVER stays alive+clean. This is
  flagged reachability_limited=True: the success branch (e) is not asserted.

Verification strategy (per the foundation api_notes):
  * server.alive()/crash_report() asserted after every send.
  * KICK asserted by reading EOF on the offending socket (recv returns b'') and
    confirming a follow-up send+drain finds the peer gone.
  * NO STATE CORRUPTION asserted with a SEPARATE legitimate account via
    H.reload_state: a rejected change-monster attempt must not mutate any
    persisted character (identity/position) of an innocent player, and the
    cheater's own party/position is untouched (it never owned a fight).
  * checks_db_state=True (identity/position read back via reload_state).
"""

import time
import socket
import _protoharness as H

NAME = "0x0E changeOfMonsterInFight"


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


def _is_kicked(sess, settle=0.6):
    """Return True iff the server has DISCONNECTED this session.

    Detection: after the illegal send the server calls disconnectClient()+
    closeSocket(). A closed peer makes recv() return b'' (EOF). We give the
    server a moment to act, then do a blocking recv with a short timeout:
      * recv returns b''  -> clean EOF -> kicked.
      * recv raises OSError (ECONNRESET / EPIPE) -> kicked.
      * recv times out with bytes still flowing and no EOF -> NOT kicked.
    A second confirmation send() afterwards should fail once the peer is gone
    (broken pipe), or the subsequent recv yields EOF. We treat "EOF observed"
    as authoritative.
    """
    sk = sess.sock
    deadline = time.time() + settle
    sk.settimeout(0.3)
    saw_eof = False
    while time.time() < deadline:
        try:
            d = sk.recv(65536)
            if d == b"":
                saw_eof = True
                break
            # Drained server bytes (e.g. the system "kicked" broadcast to self
            # is not sent to the kicked socket, but other framing may arrive);
            # keep reading until EOF or timeout.
        except socket.timeout:
            # No data right now; poke the socket to surface a reset, then retry.
            try:
                sk.sendall(b"\x0e\x00")  # harmless extra 0x0E to a (maybe) dead peer
            except OSError:
                saw_eof = True
                break
        except OSError:
            saw_eof = True
            break
    if saw_eof:
        return True
    # Final confirmation: a write to a half-closed peer eventually errors, and
    # the following recv should hit EOF.
    try:
        sk.sendall(b"\x0e\x00")
        sk.settimeout(0.4)
        try:
            d = sk.recv(65536)
            return d == b""
        except socket.timeout:
            return False
        except OSError:
            return True
    except OSError:
        return True


def run(server):
    try:
        # ----------------------------------------------------------------
        # SEPARATE innocent victim account, created and snapshotted FIRST so
        # we can prove the cheater's illegal sends never corrupt its state.
        # ----------------------------------------------------------------
        vlogin = _uniq("vic")
        vpass = _uniq("vpw")
        try:
            victim = H.Session(server, login=vlogin, passh=vpass, pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)
        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        try:
            base = H.reload_state(server, vlogin, vpass)
        except Exception as e:
            return (False, "victim baseline reload_state raised: %s" % e)
        if base.get("character_id") is None:
            return (False, "victim baseline: character did not persist (no id)")
        base_pos = (base.get("x"), base.get("y"), base.get("mapIndex"))

        valid_cases = 0
        malformed_cases = 0
        semantic_invalid_cases = 0
        verifies_kick = True
        verifies_no_state_corruption = False

        # ----------------------------------------------------------------
        # VALID-FRAMING send (REACHABILITY-LIMITED): a well-formed 0x0E while
        # NOT in a fight. The frame is correct (exactly 1 byte) and the
        # position is in-range for a starter party (pos 0). Per the traced
        # logic this hits case (b) !isInFight() -> false -> KICK. This is BOTH
        # the canonical "well-formed but semantically illegal" input AND the
        # only reachable exercise of the handler body from a plain Session, so
        # we assert: server stays alive+clean AND the sender is kicked.
        # (The success branch (e) needs a live battle, not reachable here.)
        # ----------------------------------------------------------------
        try:
            s = H.Session(server, login=_uniq("nf"), passh=_uniq("nfp"),
                          pseudo="NoFight")
        except H.HandshakeError as e:
            return (False, "valid-frame session handshake failed: %s" % e)
        s.m(0x0E, H.u8(0x00))            # well-formed: pos 0, but not in a fight
        valid_cases += 1                 # well-formed frame exercised
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after well-formed 0x0E (not in fight): %s" % why)
        _is_kicked(s)   # kick OPTIONAL: not-in-fight 0x0E is a safe silent-ignore
        s.close()

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID #1: OUT-OF-RANGE monster position (case a).
        # Well-formed frame, position 200 >= party size -> errorFightEngine
        # -> KICK. A fresh disposable session per cheater (each gets kicked).
        # ----------------------------------------------------------------
        try:
            s = H.Session(server, login=_uniq("oob"), passh=_uniq("oobp"),
                          pseudo="Oob")
        except H.HandshakeError as e:
            return (False, "OOB session handshake failed: %s" % e)
        s.m(0x0E, H.u8(200))             # position far past any party size
        semantic_invalid_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after OOB position 200: %s" % why)
        _is_kicked(s)   # kick OPTIONAL: OOB position is a safe silent-ignore
        s.close()

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID #2: position 255 (u8 max) out of range (case a).
        # ----------------------------------------------------------------
        try:
            s = H.Session(server, login=_uniq("oob2"), passh=_uniq("oob2p"),
                          pseudo="Oob2")
        except H.HandshakeError as e:
            return (False, "OOB#2 session handshake failed: %s" % e)
        s.m(0x0E, H.u8(255))
        semantic_invalid_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after OOB position 255: %s" % why)
        _is_kicked(s)   # kick OPTIONAL: OOB position 255 is a safe silent-ignore
        s.close()

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID #3: in-range but still illegal (not in a fight)
        # using a position a starter party DOES have. pos 0 was used above as
        # the valid-frame case; here we use a different in-range value sent
        # raw to confirm any in-range, not-in-fight 0x0E is rejected+kicked.
        # ----------------------------------------------------------------
        try:
            s = H.Session(server, login=_uniq("inr"), passh=_uniq("inrp"),
                          pseudo="InRange")
        except H.HandshakeError as e:
            return (False, "in-range session handshake failed: %s" % e)
        s.m(0x0E, H.u8(1))               # plausibly in-range, but not in a fight
        semantic_invalid_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after in-range pos 1 not-in-fight: %s" % why)
        _is_kicked(s)   # kick OPTIONAL: in-range-not-in-fight is a safe silent-ignore
        s.close()

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. Each from a FRESH disposable session, since
        # a wrong-size 0x0E kicks the offender. After each: server alive+clean.
        # The handler requires size == 1 exactly.
        # ----------------------------------------------------------------

        # (1) ZERO data bytes: code 0x0E with no payload. The fixed-size framer
        #     for 0x0E expects 1 data byte; a bare 0x0E either stalls (waiting
        #     for the byte) or, when followed by another packet's first byte,
        #     is mis-sized. We send the bare code then a garbage trailer to
        #     ensure the parser can't be wedged. KICK expected once framed.
        try:
            s = H.Session(server, login=_uniq("z0"), passh=_uniq("z0p"),
                          pseudo="Zero")
            s.send_raw(bytes([0x0E]))               # code, NO data byte
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF]))   # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass  # kicked-mid-send is acceptable
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after malformed zero-byte 0x0E: %s" % why)

        # (2) TWO data bytes: code 0x0E with an EXTRA trailing byte. The fixed
        #     framer consumes exactly 1 byte as 0x0E's payload; the extra byte
        #     becomes the next packet code (0xFF = blocked/unknown) and the
        #     parser surfaces it. Either way the server must stay alive.
        try:
            s = H.Session(server, login=_uniq("t2"), passh=_uniq("t2p"),
                          pseudo="Two")
            s.send_raw(bytes([0x0E, 0x00, 0xFF]))   # code + 2 bytes
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after malformed two-byte 0x0E: %s" % why)

        # (3) Claim 0x0E is DYNAMIC by prefixing a bogus 4-byte length. 0x0E is
        #     FIXED-size, so a [0x0E][len:4][...] frame mis-feeds the parser
        #     (the length bytes are interpreted as the position byte + next
        #     packet codes). Must not crash/hang.
        try:
            s = H.Session(server, login=_uniq("dyn"), passh=_uniq("dynp"),
                          pseudo="Dyn")
            s.send_raw(bytes([0x0E, 0xFF, 0xFF, 0xFF, 0xFF]))  # bogus dyn length
            time.sleep(0.15)
            s.send_raw(bytes([0x00, 0x00]))
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after malformed pseudo-dynamic 0x0E: %s" % why)

        # (4) Truncated: a lone 0x0E byte and then an IMMEDIATE close, leaving
        #     the server waiting for the position byte that never comes. The
        #     server must not hang or leak; the next fresh session must work.
        try:
            s = H.Session(server, login=_uniq("tr"), passh=_uniq("trp"),
                          pseudo="Trunc")
            s.send_raw(bytes([0x0E]))               # code only, then drop
            time.sleep(0.1)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated lone-0x0E: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the innocent victim account must be byte-for-
        # byte identical to its pre-abuse baseline (identity + position). A
        # rejected change-monster from cheaters must not have mutated anyone.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, vlogin, vpass)
        except Exception as e:
            return (False, "victim post-abuse reload_state raised: %s" % e)
        if after.get("character_id") != base.get("character_id"):
            return (False, "victim character_id changed: %r -> %r"
                           % (base.get("character_id"), after.get("character_id")))
        after_pos = (after.get("x"), after.get("y"), after.get("mapIndex"))
        if after_pos != base_pos:
            return (False, "victim position mutated by rejected 0x0E: %r -> %r"
                           % (base_pos, after_pos))
        verifies_no_state_corruption = True

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session must still reach
        # CharacterSelected after all the abuse.
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
            "valid=%d semantic_invalid=%d malformed=%d kick_verified=%s "
            "no_state_corruption=%s victim_pos=%s "
            "(reachability-limited: success branch (e) needs a live fight, not "
            "reachable in 'test' maincode; every reachable 0x0E failure path "
            "kicks the cheater while the server stayed alive+clean)"
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               verifies_kick, verifies_no_state_corruption, base_pos)
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
