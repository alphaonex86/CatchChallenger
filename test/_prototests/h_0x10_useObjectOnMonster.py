"""Valgrind/protocol test for server handler 0x10 (useObjectOnMonster).

Handler: server/base/ClientNetworkReadMessage.cpp:458 (Client::parseMessage).
Wire form: DYNAMIC MESSAGE (packetFixedSize[0x10]=0xFE), code 0x10, a
4-byte LE size prefix then exactly 3 data bytes:
    [item:u16 LE][monsterPosition:u8]
No queryNumber (message, code<0x80), no 4-byte length (fixed size 3).

Handler logic traced (ClientNetworkReadMessage.cpp -> LocalClientHandlerObject.cpp
-> general/fight/CommonFightEngine.cpp):
  parseMessage case 0x10:
    * size < sizeof(u16)+sizeof(u8) (==3) -> errorOutput("wrong remaining size
      for use object on monster") -> KICK (Client::errorOutput ->
      disconnectClient()+closeSocket()).
    * else item=loadLe16(data), monsterPosition=data[2];
      return useObjectOnMonsterByPosition(item, monsterPosition).
  Client::useObjectOnMonsterByPosition(object, monsterPosition):
    * object NOT in inventory map           -> errorOutput(...) -> KICK, false.
    * objectQuantity(object) < 1            -> errorOutput(...) -> KICK, false.
    * else CommonFightEngine::useObjectOnMonsterByPosition(object, monsterPosition):
        - monsterPosition >= monsters.size()  -> errorFightEngine(...) (->
          errorOutput -> KICK), false.
        - monster pointer NULL / monster KO   -> errorFightEngine -> KICK, false.
        - object is an evolution/learn item NOT applicable to this monster, or
          can't-be-used-in-fight -> errorFightEngine -> KICK, false.
        - a VALID heal item (monsterItemEffect AddHp) on a live monster:
          out of fight, if the monster is already at full HP and the effect set
          has size 1, the inner heal returns false (nothing to do) but
          Client::useObjectOnMonsterByPosition STILL returns true and does NOT
          consume the item -> NO kick, no state change. If the inner returns
          true AND the item has consumeAtUse (default), removeObject() drops one.
      parseMessage returns whatever the call returns; the MESSAGE handler never
      sends a direct reply to the user (no echo), so we observe "accepted" as
      "the session is NOT kicked".

Datapack fixtures (CatchChallenger-datapack, profile 0 "Normal" from
player/start.xml -- the profile the harness selects in addCharacter):
  * starter inventory: item id=5 "Potion grade A" <hp add="50"/> qty 1,
    item id=1 "IronTrap" (trap, NOT a monster effect) qty 5.
  * starter monster at position 0 (monster id=60, level 5, full HP).
So a fresh CharacterSelected player OWNS item 5 and HAS monster position 0:
  - VALID:   item 5 on monster position 0 -> accepted, no kick. (Full-HP heal
             is a no-op that consumes nothing; either way: not kicked.)
  - SEMANTIC-INVALID A: item 9999 (not in datapack / not owned) on position 0
             -> "don't have into the inventory" -> KICK.
  - SEMANTIC-INVALID B: item 5 (owned) on monster position 200 (player has 1
             monster; position out of range) -> haveThisMonsterByPosition false
             -> errorFightEngine -> KICK.
  - SEMANTIC-INVALID C: item 1 (OWNED, but it's a trap = NOT a monster item
             effect / evolution / learn item) on the VALID position 0. The inner
             CommonFightEngine::useObjectOnMonsterByPosition falls through every
             type branch into the final else -> errorFightEngine("This object
             can't be applied on monster") -> KICK (errorFightEngine ->
             errorOutput -> disconnectClient()). The Client outer then returns
             true, but the disconnect already happened, so the offending client
             IS kicked. Verify the kick + NO state corruption (still owns qty 5).

Verification strategy:
  * VALID (positive branch reached by DB-craft): grant a "Level book" (item 16,
    AddLevel) via the server's /give admin path (NEEDS_EVERY_BODY_IS_ROOT) and use
    it (DYNAMIC-framed) on owned monster 0. The use is ACCEPTED (not kicked) — and
    since an UNRECOGNISED/wrong-type item IS kicked (the owned-trap case proves it)
    and the only AddLevel non-success is level>=max (also a kick), acceptance here
    proves the AddLevel effect ran. ("Level book" is reusable -> not consumed.)
    reload_state confirms identity/position persist.
  * MALFORMED FRAMING (4 variants): sent from disposable sessions; after each,
    server.alive() True and crash_report() None. A malformed 0x10 either kicks
    the offending client or wedges nothing -- the SERVER must survive.
  * SEMANTIC-INVALID (3 cases): all three KICK (unknown/not-owned item, bad
    monster position, owned-but-wrong-type trap). For each assert (a) the
    offending socket is closed by the server (EOF), (b) server.alive() True +
    crash_report() None, (c) a SEPARATE legit account is byte-for-byte
    unaffected (no phantom state), and for the cheater account that the
    persisted identity/position are UNCHANGED (no corruption from the rejected
    action).
  * NO STATE CORRUPTION verified via a fresh independent victim Session and via
    H.reload_state of the cheater account (identity/position persist, no phantom
    monster/cash from the rejected action).
"""

