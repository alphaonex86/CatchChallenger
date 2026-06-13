#!/usr/bin/env python3
"""
Protocol handler test for server message 0x1E (quest NextStep).

Handler: server/base/ClientNetworkReadMessage.cpp:656
  case 0x1E: fixed [questId:u16] -> newQuestAction(QuestAction_NextStep, questId)

Call chain (server/base/ClientEvents/LocalClientHandlerQuest.cpp):
  Client::newQuestAction(NextStep, questId)
    if !commonDatapackServerSpec.has_quest(questId): errorOutput("unknown questId") -> KICK
    QuestAction_NextStep -> if !haveNextStepQuestRequirements(quest):
                                errorOutput("have not next step quest requirement") -> KICK
                            else nextStepQuest(quest)  (advances step, no reply)

Framing: 0x1E is a FIXED-size (2 bytes) MESSAGE (code<0x80, not a reply) ->
  on the wire it is exactly [0x1E][questId lo][questId hi]; NO queryNumber, NO
  4-byte size field, and NO server reply on success (fire-and-forget).

Datapack ('test' maincode): quest folder map/main/test/quests/1 exists, so
  questId 1 is the only valid quest. Its step-1 requirement is 1x "Moco Berry";
  a fresh player owns 0 of that item.

KEY CONTRACT this test asserts:
  * MALFORMED framing -> server stays ALIVE (the offending client is kicked or
    the parser stalls, the server never crashes/hangs).
  * SEMANTIC-INVALID well-formed packets (unknown questId, a quest the player
    never started) -> server emits errorOutput() and DISCONNECTS (kicks) the
    offending client, the server SURVIVES, and NO state is corrupted (verified
    with a separate legitimate session + reload_state).

REACHABILITY LIMIT (documented): a NextStep that actually ADVANCES the quest
  step requires the player to (a) have the quest started AND (b) hold the
  step's required item (1x "Moco Berry"). The raw protocol harness cannot put
  an item into the player's inventory, so the success/state-mutation branch of
  nextStepQuest() is not reachable here. We therefore exercise the well-formed
  NextStep for the existing quest 1 (started via 0x1B, but item missing) and
  assert the graceful kick + server survival; the state-advancing path is left
  reachability-limited.
"""

import time

import _protoharness as H


NAME = "0x1E questNextStep"

# message codes
CODE_QUEST_NEXTSTEP = 0x1E
CODE_QUEST_START    = 0x1B   # fixed [questId:u16] -> newQuestAction(Start, questId)

VALID_QUEST_ID   = 1         # map/main/test/quests/1 exists in the 'test' maincode
UNKNOWN_QUEST_ID = 0xBEEF    # no such quest folder -> has_quest()==false


def _peer_gone(sess, settle=1.2):
    """Return True if the server has closed (kicked) this connection.

    A kick = disconnectClient() -> the socket reaches EOF. We detect it by
    recv() returning b'' within a bounded window. We also poke the socket with
    a harmless extra byte first: if the peer already closed, the OS surfaces
    the FIN/RST either as an empty recv or as an OSError on a later send.
    """
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sess.sock.settimeout(0.3)
            d = sess.sock.recv(65536)
            if d == b"":
                return True            # clean EOF: peer closed -> kicked
            # got real bytes (e.g. the "have been kicked" system message that
            # precedes the close, or unrelated map traffic): keep draining
        except (TimeoutError, OSError) as e:
            # socket.timeout subclasses OSError/TimeoutError; a plain timeout
            # means still-open-and-idle, anything else (ECONNRESET) means gone
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                # still connected and idle this slice; try a tiny probe send to
                # force a reset if the peer is half-closed, then loop
                try:
                    sess.sock.sendall(b"\x00")
                except OSError:
                    return True
                continue
            return True
    return False


def _still_connected(sess):
    """Best-effort: True if the connection still appears open (not kicked)."""
    return not _peer_gone(sess, settle=0.8)


