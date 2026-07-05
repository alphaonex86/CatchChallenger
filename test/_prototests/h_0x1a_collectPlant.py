#!/usr/bin/env python3
"""
h_0x1a_collectPlant.py — protocol test for server handler 0x1A (collect plant).

Handler (server/base/ClientNetworkReadMessage.cpp:607):

    //Collect mature plant
    case 0x1A:
        collectPlant();
    break;

WIRE SHAPE: 0x1A is a FIXED-size-0 MESSAGE (packetFixedSize[0x1A]==0,
ProtocolParsingGeneral.cpp). On the wire it is exactly ONE byte:
    [0x1A]
No queryNumber (code<0x80, not a reply), and NO 4-byte dynamic length (fixed
size, not 0xFE). The packet carries NO payload at all — collectPlant() reads
nothing from the wire; it operates on the cell the player is currently LOOKING
at. There is also NO size guard in the dispatch (unlike 0x19/0x1B): the parser
frames it as a zero-length packet, so any trailing byte is the START of the next
packet, never part of 0x1A.

SEMANTICS (server/base/crafting/ClientLocalBroadcastCrafting.cpp:121 collectPlant()):
    new_map = mapAndPosIfMoveInLookingDirectionJumpColision(...)
    if(new_map==nullptr)              errorOutput("Can't move at this direction") -> KICK
    if(!isDirt(*new_map,x,y))         errorOutput("Try collect plant out of the dirt") -> KICK
    mapData = public_and_private_informations.mapData[new_map_index]
    if(mapData.plants has (x,y)):
        plant = ...
        if(!has_plant(plant.plant))   errorOutput("plant not found to collect") -> KICK
        if(now < plant.mature_at)     errorOutput("current_time<plant.mature_at") -> KICK
        erase plant; addObjectAndSend(itemUsed, quantity)   <-- the ONLY success path
    else:
        errorOutput("!...plantOnMap.contains...") -> KICK

errorOutput() ALWAYS calls disconnectClient() (server/base/Client.cpp:524) — so
EVERY semantic-reject path KICKS the offending client; the server stays alive
(production-safe graceful disconnect). collectPlant() returns void, so the parse
layer never sees `false` and HARDENED does NOT abort — the kick is the contract.

REACHABILITY-LIMITED VALID CASE: the success path needs (1) the player to be
FACING a DIRT cell and (2) a MATURED plant already at that cell. A plant only
exists after a prior 0x19 plantSeed onto dirt-in-front AND waiting
fruits_seconds for it to mature — neither of which is deterministically set up
for a freshly-spawned player on the 'test' maincode map (the spawn cell is not
guaranteed to face dirt, and we cannot fast-forward maturation). So a genuine
"collect a mature plant" cannot be staged here. We instead exercise the handler
exactly as a real client would for an EMPTY/illegal cell: a well-formed single
0x1A byte. For a fresh on-map player every such send is semantically rejected
(not-dirt OR no-plant-present) and therefore KICKS the sender — which is itself
the right, safe behaviour to verify (server refuses gracefully, never crashes).
Documented as 'reachability-limited': the success branch is unreachable in the
'test' maincode, so the test asserts the refuse-and-kick contract + no crash +
no state corruption, which is the entirety of what 0x1A can do here.

OBSERVABILITY: collectPlant() emits NO reply on any reject path (and its only
side effect on success is the inventory-add, which we can't reach). So the
observable contract is purely: SERVER ALIVE + offending client KICKED + NO state
corruption (a separate witness account and the kicked account's persisted
identity/position remain intact). reload_state frames back id/pseudo/position
only (DOCUMENTED LIMIT — inventory/cash/plants live deeper in the HPS block), so
the corruption check uses identity/position; no inventory mutation can occur
anyway because the collect never succeeds.
"""

import time
import socket
import _protoharness as H

NAME = "0x1A collectPlant"


# ---- small helpers ---------------------------------------------------------

