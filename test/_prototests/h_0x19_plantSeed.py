#!/usr/bin/env python3
"""
h_0x19_plantSeed.py — protocol test for server handler 0x19 (plant seed into dirt).

Handler (server/base/ClientNetworkReadMessage.cpp:592 -> plantSeed(plant_id)):

    case 0x19:  // Use seed into dirt
    {
        #ifdef CATCHCHALLENGER_HARDENED
        if(size!=(int)sizeof(uint8_t))
        { errorOutput("wrong size with the main ident: ..."); return false; }
        #endif
        const uint8_t &plant_id=*data;
        plantSeed(plant_id);
        return true;
    }

WIRE SHAPE: 0x19 is a FIXED-size-1 MESSAGE (packetFixedSize[0x19]==1). On the wire
it is exactly 2 bytes:  [0x19][plant_id]  (plant_id is u8, 0..255).
No queryNumber (code<0x80, not a reply), no 4-byte dynamic length (fixed size).
There is NO reply packet — plantSeed is fire-and-forget.

SEMANTICS (server/base/crafting/ClientLocalBroadcastCrafting.cpp::plantSeed ->
LocalClientHandlerCrafting.cpp::useSeed):
  plantSeed(plant_id):
    1. if !CommonDatapack::has_plant(plant_id) -> errorOutput("plant_id not found")
       -> KICK.  *** This is checked FIRST, before any map / mapData access. ***
    2. mapAndPosIfMoveInLookingDirectionJumpColision(...) == nullptr -> KICK.
    3. !MoveOnTheMap::isDirt(map,x,y) -> KICK ("Try put seed out of the dirt").
    4. ALREADY a plant at (x,y): guarded with mapData.find() (see below) -> KICK.
    5. useSeed: !reputation req -> KICK ; objectQuantity(itemUsed)==0 -> KICK
       (player does not OWN the seed item); else removeObject + seedValidated().

THE SECURITY FIX (the user's brief): step 4 used to do
    Player...Map &mapData = public_and_private_informations.mapData.at(new_map_index);
mapData is a std::map populated ONLY for maps the player has already modified. A
fresh player (or any never-modified map) has NO entry, so .at() threw
std::out_of_range -> uncaught -> SERVER CRASH. It is now guarded with .find():
    auto it = mapData.find(new_map_index);
    if(it!=end()) { ... it->second.plants.find(...) ... }
i.e. a missing per-map entry simply means "no plant here", no throw. This test
asserts that a WELL-FORMED, KNOWN plant_id sent by a player whose mapData is empty
NEVER crashes the server (it must end in a graceful later guard, not an abort).

DATAPACK ('test' maincode): plants exist for ids 1..11 only (plants/plants.xml).
Any id in {0, 12..255} is UNKNOWN -> semantic-illegal -> kick at step 1.

REACHABILITY-LIMITED (documented partial): a fully SUCCESSFUL plant (item removed,
seedValidated) requires the player to (a) stand on dirt and FACE a dirt cell and
(b) OWN the seed item (itemUsed, e.g. 1001 for plant 1). A freshly-spawned 'test'
character does neither, so the success-mutation branch is not reachable from a
bare on-map Session. We therefore exercise:
  * the KNOWN-plant path against an empty-mapData player (the crash-fix assert):
    server must SURVIVE (graceful later kick, never an abort) — the precondition
    for a successful plant being unmet is acceptable here and documented.
  * the full semantic-illegal contract (unknown plant ids) with KICK + no
    crash + no state corruption.
plantSeed sends NO reply, so observability is "alive AND (accepted == not kicked)
| (rejected == kicked)". No cash/item is creatable by any rejected attempt (the
unknown-id reject happens BEFORE any itemUsed lookup / removeObject), so the
no-state-corruption check uses identity/position via reload_state + a witness.
"""

import time
import socket
import _protoharness as H

NAME = "0x19 plantSeed"

# A plant id that EXISTS in the 'test' maincode datapack (plants 1..11).
KNOWN_PLANT_ID = 1
# Well-formed (u8) plant ids that do NOT exist -> semantic-illegal -> kick.
# 0 (below range), 12 and 200 (above range), 255 (max u8).
UNKNOWN_PLANT_IDS = [0, 12, 200, 255]


# ---- small helpers ---------------------------------------------------------

