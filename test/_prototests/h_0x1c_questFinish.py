#!/usr/bin/env python3
"""
h_0x1c_questFinish.py — protocol test for server handler 0x1C (quest finish).

Handler (server/base/ClientNetworkReadMessage.cpp:626 -> newQuestAction(Finish)):

    case 0x1C:  // Quest finish
    {
        #ifdef CATCHCHALLENGER_HARDENED
        if(size!=((int)sizeof(uint16_t)))
        { errorOutput("wrong remaining size for quest finish"); return false; }
        #endif
        const uint16_t &questId=CatchChallenger::loadLe16(data);
        newQuestAction(QuestAction_Finish,questId);
        return true;
    }

WIRE SHAPE: 0x1C is a FIXED-size-2 MESSAGE (packetFixedSize[0x1C]==2,
ProtocolParsingGeneral.cpp). On the wire it is exactly 3 bytes:
    [0x1C][questId_lo][questId_hi]      (questId is u16, LITTLE-ENDIAN)
No queryNumber (code<0x80, not a reply), no 4-byte dynamic length (fixed size).
There is NO reply packet — quest finish is fire-and-forget; success mutates the
player's quest/inventory/reputation server-side and re-syncs the DB, but emits
no dedicated 0x1C reply to the offending client.

SEMANTICS (server/base/ClientEvents/LocalClientHandlerQuest.cpp):
  newQuestAction(Finish, questId):
    * if !has_quest(questId)                       -> errorOutput("unknown questId")          -> KICK
    * else if !haveNextStepQuestRequirements(q)    -> errorOutput("have not next step ...")   -> KICK
    * else nextStepQuest(quest):                      (Finish and NextStep share this path)
          removeObject() the step's required items, advance quests[id].step;
          when step exceeds the last step -> finish_one_time=true, hand out
          rewards (items/reputation/allow_create_clan), syncDatabaseQuest(). NO reply.

  haveNextStepQuestRequirements(q) returns false (=> KICK on Finish) when:
    - the player NEVER STARTED this quest (id not in public_and_private_informations.quests), OR
    - the player's stored step is out of range (step<=0 || step>steps.size()), OR
    - a required ITEM is not owned in the needed quantity (objectQuantity < quantity), OR
    - a required bot in the step has not been beaten.

DATAPACK ('test' maincode): exactly ONE quest exists, id 1 ("Berry exchange",
repeatable). Its single step requires 1 "Moco Berry" item and rewards 1 "Tuber"
+ allow_create_clan — see map/main/test/quests/1/definition.xml. A FRESH player
(start.xml: cash 0, NO starting items — map/main/test/start.xml) owns ZERO Moco
Berry, and there is NO in-band protocol handler that grants a player an arbitrary
item. THEREFORE:

  * 0x1C Finish on a quest that was NEVER STARTED     -> not in quests map  -> KICK
  * 0x1C Finish on a STARTED quest 1 but WITHOUT the
    required Moco Berry item (the only reachable state
    for a test-maincode player)                       -> item requirement  -> KICK
  * 0x1C Finish with any UNKNOWN quest id             -> unknown quest      -> KICK

REACHABILITY-LIMITED VALID CASE: a genuinely-SUCCESSFUL finish needs the player
to OWN the required item, which cannot be produced in-band in the 'test'
maincode (no item-grant packet; start.xml gives nothing). So there is no input
that makes 0x1C succeed silently here. We still exercise the full legitimate
flow — start quest 1 (0x1B, accepted) then attempt 0x1C Finish — and assert the
handler refuses GRACEFULLY (kick, no crash/hang/leak) because the item is
missing. This is documented as reachability-limited: the success branch of
nextStepQuest() is not reachable without item granting, but every REFUSAL branch
AND the framing are exercised. The dominant, fully-reachable contract here is the
SEMANTIC-INVALID kick path, which is verified exhaustively.

OBSERVABILITY: questFinish sends no reply. reload_state only frames back
id/pseudo/position (DOCUMENTED LIMIT) — not the quest/inventory sub-block — so
the "no state corruption" check uses a separate untouched witness account plus
the kicked account's persisted identity/position (must be byte-identical).
"""

import time
import socket
import _protoharness as H

NAME = "0x1C questFinish"

# Quest id that EXISTS in the 'test' maincode datapack (startable, repeatable).
EXISTING_QUEST_ID = 1
# Quest ids that do NOT exist in the 'test' datapack -> semantic-illegal -> kick.
UNKNOWN_QUEST_IDS = [9999, 0xFFFF, 0]