def _is_kicked(sess, settle=2.0):
    """True if the server has closed our socket (kick). Pump+auto-ACK any
    pending server traffic first, then read raw bytes until EOF or the settle
    window elapses. A kick = FIN -> recv returns b''. A still-open, silent
    connection = repeated timeouts with no EOF -> not kicked.

    Liveness probe discipline: send AT MOST ONE genuinely-harmless in-game
    packet (a single valid 0x02 move) to nudge a deferred socket teardown.
    DO NOT send a probe on every timeout slice and DO NOT send an unknown code:
    a repeated probe trips the server's per-packet DDoS counter and an unknown
    code self-triggers a KICK ("unknown main ident") — either turns this
    detector into a FALSE POSITIVE on a client the server never kicked. One
    harmless move is enough; after it we only watch for EOF."""
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
        #    0x1A is a one-byte fixed-size-0 message; a real client sends it
        #    verbatim. For a fresh on-map player the collect target cell holds
        #    no mature plant (the success branch is unreachable in 'test', see
        #    module docstring), so the handler refuses and KICKS the sender.
        #    The contract we CAN assert: the well-formed packet keeps the SERVER
        #    alive+clean and the refusal is the documented graceful disconnect,
        #    not a crash/hang. We record the account so we can re-read its
        #    persisted state afterwards and prove nothing was mutated.
        # ===============================================================
        s_valid = H.Session(server, pseudo="CpV")
        valid_login, valid_pass = s_valid.login_creds, s_valid.pass_creds
        valid_id = s_valid.character_id
        valid_pos = (s_valid.x, s_valid.y, s_valid.mapIndex)

        # well-formed single byte, sent through the normal message path
        s_valid.m(0x1A)
        ok, why = _alive_and_clean(server)
        if not ok:
            try: s_valid.close()
            except Exception: pass
            return False, "after valid 0x1A (one-byte collect): %s" % why

        # the fresh player faces no mature plant -> graceful refuse == KICK.
        valid_kicked = _is_kicked(s_valid, settle=2.0)
        try: s_valid.close()
        except Exception: pass
        if not valid_kicked:
            # If a future 'test' map ever DID let this succeed, the server would
            # keep the connection open with no reply — that is also acceptable
            # (the collect simply happened). But it must NEVER crash; that is
            # already verified above. So treat a non-kick as a benign accept and
            # do not fail. (Documented: success branch unreachable here.)
            pass
        ok, why = _alive_and_clean(server)
        if not ok:
            return False, "after valid 0x1A refuse/kick: %s" % why

        # ===============================================================
        # 2) MALFORMED-FRAMING variants. Each on its OWN fresh session. After
        #    EACH: server.alive() True and crash_report None. 0x1A is a fixed
        #    size-0 packet, so we malform the surrounding framing the parser
        #    must defend against (spurious payload, fake dynamic length, a
        #    truncated/absurd dynamic neighbour packet).
        # ===============================================================
        malformed = [
            # (a) 0x1A with a SPURIOUS trailing byte. 0x1A consumes zero bytes,
            #     so the parser treats the next byte as a new packet code. 0x01
            #     is a blocked/unknown code (fixedsize 0xFF) -> the parse pass
            #     terminates / the client is kicked. Server must not crash.
            b"\x1a\x01",
            # (b) 0x1A pretending to be a DYNAMIC packet by prefixing a bogus
            #     4-byte length the parser must NOT read (0x1A is FIXED size 0).
            #     Exercises the fixed-vs-dynamic framing decision: the 4 bytes
            #     are parsed as four further packet codes, not as a length.
            b"\x1a" + b"\x04\x00\x00\x00",
            # (c) truncated DYNAMIC neighbour: 0x03 (chat) declares 16 data
            #     bytes but only 2 follow. Parser must wait / not over-read, not
            #     crash, on a starved dynamic length.
            b"\x03" + H.u32(16) + b"\x00\x01",
            # (d) absurd DYNAMIC length: 0x03 chat claiming ~4 GiB of payload.
            #     Server must not allocate / hang / over-read on the bogus size.
            b"\x03" + H.u32(0xFFFFFFFF),
        ]
        for i, raw in enumerate(malformed):
            sm = H.Session(server, pseudo="CpM%d" % i)
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
        # 3) SEMANTIC-INVALID (the key contract): well-formed one-byte 0x1A
        #    packets that are semantically illegal — a collect attempt where
        #    there is NO mature plant at the looked-at cell (the only state a
        #    fresh on-map player can be in; the cell is either not dirt or has
        #    no plant). collectPlant() rejects with errorOutput(...) -> KICK.
        #    Sending it repeatedly (each on a fresh cheater session) proves the
        #    handler kicks deterministically on every illegal collect. For EACH:
        #       (a) KICK the offending client,
        #       (b) leave the SERVER alive + clean (+ valgrind clean),
        #       (c) cause NO state corruption — witness + the kicked account's
        #           own persisted identity/position unchanged.
        # ===============================================================
        kicked_login = kicked_pass = None
        kicked_id = kicked_pos = None
        for idx in range(3):
            s_cheat = H.Session(server, pseudo="CpX%d" % idx)
            if idx == 0:
                kicked_login, kicked_pass = s_cheat.login_creds, s_cheat.pass_creds
                kicked_id = s_cheat.character_id
                kicked_pos = (s_cheat.x, s_cheat.y, s_cheat.mapIndex)

            # well-formed single byte, semantically illegal: collect with no
            # mature plant in front of the player.
            s_cheat.m(0x1A)

            # (b) server survives
            ok, why = _alive_and_clean(server)
            if not ok:
                try: s_cheat.close()
                except Exception: pass
                return False, "after illegal collect #%d: %s" % (idx, why)

            # (a) the cheater is kicked (socket reaches EOF)
            kicked = _is_kicked(s_cheat)
            try: s_cheat.close()
            except Exception: pass
            if not kicked:
                return (False,
                        "illegal 0x1A (collect with no mature plant in front) #%d "
                        "did NOT kick the client (connection stayed open) — "
                        "contract violation" % idx)

            # re-check the server is still healthy after the kick
            ok, why = _alive_and_clean(server)
            if not ok:
                return False, "after illegal-collect kick #%d: %s" % (idx, why)

        # (c) NO STATE CORRUPTION
        #  - witness (a different account, still connected) is untouched: same
        #    spawn position, server still serving it.
        if (witness.x, witness.y, witness.mapIndex) != (wx, wy, wmap):
            return (False,
                    "witness position changed by another client's illegal collect: "
                    "%r -> %r" % ((wx, wy, wmap),
                                  (witness.x, witness.y, witness.mapIndex)))
        # the witness socket must still be open (not collaterally kicked)
        if _is_kicked(witness, settle=0.8):
            return False, "witness was collaterally kicked by a cheater's illegal collect"

        #  - the kicked account's PERSISTED character is intact: the rejected
        #    collect must not have moved it or altered its identity. Give the
        #    disconnect-save a moment, then read it back.
        witness.close()
        time.sleep(0.4)
        snap = H.reload_state(server, kicked_login, kicked_pass)
        if snap.get("character_id") is None:
            return (False,
                    "kicked account's character vanished after illegal collect "
                    "(reload_state could not read it back): %r" % snap)
        if kicked_id is not None and snap.get("character_id") != kicked_id:
            return (False,
                    "kicked account's character_id changed: %r -> %r"
                    % (kicked_id, snap.get("character_id")))
        if kicked_pos is not None and kicked_pos[0] is not None:
            if (snap.get("x"), snap.get("y"), snap.get("mapIndex")) != kicked_pos:
                return (False,
                        "kicked account's persisted position mutated by the "
                        "rejected collect: %r -> %r"
                        % (kicked_pos,
                           (snap.get("x"), snap.get("y"), snap.get("mapIndex"))))

        #  - the VALID-case account (also kicked, since success was unreachable)
        #    persisted intact too: no phantom item/position change from its 0x1A.
        snap_v = H.reload_state(server, valid_login, valid_pass)
        if snap_v.get("character_id") is None:
            return (False,
                    "valid-case account's character vanished after its 0x1A: %r"
                    % snap_v)
        if valid_id is not None and snap_v.get("character_id") != valid_id:
            return (False,
                    "valid-case account's character_id changed: %r -> %r"
                    % (valid_id, snap_v.get("character_id")))
        if valid_pos[0] is not None:
            if (snap_v.get("x"), snap_v.get("y"), snap_v.get("mapIndex")) != valid_pos:
                return (False,
                        "valid-case account's persisted position mutated by its "
                        "0x1A: %r -> %r"
                        % (valid_pos,
                           (snap_v.get("x"), snap_v.get("y"), snap_v.get("mapIndex"))))

        # ===============================================================
        # 4) leave the server usable: prove a fresh legit Session still works.
        # ===============================================================
        post = H.Session(server, pseudo="CpPost")
        ok, why = _alive_and_clean(server)
        post.close()
        if not ok:
            return False, "server unusable after the test sequence: %s" % why

        return (True,
                "0x1A collectPlant: well-formed one-byte 0x1A from a fresh on-map "
                "player keeps the SERVER alive+clean (REACHABILITY-LIMITED — the "
                "success branch needs a matured plant on a dirt cell in front, "
                "unstageable in 'test', so the handler gracefully refuses & kicks "
                "the sender rather than ever succeeding/crashing); 4 "
                "malformed-framing variants each kept the server alive+clean; 3 "
                "illegal collects (no mature plant in front) each KICK the cheater "
                "while the server survives, and no state corruption (witness + "
                "both kicked accounts' persisted id/position intact). collectPlant "
                "emits no reply and the only mutation is on the unreachable success "
                "path, so identity/position is the DB-corruption witness "
                "(documented limit of reload_state)")
    except H.HandshakeError as e:
        return False, "handshake failed: %r" % e
    except Exception as e:
        import traceback
        return False, "unexpected exception: %r\n%s" % (e, traceback.format_exc()[-800:])