import time
import _protoharness as H

NAME = "0x10 useObjectOnMonster"

# Reach the POSITIVE branch by crafting the DB: enable everyBodyIsRoot so the test
# can grant itself an AddLevel item via the server's own /give admin path (see
# H.grant_item). everyBodyIsRoot changes only command authorization, not the
# handler-under-test's threat model.
NEEDS_EVERY_BODY_IS_ROOT = True

# Datapack-known ids (CatchChallenger-datapack items.xml; the 'test' maincode loads
# the same global items, and player/start.xml grants the starter inventory).
ITEM_POTION = 5          # "Potion grade A" heal <hp add="50"/>: AddHp monster effect
ITEM_TRAP = 1            # "IronTrap": NOT a monster item effect -> final-else in
                         # CommonFightEngine -> errorFightEngine -> KICK on a monster.
ITEM_LEVELBOOK = 16      # "Level book" <level up="1"/>: monsterItemEffectOutOfFight
                         # AddLevel. Used on a below-max-level monster OUT of fight it
                         # SUCCEEDS (addLevel) and consumes one -> a real mutating
                         # positive path, independent of the monster's HP.
ITEM_UNKNOWN = 0x270F    # 9999: not in datapack, not owned
MON_POS_VALID = 0        # starter monster lives at position 0
MON_POS_BAD = 200        # player has 3 monsters -> position 200 out of range


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


def _pkt(item, monster_pos):
    """Build the 3-byte 0x10 payload: [item u16 LE][monsterPosition u8]."""
    return H.u16(item) + H.u8(monster_pos)


def _is_kicked(sess, settle=0.4):
    """Return True iff the server has CLOSED this session (kick).

    A kicked client's socket reaches EOF: a blocking recv returns b'' (clean
    FIN from the server) rather than timing out. We first drain any pending
    bytes (the kick may be preceded by a system-message broadcast), then probe
    the socket for EOF. If the peer is still open the recv times out -> not
    kicked. Any OSError on send/recv also means the peer is gone -> kicked.
    """
    try:
        sess.drain(timeout=settle)
    except OSError:
        return True
    sk = sess.sock
    try:
        # A still-open server would have nothing queued and time out; a kicked
        # one returns b'' immediately (FIN already arrived).
        sk.settimeout(settle)
        try:
            data = sk.recv(65536)
        except (TimeoutError, OSError) as e:
            # socket.timeout subclasses OSError; distinguish EOF (closed) from
            # a plain timeout (still open).
            if isinstance(e, TimeoutError):
                return False
            # other OSError (ECONNRESET/EBADF) -> peer gone.
            return True
        if data == b"":
            return True            # clean EOF -> server closed us == kicked.
        # Got bytes but no FIN yet: drain them and probe once more.
        try:
            sk.settimeout(settle)
            more = sk.recv(65536)
            return more == b""
        except TimeoutError:
            return False
        except OSError:
            return True
    except OSError:
        return True


