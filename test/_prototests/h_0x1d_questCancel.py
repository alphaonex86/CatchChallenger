#!/usr/bin/env python3
"""
h_0x1d_questCancel.py — protocol test for server handler 0x1D (quest cancel).

Handler (server/base/ClientNetworkReadMessage.cpp:641 -> newQuestAction(Cancel)):

    case 0x1D:  // Quest cancel
    {
        #ifdef CATCHCHALLENGER_HARDENED
        if(size!=((int)sizeof(uint16_t)))
        { errorOutput("wrong remaining size for quest cancel"); return false; }
        #endif
        const uint16_t &questId=CatchChallenger::loadLe16(data);
        newQuestAction(QuestAction_Cancel,questId);
        return true;
    }

WIRE SHAPE: 0x1D is a FIXED-size-2 MESSAGE (packetFixedSize[0x1D]==2,
ProtocolParsingGeneral.cpp:94). On the wire it is exactly 3 bytes:
    [0x1D][questId_lo][questId_hi]      (questId is u16, LITTLE-ENDIAN)
No queryNumber (code<0x80, not a reply), no 4-byte dynamic length (fixed size).
There is NO reply packet — quest cancel is fire-and-forget.

SEMANTICS (server/base/ClientEvents/LocalClientHandlerQuest.cpp):
  newQuestAction(Cancel, questId):
    * if !has_quest(questId)  -> errorOutput("unknown questId: ...") -> KICK
    * else cancelQuest(questId):  public_and_private_informations.quests.erase(questId);
                                  syncDatabaseQuest();
  cancelQuest() is a pure erase: it removes the quest if present (a no-op if the
  player never started it) and re-syncs the quest block to the DB. It NEVER
  errors / kicks for an EXISTING-but-not-owned quest id — erasing a non-present
  key is harmless. So the ONLY kick path for 0x1D is an UNKNOWN quest id (a quest
  id that does not exist in the datapack at all).

DATAPACK ('test' maincode): exactly ONE quest exists, id 1 ("Berry exchange",
repeatable). Any other id (e.g. 9999, 0xFFFF, 0) is unknown -> kick.

OBSERVABILITY: questCancel sends no reply, so the VALID case is observed as
"server alive AND client NOT kicked" (the message is accepted and processed). The
quest map is persisted into the HPS character block at disconnect, but
H.reload_state only frames back id/pseudo/position (DOCUMENTED LIMIT), not the
quest sub-block — so the quest mutation itself is asserted in-band (no kick =
accepted) and the DB check is used for IDENTITY/POSITION-not-corrupted only.
"""

import time
import socket
import _protoharness as H

NAME = "0x1D questCancel"