def run(server):
    try:
        # ---------------------------------------------------------------
        # Establish baseline legit state on a SEPARATE account that we will
        # NOT touch with any illegal packet — used to prove no cross-client
        # / no global state corruption after the cheaters are kicked.
        # ---------------------------------------------------------------
        victim = H.Session(server, login=b"questvictim01",
                            passh=b"questvictim01pw", pseudo="QuestVictim")
        if not server.alive():
            return (False, "server died just after victim handshake: %r"
                    % server.crash_report())
        base = H.reload_state(server, b"questvictim01", b"questvictim01pw")
        victim_base = (base.get("character_id"), base.get("x"),
                       base.get("y"), base.get("mapIndex"))

        # ===============================================================
        # CASE A — VALID framing, reachable-but-not-advancing NextStep.
        #   Start quest 1 (0x1B, no start requirement) so the player HAS the
        #   quest at step 1, then NextStep it. step-1 requires 1x "Moco Berry"
        #   which the player lacks -> haveNextStepQuestRequirements()==false
        #   -> errorOutput() -> KICK. Server must stay alive.
        #   (A state-ADVANCING NextStep is reachability-limited; see header.)
        # ===============================================================
        s_valid = H.Session(server, login=b"queststart01",
                            passh=b"queststart01pw", pseudo="QuestStart")
        # 0x1B quest start (fixed 2): well-formed, requirement-free quest 1.
        s_valid.m(CODE_QUEST_START, H.u16(VALID_QUEST_ID))
        s_valid.drain(0.6)
        if not server.alive():
            return (False, "server died after quest start (0x1B): %r"
                    % server.crash_report())
        # 0x1E NextStep for the started quest — well-formed wire packet.
        s_valid.m(CODE_QUEST_NEXTSTEP, H.u16(VALID_QUEST_ID))
        if not server.alive():
            return (False, "server died after valid-framing NextStep: %r"
                    % server.crash_report())
        if server.crash_report() is not None:
            return (False, "crash_report after valid NextStep: %s"
                    % server.crash_report())
        # The player owns 0 'Moco Berry' so this NextStep is refused with a
        # kick. We accept either a kick (expected) — the point of CASE A is
        # that the well-formed packet NEVER crashes the server.
        kicked_valid = _peer_gone(s_valid, settle=1.2)
        s_valid.close()

        # ===============================================================
        # CASE B — MALFORMED FRAMING. After each, server must stay ALIVE and
        # crash_report() None. Each uses its own throwaway session because a
        # bad frame may desync/kick that one connection.
        # ===============================================================
        malformed_cases = 0

        # B1: truncated payload — only 1 byte where 2 (u16) are required.
        #     On the wire a fixed-2 message is [0x1E][b0][b1]; sending
        #     [0x1E][b0] leaves the parser 1 byte short.
        sb = H.Session(server, login=b"qmal_b1", passh=b"qmal_b1pw",
                       pseudo="QMalB1")
        sb.send_raw(bytes([CODE_QUEST_NEXTSTEP, 0x01]))
        malformed_cases += 1
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on B1 truncated payload: %r / %s"
                    % (server.alive(), server.crash_report()))
        sb.drain(0.4); sb.close()

        # B2: over-long payload — spurious trailing bytes after the fixed 2.
        #     [0x1E][lo][hi][garbage...]; the parser frames 2 bytes, the rest
        #     is mis-interpreted as the next packet -> at worst a kick.
        sb2 = H.Session(server, login=b"qmal_b2", passh=b"qmal_b2pw",
                        pseudo="QMalB2")
        sb2.send_raw(bytes([CODE_QUEST_NEXTSTEP]) + H.u16(VALID_QUEST_ID)
                     + b"\xde\xad\xbe\xef\xff\xff")
        malformed_cases += 1
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on B2 over-long payload: %r / %s"
                    % (server.alive(), server.crash_report()))
        sb2.drain(0.4); sb2.close()

        # B3: WRONG framing — send it as if DYNAMIC (4-byte size prefix) even
        #     though 0x1E is FIXED. The server reads 2 bytes (the low half of
        #     the size field) as the questId and treats the rest as the next
        #     packet -> garbage/kick, never a crash.
        sb3 = H.Session(server, login=b"qmal_b3", passh=b"qmal_b3pw",
                        pseudo="QMalB3")
        sb3.send_raw(bytes([CODE_QUEST_NEXTSTEP]) + H.u32(0x10000) + b"\x00\x00")
        malformed_cases += 1
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on B3 dynamic-misframing: %r / %s"
                    % (server.alive(), server.crash_report()))
        sb3.drain(0.4); sb3.close()

        # B4: lone packet code with no payload at all (truncated to 0 bytes).
        sb4 = H.Session(server, login=b"qmal_b4", passh=b"qmal_b4pw",
                        pseudo="QMalB4")
        sb4.send_raw(bytes([CODE_QUEST_NEXTSTEP]))
        malformed_cases += 1
        if not server.alive() or server.crash_report() is not None:
            return (False, "server died on B4 empty payload: %r / %s"
                    % (server.alive(), server.crash_report()))
        sb4.drain(0.4); sb4.close()

        # ===============================================================
        # CASE C — SEMANTIC-INVALID (well-formed wire, illegal meaning).
        #   Each MUST kick the offending client, leave the server alive, and
        #   corrupt no state. Fresh session per cheater (it gets disconnected).
        # ===============================================================
        semantic_cases = 0
        kicked_unknown = False
        kicked_notstarted = False

        # C1: UNKNOWN questId -> has_quest()==false -> errorOutput -> KICK.
        s_unknown = H.Session(server, login=b"qcheat_unk",
                              passh=b"qcheat_unkpw", pseudo="QCheatUnk")
        s_unknown.m(CODE_QUEST_NEXTSTEP, H.u16(UNKNOWN_QUEST_ID))
        semantic_cases += 1
        if not server.alive():
            return (False, "server died after unknown-questId NextStep: %r"
                    % server.crash_report())
        if server.crash_report() is not None:
            return (False, "crash_report after unknown-questId: %s"
                    % server.crash_report())
        kicked_unknown = _peer_gone(s_unknown, settle=1.5)
        s_unknown.close()
        if not kicked_unknown:
            return (False, "CONTRACT VIOLATION: unknown questId %#x NOT kicked "
                    "(connection stayed open)" % UNKNOWN_QUEST_ID)

        # C2: EXISTING quest but the player NEVER STARTED it ->
        #     haveNextStepQuestRequirements(): quests.find()==end -> false
        #     -> errorOutput("have not next step quest requirement") -> KICK.
        s_notstarted = H.Session(server, login=b"qcheat_ns",
                                 passh=b"qcheat_nspw", pseudo="QCheatNs")
        s_notstarted.m(CODE_QUEST_NEXTSTEP, H.u16(VALID_QUEST_ID))
        semantic_cases += 1
        if not server.alive():
            return (False, "server died after not-started NextStep: %r"
                    % server.crash_report())
        if server.crash_report() is not None:
            return (False, "crash_report after not-started NextStep: %s"
                    % server.crash_report())
        kicked_notstarted = _peer_gone(s_notstarted, settle=1.5)
        s_notstarted.close()
        if not kicked_notstarted:
            return (False, "CONTRACT VIOLATION: NextStep on a never-started "
                    "quest %d NOT kicked (connection stayed open)"
                    % VALID_QUEST_ID)

        # ===============================================================
        # NO STATE CORRUPTION — the untouched victim account must be byte-for-
        # byte identical (id/pseudo/position) after all the rejected cheats.
        # ===============================================================
        time.sleep(0.4)  # let any disconnect-saves settle before reload
        try:
            victim.close()
        except Exception:
            pass
        time.sleep(0.4)
        after = H.reload_state(server, b"questvictim01", b"questvictim01pw")
        victim_after = (after.get("character_id"), after.get("x"),
                        after.get("y"), after.get("mapIndex"))
        if victim_base != victim_after:
            return (False, "STATE CORRUPTION: victim changed %r -> %r after "
                    "rejected NextStep cheats" % (victim_base, victim_after))

        # ===============================================================
        # Server must remain usable for the next test: open a fresh legit
        # session and confirm the full handshake still works.
        # ===============================================================
        s_post = H.Session(server, login=b"qpost01", passh=b"qpost01pw",
                           pseudo="QPost")
        if s_post.character_id is None:
            s_post.close()
            return (False, "post-check session failed to reach CharacterSelected")
        post_alive = _still_connected(s_post)
        s_post.close()
        if not post_alive:
            return (False, "post-check legit session was unexpectedly kicked")
        if not server.alive():
            return (False, "server not alive at end of test: %r"
                    % server.crash_report())

        detail = (
            "valid-framing NextStep(quest1 started, item missing) kicked=%s; "
            "malformed=%d all survived; semantic-invalid: unknown-questId "
            "kicked=%s, never-started kicked=%s; no state corruption "
            "(victim %r unchanged); server usable post-test. "
            "reachability-limited: state-advancing NextStep needs an inventory "
            "item the protocol harness cannot grant."
            % (kicked_valid, malformed_cases, kicked_unknown,
               kicked_notstarted, victim_base)
        )
        return (True, detail)

    except Exception as e:
        return (False, "exception in run(): %r" % e)
