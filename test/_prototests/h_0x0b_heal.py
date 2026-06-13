#!/usr/bin/env python3
"""
h_0x0b_heal.py — protocol test for server handler 0x0B (heal all the monster).

Handler (server/base/ClientNetworkReadMessage.cpp:394 -> Client::heal() in
server/base/ClientEvents/LocalClientHandlerFightManage.cpp:21):

    case 0x0B: heal(); break;

WIRE SHAPE: 0x0B is a FIXED-size-0 MESSAGE (MSG_FIXED[0x0b]==0). On the wire it
is the single byte 0x0B — no queryNumber, no 4-byte length, no payload. There is
NO reply packet; success heals all monsters server-side (no client reply); every
error path inside heal() calls errorOutput(), which KICKS (disconnects) the
client (server/base/Client.cpp:524). parseMessage() itself always returns true
for 0x0B (the switch falls through to `return true`), so the kick is the only
observable effect on the offending socket.

heal() refuses (and thus errorOutput()->kicks) when:
  * mapIndex>=65535            -> silent return (not reachable here; on map)
  * isInFight()               -> "Try do heal action when is in fight"
  * looking-direction tile is a collision / off-map (mapAndPos...==nullptr)
                              -> "Can't move at this direction to heal"
  * the looking-direction tile is NOT a heal point (map->heal set)
                              -> "no heal point in this direction"

REACHABILITY: heal points come from a bot step type="heal" (MapServer.cpp:236);
in the 'test' maincode the only one is the house2 heal bot, and there is no
deterministic way to walk a fresh spawn to stand FACING that bot from the raw
protocol harness. So a fresh on-map Session is (essentially always) NOT facing a
heal point: the well-formed 0x0B is processed and gracefully refused with a KICK.
That refuse-with-kick IS this handler's semantic-illegal contract (heal where
heal is not permitted), so the "valid" send and the "semantic-invalid" send
coincide for this handler — we exercise it as the semantic-illegal kick case and
mark the successful-heal path as reachability-limited.
"""

import time
import socket
import _protoharness as H

NAME = "0x0B heal"


# ---- small helpers ---------------------------------------------------------

