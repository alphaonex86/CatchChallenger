"""Valgrind/protocol test for server handler 0x11 (useSkill).

Handler: server/base/ClientNetworkReadMessage.cpp:471 (Client::parseMessage).
Wire form: FIXED-size MESSAGE, code 0x11, exactly 2 data bytes:
    [skill:u16]
No queryNumber (message, code<0x80), no 4-byte length (fixed size 2).

Handler logic traced (server/base/ClientNetworkReadMessage.cpp:471 ->
general/base/fight/CommonFightEngineSkill.cpp:137 CommonFightEngine::useSkill):

  * parseMessage: size!=sizeof(uint16_t) (i.e. != 2)
        -> Client::errorOutput("Wrong size in move packet") -> KICK.
  * useSkill(skill):
      - !isInFight()        -> errorFightEngine("Try use skill when not in
                               fight") -> Client::errorFightEngine ->
                               Client::errorOutput -> KICK.   <-- dominant path
      - currentMonster==NULL -> errorFightEngine -> KICK.
      - currentMonsterIsKO() -> errorFightEngine("Can't attack with KO
                               monster") -> KICK.
      - otherMonster==NULL   -> errorFightEngine -> KICK.
      - getTheSkill(skill)==NULL (skill not owned / no endurance) and NOT the
        (skill==0 && no-endurance default-attack) special case
                             -> errorFightEngine("...have not the skill ...")
                             -> KICK.
      - skill owned but endurance==0
                             -> errorFightEngine("Try use skill without
                               endurance") -> KICK.
      - else doTheTurn(); returns true.
    Client::errorFightEngine -> Client::errorOutput -> disconnectClient()
    (server/base/Client.cpp:622,524). So EVERY rejection of useSkill KICKS the
    offending client; the server stays alive (production-safe parse path under
    HARDENED-only kicks, does not abort — see foundation api_notes).

REACHABILITY (documented): an out-of-the-box on-map Session in the 'test'
maincode is NOT in a fight (isInFight()==False). Entering a real fight needs a
wild-encounter walk onto grass or a PvP battle handshake that is not
deterministically reachable from this harness. THEREFORE every WELL-FORMED
0x11 packet we can send from a reachable session is semantically ILLEGAL
(use-skill-when-not-in-fight) and MUST be answered with a KICK. We exploit
exactly that: useSkill's first guard is !isInFight(), so an unknown skill id, a
not-owned skill id and skill 0 all funnel through the same kick — each is a
distinct semantic-invalid input whose contract is "cheater kicked, server
survives, no state corruption". This is marked reachability_limited=True
because the in-fight VALID branch (doTheTurn succeeds, no kick) cannot be
exercised here; we assert the not-in-fight refusal+kick instead, which is the
security-relevant contract.

Verification strategy:
  * useSkill is a MESSAGE (no reply to the sender). We confirm the contract by
    detecting that the offending socket is CLOSED by the server (EOF) after the
    illegal send -> the cheater was kicked. A silent ignore (socket stays open)
    would be a contract violation.
  * After EACH send we assert server.alive() and crash_report() is None.
  * NO STATE CORRUPTION is checked with a SEPARATE legit account: its persisted
    identity/position (H.reload_state) is identical before and after all the
    abuse, and a fresh session still handshakes -> the server is unharmed.
"""

import time
import _protoharness as H

NAME = "0x11 useSkill"


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