# ---- small helpers ---------------------------------------------------------

def _is_kicked(sess, settle=2.0):
    """True if the server has closed our socket (kick). Pump+auto-ACK any
    pending server traffic first, then read raw bytes until EOF or the settle
    window elapses. A kick = FIN -> recv returns b''. A still-open, silent
    connection = repeated timeouts with no EOF -> not kicked.

    Liveness probe discipline (mirrors h_0x1b_questStart): send AT MOST ONE
    genuinely-harmless in-game packet (a single valid 0x02 move) to nudge a
    deferred socket teardown. DO NOT probe on every timeout slice and DO NOT
    send an unknown code: a repeated probe trips the server's per-packet DDoS
    counter and an unknown code self-triggers a KICK, turning this detector into
    a FALSE POSITIVE on a client the server never kicked. One harmless move is
    enough; after it we only watch for EOF."""
    try:
        sess.drain(0.4)
    except Exception:
        # drain only blows up if the socket is already dead -> treat as kicked
        return True
    probed = False
    deadline = time.time() + settle
    while time.time() < deadline:
        sess.sock.settimeout(0.4)
        try:
            d = sess.sock.recv(65536)
            if d == b"":
                return True            # EOF -> kicked
            # got real bytes (e.g. a trailing broadcast); keep reading for EOF
        except socket.timeout:
            # no data this slice. Send the SINGLE harmless liveness nudge once;
            # thereafter only watch for EOF (no repeated kickable traffic).
            if not probed:
                probed = True
                try:
                    sess.sock.sendall(bytes([0x02, 0x01, 0x04]))  # one valid move
                except OSError:
                    return True
        except OSError:
            return True
    return False


def _alive_and_clean(server):
    """(ok, why) — server still up AND no crash report."""
    if not server.alive():
        return False, "server not alive"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % cr[:300]
    return True, ""


# ---- the test --------------------------------------------------------------