def _is_kicked(sess, settle=2.0):
    """True iff the server kicked THIS session.

    heal()'s "no heal point" / "in fight" rejects call Client::errorOutput()
    (== a kick: full disconnect + state teardown) but the 0x0B dispatch case
    still `break`s and parseMessage returns true, so the TCP FIN is DEFERRED to
    the client's next packet (the input/output layer only closeSocket()s on a
    parse-FALSE). A raw socket-EOF probe is therefore racy here. The
    authoritative, synchronous signal is the server's
    "<pseudo>: Kicked by: ..." log line, which errorOutput() writes the moment
    the kick happens — H.session_was_kicked() polls for it (and nudges the
    socket so a deferred kick still runs its disconnect path)."""
    return H.session_was_kicked(sess, timeout=max(settle, 4.0))


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
        # 0) A separate, untouched legit session (different account) used as
        #    the "no state corruption" witness — it must be unaffected by every
        #    illegal/ malformed thing the cheaters do.
        # ---------------------------------------------------------------
        witness = H.Session(server, pseudo="Witness")
        wx, wy, wmap = witness.x, witness.y, witness.mapIndex

        # ===============================================================
        # 1) VALID well-formed packet (reachability-limited).
        #    A fresh on-map player sends 0x0B. It is correctly framed (single
        #    byte, size 0). Because we cannot deterministically stand on a heal
        #    point in the 'test' maincode, the server refuses with a KICK
        #    ("no heal point in this direction" / "Can't move ... to heal").
        #    We require the SERVER to survive and the offending client to be
        #    kicked (the documented graceful-refusal contract). The
        #    successful-heal branch is reachability-limited and not asserted.
        # ===============================================================
        s_valid = H.Session(server, pseudo="HealV")
        login_v, pass_v = s_valid.login_creds, s_valid.pass_creds
        s_valid.m(0x0B)                      # the heal MESSAGE, well-formed
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid 0x0B: %s" % why
        if not _is_kicked(s_valid):
            # Not kicked could mean we *were* on a heal point (success path):
            # that is also acceptable, but the server MUST still be alive/clean.
            ok, why = _alive_and_clean(server)
            if not ok:
                return False, "valid 0x0B not kicked AND %s" % why
            # If not kicked, the connection is still usable — fine either way.
        s_valid.close()

        # ===============================================================
        # 2) MALFORMED-FRAMING variants. Each on its OWN fresh session so the
        #    survival of the SERVER is what we observe (the offending client
        #    may be kicked). After EACH: server.alive() True and crash_report
        #    None. 0x0B carries no length/qnum of its own, so we malform the
        #    surrounding framing the parser must defend against.
        # ===============================================================
        malformed = [
            # (a) 0x0B immediately followed by a blocked/unknown trailing code
            #     0x01 (client->server reply marker; not a valid inbound here):
            #     parser must frame 0x0B (heal) then reject the 0x01 cleanly.
            b"\x0b\x01",
            # (b) truncated DYNAMIC packet: 0x03 (chat) declares 16 data bytes
            #     but only 2 follow. Parser must wait/not over-read, not crash.
            b"\x03" + H.u32(16) + b"\x00\x01",
            # (c) absurd DYNAMIC length: 0x03 chat claiming ~4 GiB of payload.
            #     Server must not allocate/hang/over-read on the bogus size.
            b"\x03" + H.u32(0xFFFFFFFF),
            # (d) truncated QUERY header: 0xAA (addCharacter, dynamic) with a
            #     queryNumber but no 4-byte length and no data at all.
            b"\xaa\x00",
        ]
        for i, raw in enumerate(malformed):
            sm = H.Session(server, pseudo="HealM%d" % i)
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
        # 3) SEMANTIC-INVALID (the key contract): a well-formed 0x0B sent where
        #    healing is NOT permitted (player is not on a heal point). This is
        #    semantically illegal for THIS handler and MUST:
        #       (a) KICK the offending client,
        #       (b) leave the SERVER alive + clean (+ valgrind clean),
        #       (c) cause NO state corruption — witness + the kicked account's
        #           own persisted identity/position unchanged.
        # ===============================================================
        s_cheat = H.Session(server, pseudo="HealX")
        login_c, pass_c = s_cheat.login_creds, s_cheat.pass_creds
        cx, cy, cmap, cid = s_cheat.x, s_cheat.y, s_cheat.mapIndex, s_cheat.character_id

        s_cheat.m(0x0B)                      # illegal heal (no heal point here)

        # (b) server survives
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after illegal heal: %s" % why

        # (a) the cheater is kicked (socket reaches EOF)
        kicked = _is_kicked(s_cheat)
        s_cheat.close()
        if not kicked:
            return (False,
                    "illegal heal (no heal point) did NOT kick the client "
                    "(connection stayed open) — contract violation")

        # re-check the server is still healthy after the kick
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after illegal-heal kick: %s" % why

        # (c) NO STATE CORRUPTION
        #  - witness (a different account, still connected) is untouched: same
        #    spawn position, server still serving it.
        if (witness.x, witness.y, witness.mapIndex) != (wx, wy, wmap):
            return (False,
                    "witness position changed by another client's illegal heal: "
                    "%r -> %r" % ((wx, wy, wmap),
                                  (witness.x, witness.y, witness.mapIndex)))
        # the witness socket must still be open (not collaterally kicked)
        if _is_kicked(witness, settle=0.8):
            return False, "witness was collaterally kicked by cheater's illegal heal"

        #  - the kicked account's PERSISTED character is intact: the rejected
        #    heal must not have moved it or altered its identity. Give the
        #    disconnect-save a moment, then read it back.
        witness.close()
        time.sleep(0.4)
        snap = H.reload_state(server, login_c, pass_c)
        if snap.get("character_id") is None:
            # account/character must still exist after a rejected action
            return (False,
                    "kicked account's character vanished after illegal heal "
                    "(reload_state could not read it back): %r" % snap)
        if cid is not None and snap.get("character_id") != cid:
            return (False,
                    "kicked account's character_id changed: %r -> %r"
                    % (cid, snap.get("character_id")))
        if cmap is not None and cx is not None and cy is not None:
            if (snap.get("x"), snap.get("y"), snap.get("mapIndex")) != (cx, cy, cmap):
                return (False,
                        "kicked account's persisted position mutated by the "
                        "rejected heal: %r -> %r"
                        % ((cx, cy, cmap),
                           (snap.get("x"), snap.get("y"), snap.get("mapIndex"))))

        # ===============================================================
        # 4) leave the server usable: prove a fresh legit Session still works.
        # ===============================================================
        post = H.Session(server, pseudo="HealPost")
        ok, why = _alive_and_clean(server)
        post.close()
        if not ok:
            return False, "server unusable after the test sequence: %s" % why

        return (True,
                "0x0B heal: well-formed send survives (kick when not on a heal "
                "point — successful-heal path reachability-limited in 'test'); "
                "4 malformed-framing variants each kept the server alive+clean; "
                "illegal heal (no heal point) KICKS the cheater, server survives, "
                "and no state corruption (witness + kicked account's persisted "
                "id/position intact)")
    except H.HandshakeError as e:
        return False, "handshake failed: %r" % e
    except Exception as e:
        import traceback
        return False, "unexpected exception: %r\n%s" % (e, traceback.format_exc()[-800:])
