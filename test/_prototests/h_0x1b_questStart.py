#!/usr/bin/env python3
"""
h_0x1b_questStart.py — protocol test for server handler 0x1B (quest start).

Handler (server/base/ClientNetworkReadMessage.cpp:611 -> newQuestAction(Start)):

    case 0x1B:  // Quest start
    {
        #ifdef CATCHCHALLENGER_HARDENED
        if(size!=((int)sizeof(uint16_t)))
        { errorOutput("wrong remaining size for quest start"); return false; }
        #endif
        const uint16_t &questId=CatchChallenger::loadLe16(data);
        newQuestAction(QuestAction_Start,questId);
        return true;
    }

WIRE SHAPE: 0x1B is a FIXED-size-2 MESSAGE (packetFixedSize[0x1B]==2,
ProtocolParsingGeneral.cpp). On the wire it is exactly 3 bytes:
    [0x1B][questId_lo][questId_hi]      (questId is u16, LITTLE-ENDIAN)
No queryNumber (code<0x80, not a reply), no 4-byte dynamic length (fixed size).
There is NO reply packet — quest start is fire-and-forget.

SEMANTICS (server/base/ClientEvents/LocalClientHandlerQuest.cpp):
  newQuestAction(Start, questId):
    * if !has_quest(questId)                 -> errorOutput("unknown questId: ...")        -> KICK
    * else if !haveStartQuestRequirement(q)  -> errorOutput("have not quest requirement")  -> KICK
    * else startQuest(quest):
          public_and_private_informations.quests[id].step = 1;
          (sets finish_one_time=false on first start)
          addQuestStepDrop(id,1); syncDatabaseQuest();   (NO reply)

  haveStartQuestRequirement(q) returns false (=> KICK on Start) when:
    - the quest is ALREADY running for this player (quests[id].step != 0), OR
    - it was finished one time and is NOT repeatable, OR
    - a prerequisite quest is missing / a reputation requirement is unmet.

DATAPACK ('test' maincode): exactly ONE quest exists, id 1 ("Berry exchange",
repeatable, NO prerequisite quests, NO reputation requirement) — see
map/main/test/quests/1/definition.xml. So for a FRESH player:
  * 0x1B with id 1            -> requirement met -> startQuest -> accepted (no kick)
  * 0x1B with id 1 AGAIN      -> quest already running (step==1!=0) -> KICK
  * 0x1B with any other id    -> unknown quest -> KICK

OBSERVABILITY: questStart sends no reply, so the VALID case is observed as
"server alive AND client NOT kicked" (the message is accepted and processed).
The quests map is persisted into the HPS character block at disconnect, but
H.reload_state only frames back id/pseudo/position (DOCUMENTED LIMIT), not the
quest sub-block — so the quest mutation itself is asserted in-band (no kick =
accepted) and the DB check is used for IDENTITY/POSITION-not-corrupted only.
"""

import time
import socket
import _protoharness as H

NAME = "0x1B questStart"

# Quest id known to EXIST in the 'test' maincode datapack and be startable by a
# fresh player (repeatable, no prerequisites, no reputation requirement).
VALID_QUEST_ID = 1
# Quest ids that do NOT exist in the 'test' datapack -> semantic-illegal -> kick.
UNKNOWN_QUEST_IDS = [9999, 0xFFFF, 0]


# ---- small helpers ---------------------------------------------------------

