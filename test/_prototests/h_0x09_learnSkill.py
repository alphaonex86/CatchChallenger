"""Valgrind/protocol test for server handler 0x09 (learnSkill).

Handler: server/base/ClientNetworkReadMessage.cpp:377 (Client::parseMessage).
Wire form: FIXED-size MESSAGE, code 0x09, exactly 3 data bytes:
    [monsterPosition:u8][skill:u16 LE]
No queryNumber (message, code<0x80), no 4-byte length (fixed size 3).

Handler logic traced (server/base/ClientNetworkReadMessage.cpp:377 ->
Client::learnSkillByMonsterPosition (LocalClientHandlerFightManage.cpp:8) ->
Client::learnSkillInternal (server/base/fight/LocalClientHandlerFightSkill.cpp:10)):

  * size != 3  (sizeof(u8)+sizeof(u16))
        -> errorOutput("wrong remaining size for learn skill") -> return false.
  * monsterPosition >= monsters.size()
        -> errorOutput("The monster is not found: ...") -> return false
           (guarded BOTH in learnSkillByMonsterPosition and learnSkillInternal).
  * skill the monster cannot learn / level-up entry the monster does not own:
        this is the JUST-FIXED out_of_range crash class.  In
        learnSkillInternal, sub_index2 == skills.size() when the monster does
        not yet own `skill`; the old code did monster.skills.at(sub_index2)
        unconditionally for a level-up learn entry (learnSkillLevel!=1) and
        threw std::out_of_range -> SIGABRT/crash.  The fix guards the .at()
        with `sub_index2 < skills.size()`.  A well-formed-but-illegal learn
        (unknown skill, not-learnable skill, level-up entry not owned) must now
        return false via errorOutput("...not into learn skill list...") and
        NEVER crash.
  * On the SERVER every `errorOutput(...)` -> disconnectClient() == KICK; the
    handler returns false so the parser ALSO sees false.  Under HARDENED-only
    (no CATCHCHALLENGER_ABORTIFERROR) a handler returning false KICKS the
    offending client; the server process stays alive (production-safe).

Reachability limitation (documented, honest):
  The `test` maincode start.xml (map/main/test/start.xml) gives a brand-new
  character ZERO monsters and ZERO cash.  There is NO in-protocol way to grant
  a monster to a fresh account in this maincode, so monsters.size() is always 0
  for a harness-created character.  Consequently EVERY well-formed 0x09 packet
  (any monsterPosition, any skill) takes the `monsterPosition >= 0` ==
  out-of-range path and is KICKED before learnSkillInternal's skill loop runs.

  This means we CANNOT drive a *successful* skill-learn (no monster exists) nor
  step the inner skills.at() crash branch directly from the wire.  We therefore:
    - exercise the handler's reachable error/security surface fully: framing
      size guards, the out-of-range monsterPosition guard, and the
      well-formed-but-semantically-illegal (skill the player can't learn on a
      position it doesn't own) inputs that are the wire-level expression of the
      just-fixed crash class;
    - assert for EVERY such input: server stays ALIVE + crash_report() is None
      (the crash class does NOT fire), the cheating client is KICKED, and NO
      state is corrupted for a separate legitimate account;
    - mark the run reachability-limited so the synthesis records that the
      *positive* learn and the inner .at() branch are not reachable in `test`.

Verification strategy:
  * learnSkill is a MESSAGE -> the mover gets NO 0x7F reply on the happy path;
    on the error path the server KICKS.  So our observable signal is: KICK
    (socket EOF) on illegal input, server.alive()+clean throughout, and a
    separate legit account's persisted state unchanged.
  * Each illegal/malformed send uses a DISPOSABLE session (it gets kicked).
  * No-state-corruption is checked with a separate, untouched legit account via
    H.reload_state (FILE_DB persists the whole character block at disconnect).
"""

import time
import socket
import _protoharness as H

NAME = "0x09 learnSkill"


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
    """Return True iff the peer has closed our socket (the client was KICKED).

    Detection: drain whatever is pending (also flushes the
    'have been kicked'/disconnect bytes), then probe the socket. A kicked
    connection reaches EOF -> recv() returns b'' (or the socket errors). We
    also push a follow-up byte; on a half-closed/closed peer the subsequent
    recv reliably reports EOF. A still-open connection yields neither EOF nor
    error within the short window."""
    try:
        sess.drain(timeout=0.4)
    except OSError:
        return True
    # Probe for EOF: a kicked peer's recv returns b'' promptly.
    try:
        sess.sock.settimeout(0.6)
        d = sess.sock.recv(4096)
        if d == b"":
            return True  # clean EOF -> kicked
        # Got bytes (e.g. a trailing kick/system message): drain more then
        # re-probe; the connection should still close.
        try:
            sess.sock.settimeout(0.6)
            d2 = sess.sock.recv(4096)
            if d2 == b"":
                return True
        except socket.timeout:
            pass
        except OSError:
            return True
        # Still readable/open after pumping: follow-up write+read to force EOF.
        try:
            sess.sock.sendall(bytes([0x0D, 0x06, 0x06]))  # benign move frame
            sess.sock.settimeout(0.6)
            d3 = sess.sock.recv(4096)
            return d3 == b""
        except OSError:
            return True
        except socket.timeout:
            return False
    except socket.timeout:
        # No EOF surfaced within the window: try to force it with a write.
        try:
            sess.sock.sendall(bytes([0x0D, 0x06, 0x06]))
            sess.sock.settimeout(0.6)
            d = sess.sock.recv(4096)
            return d == b""
        except OSError:
            return True
        except socket.timeout:
            return False
    except OSError:
        return True