def _is_kicked(sess, settle=0.4):
    """Return True iff the server has CLOSED this session's socket (the client
    was kicked). Detection: read from the raw socket; the server's
    disconnectClient() shuts the connection, so recv() returns b'' (EOF). We
    also poke the peer once more to force the FIN to be observed even if the
    first recv only drained buffered bytes. A still-open socket (timeout, no
    EOF, send succeeds) means NOT kicked.
    """
    sk = sess.sock
    deadline = time.time() + settle
    # 1) drain whatever is buffered and look for EOF.
    while time.time() < deadline:
        try:
            sk.settimeout(0.2)
            d = sk.recv(65536)
            if d == b"":
                return True  # clean EOF -> server closed us -> kicked
            # bytes arrived (e.g. the system "kicked" message just before FIN);
            # keep reading until EOF or timeout.
        except OSError as e:
            # ECONNRESET / EBADF etc. -> peer gone -> kicked.
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                break
            return True
    # 2) Force the issue: a write to a half-closed peer eventually errors, and a
    # follow-up recv on a server-closed socket returns EOF.
    try:
        sk.sendall(bytes([0x11, 0x00, 0x00]))  # another (illegal) useSkill
    except OSError:
        return True  # send failed -> peer gone -> kicked
    try:
        sk.settimeout(0.4)
        d = sk.recv(65536)
        if d == b"":
            return True
        # Still getting data and no EOF -> read once more for the FIN.
        try:
            d2 = sk.recv(65536)
            if d2 == b"":
                return True
        except OSError as e:
            if not (isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout"):
                return True
    except OSError as e:
        if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
            return False  # socket still open, server kept us -> NOT kicked
        return True       # reset -> kicked
    return False


def run(server):
    try:
        # ----------------------------------------------------------------
        # Baseline legit account whose persisted state must stay intact
        # through all the abuse. Create it, record its persisted snapshot,
        # then close it so reload_state can read it back cleanly.
        # ----------------------------------------------------------------
        victim_login = _uniq("vic")
        victim_pass = _uniq("vpw")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)

        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        checks_db = False
        try:
            before = H.reload_state(server, victim_login, victim_pass)
            if before.get("character_id") is None:
                return (False, "victim did not persist (no character_id)")
            checks_db = True
        except Exception as e:
            return (False, "reload_state(before) raised: %s" % e)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID well-formed useSkill packets. From an on-map (NOT
        # in fight) session every one of these is illegal: useSkill's first
        # guard is !isInFight() -> errorFightEngine -> errorOutput -> KICK.
        # Each is a DISTINCT illegal input; for EACH we assert:
        #   (a) the offending client is KICKED (socket closed by server),
        #   (b) the SERVER survives (alive + no crash),
        #   (c) it leaves no corrupt state (checked once at the end).
        # Each uses a FRESH disposable session because it gets kicked.
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Cheat")

        # skill ids to try, all WELL-FRAMED (exactly 2 bytes):
        #   0      -> the default-attack id; but with no endurance/not-in-fight
        #             it still hits the not-in-fight guard first -> kick.
        #   1      -> a low id that the freshly-spawned monster does not own
        #             (not-owned skill).
        #   0xFFFF -> an id that does not exist in the datapack at all
        #             (unknown skill).
        for skill_id, label in ((0x0000, "skill=0 (default-attack, not in fight)"),
                                 (0x0001, "skill=1 (not owned)"),
                                 (0xFFFF, "skill=0xFFFF (unknown id)")):
            try:
                s = fresh("cs")
            except H.HandshakeError as e:
                return (False, "semantic#%d handshake failed: %s"
                        % (semantic_invalid_cases + 1, e))
            try:
                s.m(0x11, H.u16(skill_id))  # well-formed useSkill, semantically illegal
            except OSError:
                pass  # kicked mid-send is acceptable evidence
            kicked = _is_kicked(s)
            try:
                s.close()
            except OSError:
                pass
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(useSkill out of fight must disconnect the cheater)"
                        % label)
            semantic_invalid_cases += 1

        # VALID-case note: a successful in-fight useSkill (doTheTurn, no kick)
        # is not deterministically reachable from this harness (needs a real
        # wild/PvP fight). We assert the security-relevant refusal+kick above
        # for every well-formed packet instead; reachability_limited=True.
        valid_cases = 0  # in-fight success branch unreachable here

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. The fixed-size handler wants exactly 2 data
        # bytes; a wrong length is rejected by the size!=2 guard -> kick. Bad
        # wire framing must never crash/hang the server. Each from a fresh
        # disposable session; after each assert alive+clean.
        # ----------------------------------------------------------------
        malformed_cases = 0

        # (1) ONE trailing data byte (size 1, < 2). The fixed-size framer waits
        # for the 2nd byte; we follow with garbage so the parser can't wedge,
        # and the server must stay alive.
        try:
            s = fresh("m1")
            s.send_raw(bytes([0x11, 0x07]))             # code + ONE data byte
            time.sleep(0.15)
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
            return (False, "after truncated 1-byte useSkill: %s" % why)

        # (2) THREE data bytes (size 3, > 2). The framer reads code 0x11 + 2
        # bytes as the packet, leaving 1 stray byte to mis-frame the next
        # packet -> the server should reject/kick, never crash. Follow with a
        # benign drain.
        try:
            s = fresh("m2")
            s.send_raw(bytes([0x11, 0x01, 0x00, 0x00]))  # code + 3 data bytes
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long 3-byte useSkill: %s" % why)

        # (3) code only, ZERO data bytes. The framer blocks waiting for the 2
        # fixed bytes; we send garbage to ensure no wedge, server stays alive.
        try:
            s = fresh("m3")
            s.send_raw(bytes([0x11]))                    # bare code, no data
            time.sleep(0.15)
            s.send_raw(bytes([0xAA, 0xBB]))              # complete then trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare-code useSkill: %s" % why)

        # (4) MESSAGE code 0x11 sent WITH a spurious 4-byte dynamic length (as
        # if it were a dynamic packet). 0x11 is FIXED, so the would-be length
        # bytes are mis-read as data+next-packet -> the parser must reject/kick
        # and the server must survive.
        try:
            s = fresh("m4")
            s.send_raw(bytes([0x11]) + H.u32(2) + H.u16(0))  # bogus len prefix
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus-dynamic-length useSkill: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim account's persisted identity/position
        # must be byte-for-byte what it was before any of the abuse, and a
        # brand-new full session must still reach CharacterSelected.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "reload_state(after) raised: %s" % e)

        if after.get("character_id") != before.get("character_id"):
            return (False, "victim character_id changed: %r -> %r"
                    % (before.get("character_id"), after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo"):
            if after.get(k) != before.get(k):
                return (False,
                        "victim state corrupted: %s %r -> %r"
                        % (k, before.get(k), after.get(k)))

        # Leave the server usable: a fresh full session still handshakes.
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
            "valid=%d (in-fight success branch unreachable here) "
            "semantic_invalid=%d (all kicked: not-in-fight + not-owned + "
            "unknown skill id) malformed=%d db_checked=%s victim_persisted=%s "
            "(server stayed alive+clean; no state corruption)"
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               checks_db, (before.get("x"), before.get("y"),
                           before.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