def _is_kicked(sess, settle=2.0):
    """True if the server has closed our socket (kick). Pump+auto-ACK any
    pending server traffic first, then read raw bytes until EOF or the settle
    window elapses. A kick = FIN -> recv returns b''. A still-open, silent
    connection = repeated timeouts with no EOF -> not kicked.

    Liveness probe discipline: send AT MOST ONE genuinely-harmless in-game
    packet (a single valid 0x02 move) to nudge a deferred socket teardown.
    DO NOT send a probe on every timeout slice and DO NOT send 0x65: a repeated
    probe trips the server's per-packet DDoS counter (kickLimitMove/Other) and a
    0x65 has no handler ("unknown main ident: 101") — either self-triggers a
    KICK and turns this detector into a FALSE POSITIVE on a client the server
    never kicked. One harmless move is enough; after it we only watch for EOF."""
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
        # 1) VALID well-formed packet.
        #    Start quest 1 (0x1B). Quest 1 is repeatable with no prerequisites
        #    and no reputation requirement, so a fresh player meets
        #    haveStartQuestRequirement() and startQuest() runs: it sets
        #    quests[1].step=1 and re-syncs the DB. There is NO reply, so the
        #    contract is: the server processes it WITHOUT kicking.
        # ===============================================================
        s_valid = H.Session(server, pseudo="QsV")
        login_v, pass_v = s_valid.login_creds, s_valid.pass_creds
        cidv = s_valid.character_id
        cposv = (s_valid.x, s_valid.y, s_valid.mapIndex)

        s_valid.m(0x1B, H.u16(VALID_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid 0x1B (start quest 1): %s" % why
        # accepted == NOT kicked (no reply for this handler)
        if _is_kicked(s_valid, settle=1.2):
            return (False,
                    "valid 0x1B (start startable quest id 1) wrongly KICKED the "
                    "client — should be accepted silently")
        s_valid.close()

        # ===============================================================
        # 2) MALFORMED-FRAMING variants. Each on its OWN fresh session. After
        #    EACH: server.alive() True and crash_report None. 0x1B is a fixed
        #    size-2 packet, so we malform its declared/actual length and the
        #    surrounding framing the parser must defend against.
        # ===============================================================
        malformed = [
            # (a) 0x1B with only ONE payload byte instead of two: the parser
            #     frames 0x1B as a fixed-2 packet, so it will wait for / consume
            #     the next byte. Append a blocked trailing code 0x01 so the parse
            #     pass terminates deterministically; under HARDENED the
            #     size!=2 guard kicks. Server must not crash.
            b"\x1b\x01",
            # (b) 0x1B prefixed with a bogus 4-byte length the parser must NOT
            #     read (0x1B is FIXED, not dynamic). We send 0x1B then a single
            #     byte, then garbage — exercises the fixed-vs-dynamic framing
            #     decision the parser makes from packetFixedSize[].
            b"\x1b" + b"\x01\x00\x00\x00\x00",
            # (c) truncated DYNAMIC packet: 0x03 (chat) declares 16 data bytes
            #     but only 2 follow. Parser must wait/not over-read, not crash.
            b"\x03" + H.u32(16) + b"\x00\x01",
            # (d) absurd DYNAMIC length: 0x03 chat claiming ~4 GiB of payload.
            #     Server must not allocate/hang/over-read on the bogus size.
            b"\x03" + H.u32(0xFFFFFFFF),
        ]
        for i, raw in enumerate(malformed):
            sm = H.Session(server, pseudo="QsM%d" % i)
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
        # 3) SEMANTIC-INVALID (the key contract): well-formed 0x1B packets that
        #    are semantically illegal for THIS handler. Two distinct illegal
        #    classes, both -> errorOutput() -> KICK:
        #      (3a) UNKNOWN questId (not in the datapack at all)
        #             -> errorOutput("unknown questId")
        #      (3b) START AN ALREADY-RUNNING quest (start id 1 twice on the same
        #           session): the second start fails haveStartQuestRequirement()
        #           because quests[1].step==1!=0 -> errorOutput("have not quest
        #           requirement"). This is the requirement-path kick unique to
        #           Start (Cancel has no such path).
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
            s_cheat = H.Session(server, pseudo="QsX%d" % idx)
            if idx == 0:
                kicked_login, kicked_pass = s_cheat.login_creds, s_cheat.pass_creds
                kicked_id = s_cheat.character_id
                kicked_pos = (s_cheat.x, s_cheat.y, s_cheat.mapIndex)

            # well-formed packet (3 bytes), semantically illegal: unknown quest
            s_cheat.m(0x1B, H.u16(bad_qid))

            # (b) server survives
            ok, why = _alive_and_clean(server)
            if not ok:
                try: s_cheat.close()
                except Exception: pass
                return False, "after illegal start qid=%d: %s" % (bad_qid, why)

            # (a) the cheater is kicked (socket reaches EOF)
            kicked = _is_kicked(s_cheat)
            try: s_cheat.close()
            except Exception: pass
            if not kicked:
                return (False,
                        "illegal 0x1B (unknown questId=%d) did NOT kick the "
                        "client (connection stayed open) — contract violation"
                        % bad_qid)

            # re-check the server is still healthy after the kick
            ok, why = _alive_and_clean(server)
            if not ok:
                return False, "after illegal-start kick qid=%d: %s" % (bad_qid, why)

        # ---- (3b) start an already-running quest ----------------------
        #   First start succeeds (accepted, no kick); the SECOND start of the
        #   same id must fail the requirement check and KICK.
        s_dup = H.Session(server, pseudo="QsDup")
        s_dup.m(0x1B, H.u16(VALID_QUEST_ID))          # legitimate first start
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_dup.close()
            except Exception: pass
            return False, "after first (valid) 0x1B on dup-session: %s" % why
        if _is_kicked(s_dup, settle=1.0):
            try: s_dup.close()
            except Exception: pass
            return (False,
                    "first 0x1B start of quest 1 on dup-session was wrongly "
                    "kicked — it should succeed before the duplicate test")
        # second start of the SAME quest -> already running -> requirement fail
        s_dup.m(0x1B, H.u16(VALID_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_dup.close()
            except Exception: pass
            return False, "after duplicate 0x1B (already-running quest): %s" % why
        dup_kicked = _is_kicked(s_dup)
        try: s_dup.close()
        except Exception: pass
        if not dup_kicked:
            return (False,
                    "starting an ALREADY-RUNNING quest (0x1B id 1 twice) did NOT "
                    "kick — haveStartQuestRequirement should reject step!=0")
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after duplicate-start kick: %s" % why

        # ---- (c) NO STATE CORRUPTION ----------------------------------
        #  - witness (a different account, still connected) is untouched: same
        #    spawn position, server still serving it.
        if (witness.x, witness.y, witness.mapIndex) != (wx, wy, wmap):
            return (False,
                    "witness position changed by another client's illegal start: "
                    "%r -> %r" % ((wx, wy, wmap),
                                  (witness.x, witness.y, witness.mapIndex)))
        # the witness socket must still be open (not collaterally kicked)
        if _is_kicked(witness, settle=0.8):
            return False, "witness was collaterally kicked by a cheater's illegal start"

        #  - the kicked account's PERSISTED character is intact: the rejected
        #    start must not have moved it or altered its identity. Give the
        #    disconnect-save a moment, then read it back.
        witness.close()
        time.sleep(0.4)
        snap = H.reload_state(server, kicked_login, kicked_pass)
        if snap.get("character_id") is None:
            return (False,
                    "kicked account's character vanished after illegal start "
                    "(reload_state could not read it back): %r" % snap)
        if kicked_id is not None and snap.get("character_id") != kicked_id:
            return (False,
                    "kicked account's character_id changed: %r -> %r"
                    % (kicked_id, snap.get("character_id")))
        if kicked_pos is not None and kicked_pos[0] is not None:
            if (snap.get("x"), snap.get("y"), snap.get("mapIndex")) != kicked_pos:
                return (False,
                        "kicked account's persisted position mutated by the "
                        "rejected start: %r -> %r"
                        % (kicked_pos,
                           (snap.get("x"), snap.get("y"), snap.get("mapIndex"))))

        # ===============================================================
        # 4) leave the server usable: prove a fresh legit Session still works.
        # ===============================================================
        post = H.Session(server, pseudo="QsPost")
        ok, why = _alive_and_clean(server)
        post.close()
        if not ok:
            return False, "server unusable after the test sequence: %s" % why

        return (True,
                "0x1B questStart: valid start of startable quest id 1 is accepted "
                "WITHOUT a kick (no reply by design); 4 malformed-framing variants "
                "each kept the server alive+clean; 4 illegal starts each KICK the "
                "cheater while the server survives — 3 unknown questIds "
                "(9999/0xFFFF/0) and 1 duplicate start of an already-running quest "
                "(requirement-path reject); no state corruption (witness + kicked "
                "account's persisted id/position intact). DB-quest sub-block not "
                "framed by reload_state -> identity/position used for the "
                "corruption check (documented limit)")
    except H.HandshakeError as e:
        return False, "handshake failed: %r" % e
    except Exception as e:
        import traceback
        return False, "unexpected exception: %r\n%s" % (e, traceback.format_exc()[-800:])