def run(server):
    try:
        # ----------------------------------------------------------------
        # Baseline legit account, left untouched, used to prove the illegal
        # learnSkill attempts corrupt NO server/DB state for other players.
        # ----------------------------------------------------------------
        victim_login = _uniq("ls_victim")
        victim_pass = _uniq("ls_vpw")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="LsVictim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)

        victim_start = (victim.x, victim.y, victim.mapIndex, victim.character_id)
        # Snapshot the victim's persisted state BEFORE any abuse.
        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        try:
            before_snap = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "victim reload_state(before) raised: %s" % e)
        if before_snap.get("character_id") is None:
            return (False, "victim did not persist (no character_id)")

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim baseline snapshot: %s" % why)

        # ----------------------------------------------------------------
        # VALID-FRAMING case (reachability-limited).
        # The `test` maincode gives a fresh character ZERO monsters, so the
        # ONLY well-formed 0x09 outcome reachable from the wire is the
        # out-of-range monsterPosition guard -> KICK. We send a perfectly
        # well-framed packet (monsterPosition=0, a real datapack skill id) and
        # assert the server does NOT crash (the just-fixed out_of_range class
        # does NOT fire) and the offending client is kicked gracefully.
        # We count this as the (reachability-limited) valid-framing case.
        # ----------------------------------------------------------------
        valid_cases = 0
        try:
            s = H.Session(server, login=_uniq("ls_v"), passh=_uniq("ls_vp"),
                          pseudo="LsValid")
        except H.HandshakeError as e:
            return (False, "valid-case handshake failed: %s" % e)
        # monsterPosition=0, skill=20 (a real skill id present in the datapack).
        s.m(0x09, H.u8(0) + H.u16(20))
        valid_cases += 1
        kicked = _is_kicked(s)
        s.close()
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after well-framed 0x09 (no-monster): %s" % why)
        if not kicked:
            # In `test` a fresh char has no monster, so this MUST be rejected
            # (errorOutput -> kick). A silent accept would be a contract break.
            return (False, "well-framed 0x09 on monster-less char was NOT "
                           "kicked (out-of-range guard silently ignored)")

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. Each from a FRESH disposable session; after
        # each, assert the server is still alive+clean (no crash, no abort, no
        # hang). Framing errors hit the size guard (size!=3) -> errorOutput ->
        # kick, OR stall the fixed-size framer (must not wedge the parser).
        # ----------------------------------------------------------------
        malformed_cases = 0

        def fresh(tag):
            return H.Session(server, login=_uniq("ls_" + tag),
                             passh=_uniq("lp_" + tag), pseudo="LsBad")

        # (1) TOO SHORT: code + only 1 data byte (need 3). The fixed-size
        # framer expects 3; a lone byte either stalls (waiting for 2 more) or
        # is rejected. We push a garbage trailer to ensure the parser can't be
        # wedged, then assert liveness.
        try:
            s = fresh("short")
            s.send_raw(bytes([0x09, 0x00]))            # code + 1 byte only
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))  # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass  # kicked-mid-send is acceptable
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated (2-byte) frame: %s" % why)

        # (2) TOO SHORT: code + 2 data bytes (need 3) then garbage.
        try:
            s = fresh("short2")
            s.send_raw(bytes([0x09, 0x00, 0x14]))      # code + 2 bytes
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated (3-byte) frame: %s" % why)

        # (3) Send 0x09 as a DYNAMIC packet (4-byte length prefix) even though
        # it is FIXED. The framer reads the next 3 bytes as the payload, so the
        # length prefix is mis-parsed as data; this is a deliberately malformed
        # frame. Must not crash; the offending client is dropped.
        try:
            s = fresh("dyn")
            # send_raw a 0x09 with a bogus 4-byte LE length then 3 data bytes.
            s.send_raw(bytes([0x09]) + H.u32(3) + H.u8(0) + H.u16(20))
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus dynamic-framed 0x09: %s" % why)

        # (4) TOO LONG: code + 4 data bytes (need 3). The 4th byte is consumed
        # by the framer as the next packet's code (0xFF -> unknown/blocked).
        # Must not crash; parser must recover or kick.
        try:
            s = fresh("long")
            s.send_raw(bytes([0x09, 0x00, 0x14, 0x00, 0xFF]))
            time.sleep(0.15)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long (4-byte) frame: %s" % why)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID cases — THE KEY CONTRACT. Well-formed 3-byte 0x09
        # packets that are semantically illegal for THIS handler. In `test`
        # the character owns NO monster, so EVERY monsterPosition is
        # out-of-range and EVERY skill is unlearnable -> these are exactly the
        # wire-level expression of the just-fixed crash class. For EACH:
        #   (a) KICK: the offending client is disconnected (errorOutput ->
        #       disconnectClient). We verify via socket EOF (_is_kicked).
        #   (b) SERVER SURVIVES: alive() True, crash_report() None (the
        #       out_of_range crash does NOT fire), valgrind stays clean.
        #   (c) NO STATE CORRUPTION: re-checked at the end against the untouched
        #       victim account.
        # Each from a FRESH disposable session (it gets kicked).
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0

        # Cases: (monsterPosition, skill, label). All are illegal because the
        # fresh char has no monster (position out of range) and/or the skill is
        # unknown/unlearnable. The crash class lived in the skill loop reached
        # only with a monster; we still assert NO crash for these wire inputs.
        sem = [
            (0,    20,     "pos0 owned-no-monster, real skill"),
            (0,    0xFFFF, "pos0, UNKNOWN skill 65535"),
            (0,    0,      "pos0, skill 0 (none)"),
            (200,  20,     "out-of-range monsterPosition 200"),
            (255,  0xFFFF, "max position 255 + unknown skill (overflow probe)"),
        ]
        for pos, skill, label in sem:
            try:
                s = fresh("sem%d_%d" % (pos, skill & 0xFFFF))
            except H.HandshakeError as e:
                return (False, "semantic[%s] handshake failed: %s" % (label, e))
            try:
                s.m(0x09, H.u8(pos) + H.u16(skill))
            except OSError:
                # Some stacks may have already torn down mid-send; that is a
                # kick. Continue to the liveness/kick assertions.
                pass
            # (b) server must survive each illegal send (crash class must NOT
            # fire: this is the explicit requirement).
            ok, why = _alive_clean(server)
            if not ok:
                try:
                    s.close()
                except OSError:
                    pass
                return (False, "after semantic-illegal [%s]: %s (CRASH CLASS "
                               "may have fired)" % (label, why))
            # (a) the cheating client must be KICKED.
            kicked = _is_kicked(s)
            try:
                s.close()
            except OSError:
                pass
            if not kicked:
                return (False, "semantic-illegal [%s] did NOT kick the client "
                               "(silent ignore is a contract violation)" % label)
            semantic_invalid_cases += 1
            # re-confirm clean after the kick handling too.
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after kick for [%s]: %s" % (label, why))

        # ----------------------------------------------------------------
        # (c) NO STATE CORRUPTION: the untouched victim account must be exactly
        # as it was before any abuse — no phantom monster/skill/position change.
        # ----------------------------------------------------------------
        checks_db = False
        try:
            after_snap = H.reload_state(server, victim_login, victim_pass)
            checks_db = True
        except Exception as e:
            return (False, "victim reload_state(after) raised: %s" % e)

        # Compare the reliably-framed identity/position header. cash/items/
        # monsters live deeper in the HPS block (FOUNDATION reads id+pos); for
        # THIS handler the only persisted thing it could ever touch is a
        # monster's skill list, and the victim has no monster, so an
        # identity+position match (plus server-clean) is the observable proof
        # no cross-account corruption occurred.
        for k in ("character_id", "x", "y", "mapIndex"):
            if before_snap.get(k) != after_snap.get(k):
                return (False, "victim state CORRUPTED: %s changed %r -> %r"
                               % (k, before_snap.get(k), after_snap.get(k)))

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim corruption check: %s" % why)

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session still handshakes
        # to CharacterSelected after all the abuse.
        # ----------------------------------------------------------------
        try:
            again = H.Session(server, login=_uniq("ls_ok"),
                              passh=_uniq("ls_okp"), pseudo="LsAfter")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d malformed=%d semantic_invalid=%d db_checked=%s "
            "reachability-limited (test maincode start.xml gives a fresh char "
            "ZERO monsters -> every well-formed 0x09 hits the out-of-range "
            "monsterPosition guard; the positive learn and the inner "
            "skills.at() branch are unreachable from the wire). All illegal "
            "inputs KICKED the cheater; server stayed ALIVE+clean (the "
            "just-fixed out_of_range crash class did NOT fire); victim "
            "id/pos=%s unchanged."
            % (valid_cases, malformed_cases, semantic_invalid_cases, checks_db,
               (before_snap.get("character_id"), before_snap.get("x"),
                before_snap.get("y"), before_snap.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