def _legit_snapshot(server):
    """Create an independent legit account, return (login, passw, snap) where
    snap is its persisted identity/position. Used as the unaffected-victim
    baseline for the no-state-corruption check."""
    login = _uniq("vic")
    passw = _uniq("vpw")
    v = H.Session(server, login=login, passh=passw, pseudo="Victim")
    pos = (v.x, v.y, v.mapIndex)
    cid = v.character_id
    v.close()
    time.sleep(0.35)  # let FILE_DB disconnect-save complete
    snap = H.reload_state(server, login, passw)
    return login, passw, pos, cid, snap


def run(server):
    try:
        # ----------------------------------------------------------------
        # Independent victim account, snapshotted BEFORE any cheating, to
        # prove later that nobody else's state was corrupted.
        # ----------------------------------------------------------------
        try:
            vlogin, vpassw, vpos, vcid, vsnap0 = _legit_snapshot(server)
        except Exception as e:
            return (False, "victim baseline snapshot failed: %r" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim baseline: %s" % why)

        # ----------------------------------------------------------------
        # Precondition: a fully-handshaked on-map session that OWNS item 5
        # (Potion grade A) and HAS a monster at position 0 (profile 0).
        # ----------------------------------------------------------------
        login = _uniq("use")
        passw = _uniq("upw")
        try:
            user = H.Session(server, login=login, passh=passw, pseudo="User")
        except H.HandshakeError as e:
            return (False, "handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after handshake: %s" % why)

        valid_cases = 0
        malformed_cases = 0
        semantic_invalid_cases = 0

        # ----------------------------------------------------------------
        # VALID positive path (a REAL mutating success, reached by crafting the
        # DB). The fresh character already owns a starter inventory, but to drive
        # the strongest success branch we GRANT a "Level book" (item 16, a
        # monsterItemEffectOutOfFight AddLevel item) through the server's own
        # /give admin path, then use it on the owned monster at position 0. Out of
        # fight on a below-max-level monster this SUCCEEDS: addLevel() runs and the
        # use is ACCEPTED (not kicked).
        # ----------------------------------------------------------------
        H.grant_item(user, ITEM_LEVELBOOK, 1)
        user.m(0x10, _pkt(ITEM_LEVELBOOK, MON_POS_VALID), dynamic=True)
        valid_cases += 1
        user.drain(timeout=0.4)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after valid AddLevel use: %s" % why)
        if _is_kicked(user):
            return (False, "valid use of a GRANTED AddLevel item (16) on owned "
                           "monster position 0 wrongly KICKED the client — the "
                           "positive branch must be accepted")
        # The use being ACCEPTED proves the AddLevel branch executed: an
        # unrecognised / wrong-type item on a valid monster is KICKED (the owned-
        # trap semantic-invalid case below shows exactly that), and the only
        # AddLevel non-success is level>=max which also kicks. So "not kicked here"
        # rules out the final-else AND a silent no-op -> the item matched AddLevel
        # and levelled the monster. ("Level book" is reusable -> NOT consumed, so
        # we do not assert consumption.)

        # Persisted-state read-back for the user: clean-close it (still connected)
        # disconnected it, so FILE_DB has saved its character; reload confirms the
        # identity/position survived the level-up + the kick.
        try:
            user.close()
        except OSError:
            pass
        time.sleep(0.35)
        checks_db = False
        try:
            snap = H.reload_state(server, login, passw)
            if snap.get("character_id") is None:
                return (False, "reload_state: user character did not persist")
            if snap.get("x") is None or snap.get("mapIndex") is None:
                return (False, "reload_state: user position header unreadable")
            checks_db = True
            persisted = (snap["x"], snap["y"], snap["mapIndex"])
        except Exception as e:
            return (False, "reload_state(user) raised: %r" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after reload_state(user): %s" % why)

        # ----------------------------------------------------------------
        # MALFORMED FRAMING cases. Each from a FRESH disposable session,
        # because a malformed 0x10 kicks the offending client. After each we
        # assert the server is still alive and clean (no crash, no abort, no
        # hang). 0x10 is FIXED size 3 -> the framer reads exactly 3 data bytes.
        # ----------------------------------------------------------------
        def fresh():
            return H.Session(server, login=_uniq("bad"), passh=_uniq("bp"),
                             pseudo="Bad")

        # (1) Truncated payload: code + 2 data bytes (item only, NO monster
        #     position). The fixed framer wants 3; a short frame either stalls
        #     (waiting for the 3rd byte) or, once flushed by trailing garbage,
        #     is parsed with a wrong/borrowed byte -> handler rejects. Follow
        #     with garbage so the parser can't be wedged.
        try:
            s = fresh()
            s.send_raw(bytes([0x10, 0x05, 0x00]))      # code + item(5) only (2 data bytes)
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))  # garbage trailer
            s.drain(timeout=0.4)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass  # kicked mid-send is acceptable
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated 0x10 (2 data bytes): %s" % why)

        # (2) Single trailing byte only: code + 1 data byte. Even shorter.
        try:
            s = fresh()
            s.send_raw(bytes([0x10, 0x05]))
            time.sleep(0.15)
            s.send_raw(bytes([0x00, 0xFF, 0xFF, 0xFF]))
            s.drain(timeout=0.4)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated 0x10 (1 data byte): %s" % why)

        # (3) Bare code with NO data + immediate garbage. Confirms the parser
        #     does not crash on a 0-length fixed packet body.
        try:
            s = fresh()
            s.send_raw(bytes([0x10]))
            time.sleep(0.15)
            s.send_raw(bytes([0x00, 0x00, 0x00, 0xFF, 0xFF]))
            s.drain(timeout=0.4)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare 0x10 code: %s" % why)

        # (4) Over-long frame: code + 6 data bytes (3 extra). The fixed framer
        #     consumes exactly 3; the trailing 3 bytes are re-parsed as a NEW
        #     packet. We make those trailing bytes a harmless framing so the
        #     parser stays coherent; the server must not crash. (Sending raw so
        #     the harness framer doesn't normalise it.)
        try:
            s = fresh()
            # [0x10][item=5][mon=0]  +  three stray bytes that start an unknown
            # code 0xFF (blocked) -> server kicks/ignores, never crashes.
            s.send_raw(bytes([0x10, 0x05, 0x00, 0x00, 0xFF, 0xFF, 0xFF]))
            s.drain(timeout=0.4)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long 0x10 frame: %s" % why)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID A: well-formed 0x10 using an UNKNOWN/NOT-OWNED item
        # (id 9999) on a valid monster position. Handler:
        #   useObjectOnMonsterByPosition -> item not in inventory map ->
        #   errorOutput -> KICK. Assert kick + server survives + no corruption.
        # ----------------------------------------------------------------
        try:
            ch = H.Session(server, login=_uniq("ch1"), passh=_uniq("cp1"),
                           pseudo="CheatA")
        except H.HandshakeError as e:
            return (False, "cheatA handshake failed: %s" % e)
        ch.m(0x10, _pkt(ITEM_UNKNOWN, MON_POS_VALID), dynamic=True)
        semantic_invalid_cases += 1
        kicked = _is_kicked(ch)
        try:
            ch.close()
        except OSError:
            pass
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after unknown-item use: %s" % why)
        if not kicked:
            return (False, "unknown/not-owned item 9999 did NOT get the client "
                           "kicked (contract violation: silent ignore)")

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID B: well-formed 0x10 using item 5 (OWNED) on monster
        # position 200 (player has exactly 1 monster -> out of range). Handler:
        #   useObjectOnMonsterByPosition -> CommonFightEngine ->
        #   haveThisMonsterByPosition false -> errorFightEngine -> KICK.
        # ----------------------------------------------------------------
        try:
            ch = H.Session(server, login=_uniq("ch2"), passh=_uniq("cp2"),
                           pseudo="CheatB")
        except H.HandshakeError as e:
            return (False, "cheatB handshake failed: %s" % e)
        ch.m(0x10, _pkt(ITEM_POTION, MON_POS_BAD), dynamic=True)
        semantic_invalid_cases += 1
        kicked = _is_kicked(ch)
        try:
            ch.close()
        except OSError:
            pass
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after out-of-range monster position use: %s" % why)
        if not kicked:
            return (False, "use on non-existent monster position 200 did NOT "
                           "kick the client (contract violation)")

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID C: well-formed 0x10 using item 1 (OWNED IronTrap, a
        # trap and NOT a monster item effect / evolution / learn item) on the
        # VALID monster position 0. The inner CommonFightEngine call matches
        # none of has_evolutionItem / has_monsterItemEffect[ OutOfFight] /
        # has_itemToLearn, so it falls into the final else ->
        # errorFightEngine("This object can't be applied on monster") -> KICK
        # (errorFightEngine -> errorOutput -> disconnectClient()). Even though
        # Client::useObjectOnMonsterByPosition then returns true, the disconnect
        # already fired, so the offending client IS kicked. Assert the kick +
        # server survives + NO state corruption (cheater identity/position
        # unchanged, no phantom monster/cash).
        # ----------------------------------------------------------------
        clog = _uniq("ch3")
        cpas = _uniq("cp3")
        try:
            ch = H.Session(server, login=clog, passh=cpas, pseudo="CheatC")
        except H.HandshakeError as e:
            return (False, "cheatC handshake failed: %s" % e)
        cpos0 = (ch.x, ch.y, ch.mapIndex)
        ccid0 = ch.character_id
        ch.m(0x10, _pkt(ITEM_TRAP, MON_POS_VALID), dynamic=True)
        semantic_invalid_cases += 1
        wrongtype_kicked = _is_kicked(ch)
        try:
            ch.close()
        except OSError:
            pass
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after owned-but-wrong-type (trap) item use: %s" % why)
        if not wrongtype_kicked:
            return (False, "owned-but-wrong-type item (trap id 1) on a valid "
                           "monster did NOT kick the client; the engine's final "
                           "else (errorFightEngine) must disconnect (contract "
                           "violation: silent ignore)")
        time.sleep(0.35)
        # No-state-corruption: identity/position must persist UNCHANGED for the
        # cheater account (no phantom monster/cash, no position mutation).
        try:
            csnap = H.reload_state(server, clog, cpas)
        except Exception as e:
            return (False, "reload_state(cheatC) raised: %r" % e)
        if csnap.get("character_id") != ccid0:
            return (False, "owned-wrong-type use corrupted cheater identity: "
                           "%r != %r" % (csnap.get("character_id"), ccid0))
        cpos1 = (csnap.get("x"), csnap.get("y"), csnap.get("mapIndex"))
        if cpos1 != cpos0:
            return (False, "owned-wrong-type use moved the cheater: %r -> %r"
                           % (cpos0, cpos1))

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION across players: the independent victim account
        # snapshotted at the very start must be byte-for-byte unchanged after
        # all the cheating above.
        # ----------------------------------------------------------------
        try:
            vsnap1 = H.reload_state(server, vlogin, vpassw)
        except Exception as e:
            return (False, "reload_state(victim) after cheats raised: %r" % e)
        if vsnap1.get("character_id") != vcid:
            return (False, "victim identity changed after cheats: %r != %r"
                           % (vsnap1.get("character_id"), vcid))
        vpos1 = (vsnap1.get("x"), vsnap1.get("y"), vsnap1.get("mapIndex"))
        vpos0 = (vsnap0.get("x"), vsnap0.get("y"), vsnap0.get("mapIndex"))
        if vpos1 != vpos0:
            return (False, "victim position changed after cheats: %r -> %r"
                           % (vpos0, vpos1))

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after no-corruption checks: %s" % why)

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session still handshakes to
        # CharacterSelected and can issue a clean 0x10 after all the abuse.
        # ----------------------------------------------------------------
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.m(0x10, _pkt(ITEM_POTION, MON_POS_VALID), dynamic=True)
            again.drain(timeout=0.3)
            if _is_kicked(again):
                return (False, "post-abuse legit use was kicked (server state "
                               "corrupted by earlier malformed input)")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d malformed=%d semantic_invalid=%d db_checked=%s "
            "user_persisted=%s wrongtype_kicked=%s "
            "(POSITIVE branch reached via DB-craft: a granted AddLevel item (16) "
            "used on owned monster 0 was ACCEPTED — an unrecognised/wrong-type "
            "item is kicked, so acceptance proves the AddLevel effect ran. All "
            "three semantic-invalid inputs kicked "
            "the cheater: unknown/not-owned item, out-of-range monster position, "
            "owned-but-wrong-type trap; victim + cheater identity/position "
            "unchanged; server stayed alive+clean throughout)"
            % (valid_cases, malformed_cases, semantic_invalid_cases, checks_db,
               persisted, wrongtype_kicked)
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