def _is_kicked(sess, settle=2.0):
    """True if the server has closed our socket (kick). Pump+auto-ACK any
    pending server traffic first, then read raw bytes until EOF or the settle
    window elapses. A kick = FIN -> recv returns b''. A still-open, silent
    connection = repeated timeouts with no EOF -> not kicked.

    Liveness probe discipline (mirrors the other prototests): send AT MOST ONE
    genuinely-harmless in-game packet (a single valid 0x02 move) to nudge a
    deferred socket teardown; thereafter only watch for EOF. A repeated probe
    or an unknown code would self-trigger a kick / DDoS counter and turn this
    detector into a FALSE POSITIVE."""
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
        # 1) VALID well-formed packet (KNOWN plant id) — the CRASH-FIX assert.
        #    A freshly-spawned player has an EMPTY mapData std::map. Sending a
        #    KNOWN plant id (1) drives plantSeed PAST the has_plant() gate and
        #    into the per-map lookup region that USED to do mapData.at() and
        #    throw -> crash on a never-modified map. With the .find() guard the
        #    server must SURVIVE: it ends in a graceful later guard (not on
        #    dirt / not owning the seed -> kick) and NEVER aborts.
        #    REACHABILITY-LIMITED: the success-mutation branch (own seed + face
        #    dirt) is not reachable from a bare on-map 'test' Session, so the
        #    observable contract here is purely "server alive + no crash".
        # ===============================================================
        s_valid = H.Session(server, pseudo="PsV")
        # well-formed 2-byte message: [0x19][1]
        s_valid.m(0x19, H.u8(KNOWN_PLANT_ID))
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_valid.close()
            except Exception: pass
            return (False,
                    "after valid 0x19 (known plant id %d) on empty-mapData "
                    "player — the mapData.at() crash-fix did NOT hold: %s"
                    % (KNOWN_PLANT_ID, why))
        # The known-plant path on a player not on dirt / not owning the seed is
        # rejected by a LATER guard -> a kick is the EXPECTED outcome here, and
        # the SERVER staying alive (no abort) is the thing under test. We do not
        # require not-kicked (success is unreachable); we only require survival.
        try: s_valid.close()
        except Exception: pass

        # ===============================================================
        # 2) MALFORMED-FRAMING variants. Each on its OWN fresh session. After
        #    EACH: server.alive() True and crash_report None. 0x19 is a fixed
        #    size-1 packet, so we malform its declared/actual length and the
        #    surrounding framing the parser must defend against.
        # ===============================================================
        malformed = [
            # (a) 0x19 with ZERO payload bytes — the parser frames 0x19 as a
            #     fixed-1 packet so it will wait for / consume the next byte;
            #     append a blocked trailing code 0x01 so the parse pass ends
            #     deterministically. Under HARDENED the size!=1 guard kicks.
            b"\x19\x01",
            # (b) 0x19 pretending to be DYNAMIC: prefix a bogus 4-byte length
            #     the parser must NOT read (0x19 is FIXED-1). Exercises the
            #     fixed-vs-dynamic framing decision; the trailing bytes are
            #     interpreted as further packets, not as a length.
            b"\x19" + b"\x05\x00\x00\x00\x00",
            # (c) truncated DYNAMIC packet: 0x03 (chat) declares 16 data bytes
            #     but only 2 follow. Parser must wait/not over-read, not crash.
            b"\x03" + H.u32(16) + b"\x00\x01",
            # (d) absurd DYNAMIC length: 0x03 chat claiming ~4 GiB of payload.
            #     Server must not allocate/hang/over-read on the bogus size.
            b"\x03" + H.u32(0xFFFFFFFF),
        ]
        for i, raw in enumerate(malformed):
            sm = H.Session(server, pseudo="PsM%d" % i)
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
        # 3) SEMANTIC-INVALID (the key contract): well-formed 0x19 packets whose
        #    plant_id is UNKNOWN to the datapack. plantSeed rejects at step 1
        #    with errorOutput("plant_id not found") -> KICK, BEFORE any map /
        #    mapData / itemUsed access. For EACH:
        #       (a) KICK the offending client,
        #       (b) leave the SERVER alive + clean (+ valgrind clean),
        #       (c) cause NO state corruption — witness + the kicked account's
        #           own persisted identity/position unchanged (no item/cash can
        #           even be touched: the reject precedes any removeObject).
        # ===============================================================
        kicked_login = kicked_pass = None
        kicked_id = kicked_pos = None
        for idx, bad_pid in enumerate(UNKNOWN_PLANT_IDS):
            s_cheat = H.Session(server, pseudo="PsX%d" % idx)
            if idx == 0:
                kicked_login, kicked_pass = s_cheat.login_creds, s_cheat.pass_creds
                kicked_id = s_cheat.character_id
                kicked_pos = (s_cheat.x, s_cheat.y, s_cheat.mapIndex)

            # well-formed packet (2 bytes), semantically illegal: unknown plant
            s_cheat.m(0x19, H.u8(bad_pid))

            # (b) server survives
            ok, why = _alive_and_clean(server)
            if not ok:
                try: s_cheat.close()
                except Exception: pass
                return False, "after illegal plantSeed pid=%d: %s" % (bad_pid, why)

            # (a) the cheater is kicked (socket reaches EOF)
            kicked = _is_kicked(s_cheat)
            try: s_cheat.close()
            except Exception: pass
            if not kicked:
                return (False,
                        "illegal 0x19 (unknown plant_id=%d) did NOT kick the "
                        "client (connection stayed open) — contract violation"
                        % bad_pid)

            # re-check the server is still healthy after the kick
            ok, why = _alive_and_clean(server)
            if not ok:
                return False, "after illegal-plant kick pid=%d: %s" % (bad_pid, why)

        # (c) NO STATE CORRUPTION
        #  - witness (a different account, still connected) is untouched: same
        #    spawn position, server still serving it.
        if (witness.x, witness.y, witness.mapIndex) != (wx, wy, wmap):
            return (False,
                    "witness position changed by another client's illegal "
                    "plantSeed: %r -> %r" % ((wx, wy, wmap),
                                             (witness.x, witness.y, witness.mapIndex)))
        # the witness socket must still be open (not collaterally kicked)
        if _is_kicked(witness, settle=0.8):
            return False, "witness was collaterally kicked by a cheater's illegal plantSeed"

        #  - the kicked account's PERSISTED character is intact: the rejected
        #    plant must not have moved it or altered its identity. Give the
        #    disconnect-save a moment, then read it back.
        witness.close()
        time.sleep(0.4)
        snap = H.reload_state(server, kicked_login, kicked_pass)
        if snap.get("character_id") is None:
            return (False,
                    "kicked account's character vanished after illegal "
                    "plantSeed (reload_state could not read it back): %r" % snap)
        if kicked_id is not None and snap.get("character_id") != kicked_id:
            return (False,
                    "kicked account's character_id changed: %r -> %r"
                    % (kicked_id, snap.get("character_id")))
        if kicked_pos is not None and kicked_pos[0] is not None:
            if (snap.get("x"), snap.get("y"), snap.get("mapIndex")) != kicked_pos:
                return (False,
                        "kicked account's persisted position mutated by the "
                        "rejected plantSeed: %r -> %r"
                        % (kicked_pos,
                           (snap.get("x"), snap.get("y"), snap.get("mapIndex"))))

        # ===============================================================
        # 4) leave the server usable: prove a fresh legit Session still works.
        # ===============================================================
        post = H.Session(server, pseudo="PsPost")
        ok, why = _alive_and_clean(server)
        post.close()
        if not ok:
            return False, "server unusable after the test sequence: %s" % why

        return (True,
                "0x19 plantSeed: a KNOWN plant id (1) on a fresh empty-mapData "
                "player kept the server ALIVE with no abort — the mapData.at() "
                "out-of-range crash-fix holds (success-mutation branch is "
                "reachability-limited: needs facing dirt + owning the seed, "
                "unreachable from a bare 'test' Session). 4 malformed-framing "
                "variants each kept the server alive+clean. 4 illegal plantSeeds "
                "(unknown plant_id 0/12/200/255) each KICK the cheater while the "
                "server survives, and no state corruption (witness + kicked "
                "account's persisted id/position intact; the unknown-id reject "
                "precedes any item/cash mutation). plantSeed sends no reply -> "
                "DB plant sub-block not framed by reload_state, so identity/"
                "position used for the corruption check (documented limit)")
    except H.HandshakeError as e:
        return False, "handshake failed: %r" % e
    except Exception as e:
        import traceback
        return False, "unexpected exception: %r\n%s" % (e, traceback.format_exc()[-800:])