def run(server):
    try:
        # ---------------------------------------------------------------
        # 0) A separate, untouched legit session (different account) used as the
        #    "no state corruption" witness — it must be unaffected by every
        #    illegal / malformed thing the cheaters do.
        # ---------------------------------------------------------------
        witness = H.Session(server, pseudo="Witness")
        wx, wy, wmap = witness.x, witness.y, witness.mapIndex

        # ===============================================================
        # 1) VALID well-formed packet (REACHABILITY-LIMITED).
        #    A genuinely-successful 0x1C finish requires owning the step's
        #    required item (1 Moco Berry); a 'test'-maincode player can NEVER
        #    own it in-band (start.xml grants nothing, no item-grant packet). So
        #    we run the full legitimate flow and assert the handler refuses
        #    GRACEFULLY (kick, no crash) on the missing item — exercising the
        #    requirement branch + the wire framing without a server fault.
        #
        #    Step 1a: start quest 1 (0x1B) — accepted, NOT kicked (proves the
        #             quest subsystem + our fixed-2 framing are correct).
        #    Step 1b: 0x1C finish quest 1 — player lacks Moco Berry ->
        #             haveNextStepQuestRequirements() false -> KICK. Server must
        #             stay alive+clean (the cheaper-than-finish reachable path).
        # ===============================================================
        s_valid = H.Session(server, pseudo="QfV")
        # 1a) legitimate quest start (must be accepted, no kick)
        s_valid.m(0x1B, H.u16(EXISTING_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid 0x1B (start quest 1) pre-finish: %s" % why
        if _is_kicked(s_valid, settle=1.2):
            return (False,
                    "starting quest 1 (0x1B) before the finish attempt wrongly "
                    "KICKED the client — cannot reach the finish path")
        # 1b) well-formed finish of the started quest, but item missing -> kick
        s_valid.m(0x1C, H.u16(EXISTING_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid-shape 0x1C (finish w/o item): %s" % why
        if not _is_kicked(s_valid):
            try: s_valid.close()
            except Exception: pass
            return (False,
                    "well-formed 0x1C finish of started quest 1 WITHOUT the "
                    "required Moco Berry did NOT kick — haveNextStepQuestRequirements "
                    "must reject the missing-item requirement")
        try: s_valid.close()
        except Exception: pass
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after reachability-limited finish kick: %s" % why

        # ===============================================================
        # 2) MALFORMED-FRAMING variants. Each on its OWN fresh session. After
        #    EACH: server.alive() True and crash_report None. 0x1C is a fixed
        #    size-2 packet, so we malform its declared/actual length and the
        #    surrounding framing the parser must defend against.
        # ===============================================================
        malformed = [
            # (a) 0x1C with only ONE payload byte instead of two: the parser
            #     frames 0x1C as a fixed-2 packet. Append a blocked trailing
            #     code 0x01 so the parse pass terminates deterministically;
            #     under HARDENED the size!=2 guard kicks. Server must not crash.
            b"\x1c\x01",
            # (b) 0x1C prefixed/followed by a bogus 4-byte length the parser must
            #     NOT read (0x1C is FIXED, not dynamic). Exercises the
            #     fixed-vs-dynamic framing decision from packetFixedSize[].
            b"\x1c" + b"\x01\x00\x00\x00\x00",
            # (c) truncated DYNAMIC packet: 0x03 (chat) declares 16 data bytes
            #     but only 2 follow. Parser must wait/not over-read, not crash.
            b"\x03" + H.u32(16) + b"\x00\x01",
            # (d) absurd DYNAMIC length: 0x03 chat claiming ~4 GiB of payload.
            #     Server must not allocate/hang/over-read on the bogus size.
            b"\x03" + H.u32(0xFFFFFFFF),
        ]
        for i, raw in enumerate(malformed):
            sm = H.Session(server, pseudo="QfM%d" % i)
            try:
                sm.send_raw(raw)
            except OSError:
                pass  # peer may already be tearing down; that's fine
            ok, why = _alive_and_clean(server)
            if not ok:
                try: sm.close()
                except Exception: pass
                return False, "after malformed[%d]=%r: %s" % (i, raw, why)
            try: sm.close()
            except Exception: pass

        # ===============================================================
        # 3) SEMANTIC-INVALID (the key contract): well-formed 0x1C packets that
        #    are semantically illegal for THIS handler. Three distinct illegal
        #    classes, all -> errorOutput() -> KICK:
        #      (3a) UNKNOWN questId (not in the datapack at all)
        #             -> errorOutput("unknown questId")
        #      (3b) FINISH A NEVER-STARTED but EXISTING quest (id 1): the quest
        #           is not in the player's quests map ->
        #           haveNextStepQuestRequirements() false ->
        #           errorOutput("have not next step quest requirement"). This is
        #           the not-owned-resource case for quests.
        #      (3c) FINISH A STARTED quest WITHOUT the required item (start id 1,
        #           then finish with 0 Moco Berry) — the item-requirement reject.
        #           (Same path as the reachability-limited case in step 1, repeated
        #           here as an explicit cheater run on a throwaway account so its
        #           kick + no-corruption is asserted alongside the other classes.)
        #    For EACH illegal input assert:
        #       (a) KICK the offending client,
        #       (b) leave the SERVER alive + clean (+ valgrind clean),
        #       (c) cause NO state corruption — witness + the kicked account's
        #           own persisted identity/position unchanged.
        # ===============================================================

        # ---- (3a) unknown quest ids -----------------------------------
        kicked_login = kicked_pass = None
        kicked_id = kicked_pos = None
        for idx, bad_qid in enumerate(UNKNOWN_QUEST_IDS):
            s_cheat = H.Session(server, pseudo="QfX%d" % idx)
            if idx == 0:
                kicked_login, kicked_pass = s_cheat.login_creds, s_cheat.pass_creds
                kicked_id = s_cheat.character_id
                kicked_pos = (s_cheat.x, s_cheat.y, s_cheat.mapIndex)

            # well-formed packet (3 bytes), semantically illegal: unknown quest
            s_cheat.m(0x1C, H.u16(bad_qid))

            # (b) server survives
            ok, why = _alive_and_clean(server)
            if not ok:
                try: s_cheat.close()
                except Exception: pass
                return False, "after illegal finish qid=%d: %s" % (bad_qid, why)

            # (a) the cheater is kicked (socket reaches EOF)
            kicked = _is_kicked(s_cheat)
            try: s_cheat.close()
            except Exception: pass
            if not kicked:
                return (False,
                        "illegal 0x1C (unknown questId=%d) did NOT kick the "
                        "client (connection stayed open) — contract violation"
                        % bad_qid)

            ok, why = _alive_and_clean(server)
            if not ok:
                return False, "after illegal-finish kick qid=%d: %s" % (bad_qid, why)

        # ---- (3b) finish a NEVER-STARTED but existing quest -----------
        s_ns = H.Session(server, pseudo="QfNS")
        s_ns.m(0x1C, H.u16(EXISTING_QUEST_ID))     # never started -> not in map
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_ns.close()
            except Exception: pass
            return False, "after finish-of-never-started quest: %s" % why
        ns_kicked = _is_kicked(s_ns)
        try: s_ns.close()
        except Exception: pass
        if not ns_kicked:
            return (False,
                    "finishing a NEVER-STARTED quest (0x1C id 1, not in the "
                    "player's quests map) did NOT kick — "
                    "haveNextStepQuestRequirements must reject it")
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after never-started-finish kick: %s" % why

        # ---- (3c) finish a STARTED quest WITHOUT the required item ----
        s_noitem = H.Session(server, pseudo="QfNI")
        s_noitem.m(0x1B, H.u16(EXISTING_QUEST_ID))   # legitimate start
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_noitem.close()
            except Exception: pass
            return False, "after start in no-item-finish case: %s" % why
        if _is_kicked(s_noitem, settle=1.0):
            try: s_noitem.close()
            except Exception: pass
            return (False,
                    "legitimate quest-1 start in the no-item-finish case was "
                    "wrongly kicked before the finish attempt")
        s_noitem.m(0x1C, H.u16(EXISTING_QUEST_ID))   # finish w/o Moco Berry
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_noitem.close()
            except Exception: pass
            return False, "after started-but-no-item finish: %s" % why
        ni_kicked = _is_kicked(s_noitem)
        try: s_noitem.close()
        except Exception: pass
        if not ni_kicked:
            return (False,
                    "finishing a STARTED quest 1 WITHOUT the required Moco Berry "
                    "did NOT kick — the item requirement must be enforced")
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after no-item-finish kick: %s" % why

        # ---- (c) NO STATE CORRUPTION ----------------------------------
        #  - witness (a different account, still connected) is untouched: same
        #    spawn position, server still serving it.
        if (witness.x, witness.y, witness.mapIndex) != (wx, wy, wmap):
            return (False,
                    "witness position changed by another client's illegal finish: "
                    "%r -> %r" % ((wx, wy, wmap),
                                  (witness.x, witness.y, witness.mapIndex)))
        # the witness socket must still be open (not collaterally kicked)
        if _is_kicked(witness, settle=0.8):
            return False, "witness was collaterally kicked by a cheater's illegal finish"

        #  - the kicked account's PERSISTED character is intact: the rejected
        #    finish must not have moved it or altered its identity. Give the
        #    disconnect-save a moment, then read it back.
        witness.close()
        time.sleep(0.4)
        snap = H.reload_state(server, kicked_login, kicked_pass)
        if snap.get("character_id") is None:
            return (False,
                    "kicked account's character vanished after illegal finish "
                    "(reload_state could not read it back): %r" % snap)
        if kicked_id is not None and snap.get("character_id") != kicked_id:
            return (False,
                    "kicked account's character_id changed: %r -> %r"
                    % (kicked_id, snap.get("character_id")))
        if kicked_pos is not None and kicked_pos[0] is not None:
            if (snap.get("x"), snap.get("y"), snap.get("mapIndex")) != kicked_pos:
                return (False,
                        "kicked account's persisted position mutated by the "
                        "rejected finish: %r -> %r"
                        % (kicked_pos,
                           (snap.get("x"), snap.get("y"), snap.get("mapIndex"))))

        # ===============================================================
        # 4) leave the server usable: prove a fresh legit Session still works.
        # ===============================================================
        post = H.Session(server, pseudo="QfPost")
        ok, why = _alive_and_clean(server)
        post.close()
        if not ok:
            return False, "server unusable after the test sequence: %s" % why

        return (True,
                "0x1C questFinish: VALID finish is reachability-limited in the "
                "'test' maincode (a successful finish needs the player to OWN the "
                "step's Moco Berry, which cannot be granted in-band — start.xml "
                "gives nothing, no item-grant packet), so the legitimate flow "
                "(start quest 1, then finish without the item) is exercised and the "
                "handler refuses GRACEFULLY with a kick, server alive+clean. "
                "4 malformed-framing variants each kept the server alive+clean. "
                "5 illegal finishes each KICK the cheater while the server survives: "
                "3 unknown questIds (9999/0xFFFF/0), 1 finish of a NEVER-STARTED "
                "existing quest, 1 finish of a STARTED quest WITHOUT the required "
                "item. No state corruption (witness + kicked account's persisted "
                "id/position intact). DB-quest/inventory sub-block not framed by "
                "reload_state -> identity/position used for the corruption check "
                "(documented limit)")
    except H.HandshakeError as e:
        return False, "handshake failed: %r" % e
    except Exception as e:
        import traceback
        return False, "unexpected exception: %r\n%s" % (e, traceback.format_exc()[-800:])