# Quest id known to EXIST in the 'test' maincode datapack.
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
        #    Start quest 1 (0x1B), then CANCEL quest 1 (0x1D). Both are
        #    well-formed FIXED-size-2 messages. cancelQuest() erases the quest
        #    and re-syncs the DB; there is NO reply, so the contract is: the
        #    server processes it WITHOUT kicking. We also send a second cancel
        #    of the SAME id (now not present) to prove the no-op erase path is
        #    likewise accepted (not kicked).
        # ===============================================================
        s_valid = H.Session(server, pseudo="QcV")
        login_v, pass_v = s_valid.login_creds, s_valid.pass_creds
        cidv, cxv, cyv, cmapv = s_valid.character_id, s_valid.x, s_valid.y, s_valid.mapIndex

        # start quest 1 (id LE u16) so the cancel has something to erase
        s_valid.m(0x1B, H.u16(VALID_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid 0x1B (start quest 1): %s" % why

        # cancel quest 1 — the handler under test
        s_valid.m(0x1D, H.u16(VALID_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid 0x1D (cancel quest 1): %s" % why
        # accepted == NOT kicked (no reply for this handler)
        if _is_kicked(s_valid, settle=1.2):
            return (False,
                    "valid 0x1D (cancel existing quest id 1) wrongly KICKED the "
                    "client — should be accepted silently")

        # cancel the same id again (now not present) — must still be a no-op,
        # never a kick (erase of a missing key is legal).
        s_valid.m(0x1D, H.u16(VALID_QUEST_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after second valid 0x1D (cancel not-present quest): %s" % why
        if _is_kicked(s_valid, settle=1.2):
            return (False,
                    "valid 0x1D (cancel not-present quest id 1) wrongly KICKED — "
                    "erasing a missing key must be a harmless no-op")
        s_valid.close()

        # ===============================================================
        # 2) MALFORMED-FRAMING variants. Each on its OWN fresh session. After
        #    EACH: server.alive() True and crash_report None. 0x1D is a fixed
        #    size-2 packet, so we malform its declared/actual length and the
        #    surrounding framing the parser must defend against.
        # ===============================================================
        malformed = [
            # (a) 0x1D with only ONE payload byte instead of two: the parser
            #     frames 0x1D as a fixed-2 packet, so it will wait for / consume
            #     the next byte. Append a blocked trailing code 0x01 so the parse
            #     pass terminates deterministically; under HARDENED the
            #     size!=2 guard kicks. Server must not crash.
            b"\x1d\x01",
            # (b) 0x1D claiming to be a DYNAMIC packet by prefixing a bogus
            #     4-byte length the parser must NOT read (0x1D is FIXED). We send
            #     0x1D then a single byte, then garbage — exercises the
            #     fixed-vs-dynamic framing decision.
            b"\x1d" + b"\x01\x00\x00\x00\x00",
            # (c) truncated DYNAMIC packet: 0x03 (chat) declares 16 data bytes
            #     but only 2 follow. Parser must wait/not over-read, not crash.
            b"\x03" + H.u32(16) + b"\x00\x01",
            # (d) absurd DYNAMIC length: 0x03 chat claiming ~4 GiB of payload.
            #     Server must not allocate/hang/over-read on the bogus size.
            b"\x03" + H.u32(0xFFFFFFFF),
        ]
        for i, raw in enumerate(malformed):
            sm = H.Session(server, pseudo="QcM%d" % i)
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
        # 3) SEMANTIC-INVALID (the key contract): well-formed 0x1D packets whose
        #    questId is UNKNOWN to the datapack (does not exist). newQuestAction
        #    rejects with errorOutput("unknown questId") -> KICK. For EACH:
        #       (a) KICK the offending client,
        #       (b) leave the SERVER alive + clean (+ valgrind clean),
        #       (c) cause NO state corruption — witness + the kicked account's
        #           own persisted identity/position unchanged.
        # ===============================================================
        kicked_login = kicked_pass = None
        kicked_id = kicked_pos = None
        for idx, bad_qid in enumerate(UNKNOWN_QUEST_IDS):
            s_cheat = H.Session(server, pseudo="QcX%d" % idx)
            if idx == 0:
                kicked_login, kicked_pass = s_cheat.login_creds, s_cheat.pass_creds
                kicked_id = s_cheat.character_id
                kicked_pos = (s_cheat.x, s_cheat.y, s_cheat.mapIndex)

            # well-formed packet (3 bytes), semantically illegal: unknown quest
            s_cheat.m(0x1D, H.u16(bad_qid))

            # (b) server survives
            ok, why = _alive_and_clean(server)
            if not ok:
                try: s_cheat.close()
                except Exception: pass
                return False, "after illegal cancel qid=%d: %s" % (bad_qid, why)

            # (a) the cheater is kicked (socket reaches EOF)
            kicked = _is_kicked(s_cheat)
            try: s_cheat.close()
            except Exception: pass
            if not kicked:
                return (False,
                        "illegal 0x1D (unknown questId=%d) did NOT kick the "
                        "client (connection stayed open) — contract violation"
                        % bad_qid)

            # re-check the server is still healthy after the kick
            ok, why = _alive_and_clean(server)
            if not ok:
                return False, "after illegal-cancel kick qid=%d: %s" % (bad_qid, why)

        # (c) NO STATE CORRUPTION
        #  - witness (a different account, still connected) is untouched: same
        #    spawn position, server still serving it.
        if (witness.x, witness.y, witness.mapIndex) != (wx, wy, wmap):
            return (False,
                    "witness position changed by another client's illegal cancel: "
                    "%r -> %r" % ((wx, wy, wmap),
                                  (witness.x, witness.y, witness.mapIndex)))
        # the witness socket must still be open (not collaterally kicked)
        if _is_kicked(witness, settle=0.8):
            return False, "witness was collaterally kicked by a cheater's illegal cancel"

        #  - the kicked account's PERSISTED character is intact: the rejected
        #    cancel must not have moved it or altered its identity. Give the
        #    disconnect-save a moment, then read it back.
        witness.close()
        time.sleep(0.4)
        snap = H.reload_state(server, kicked_login, kicked_pass)
        if snap.get("character_id") is None:
            return (False,
                    "kicked account's character vanished after illegal cancel "
                    "(reload_state could not read it back): %r" % snap)
        if kicked_id is not None and snap.get("character_id") != kicked_id:
            return (False,
                    "kicked account's character_id changed: %r -> %r"
                    % (kicked_id, snap.get("character_id")))
        if kicked_pos is not None and kicked_pos[0] is not None:
            if (snap.get("x"), snap.get("y"), snap.get("mapIndex")) != kicked_pos:
                return (False,
                        "kicked account's persisted position mutated by the "
                        "rejected cancel: %r -> %r"
                        % (kicked_pos,
                           (snap.get("x"), snap.get("y"), snap.get("mapIndex"))))

        # ===============================================================
        # 4) leave the server usable: prove a fresh legit Session still works.
        # ===============================================================
        post = H.Session(server, pseudo="QcPost")
        ok, why = _alive_and_clean(server)
        post.close()
        if not ok:
            return False, "server unusable after the test sequence: %s" % why

        return (True,
                "0x1D questCancel: valid cancel of existing quest id 1 (after a "
                "0x1B start) is accepted WITHOUT a kick, and a repeat cancel of "
                "the now-absent quest is a harmless no-op (no reply by design); "
                "4 malformed-framing variants each kept the server alive+clean; "
                "3 illegal cancels (unknown questId 9999/0xFFFF/0) each KICK the "
                "cheater while the server survives, and no state corruption "
                "(witness + kicked account's persisted id/position intact). "
                "DB-quest sub-block not framed by reload_state -> identity/"
                "position used for the corruption check (documented limit)")
    except H.HandshakeError as e:
        return False, "handshake failed: %r" % e
    except Exception as e:
        import traceback
        return False, "unexpected exception: %r\n%s" % (e, traceback.format_exc()[-800:])
