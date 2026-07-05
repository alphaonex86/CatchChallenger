"""Valgrind/protocol test for server handler 0x18 (takeAnObjectOnMap).

Handler: server/base/ClientNetworkReadMessage.cpp:588 (Client::parseMessage),
case 0x18 -> Client::takeAnObjectOnMap()
(server/base/crafting/LocalClientHandlerCrafting.cpp:97).

Wire form: FIXED-size MESSAGE, code 0x18, size 0
(packetFixedSize[0x18]=0, general/base/ProtocolParsingGeneral.cpp:89).
On the wire it is exactly ONE byte: [0x18]. No queryNumber (message,
code<0x80), no 4-byte length (fixed size, not 0xFE). The client sends it as
packOutcommingData(0x18,NULL,0) (Api_protocol.cpp:979). The handler reads NO
payload from the wire at all.

Handler logic traced (LocalClientHandlerCrafting.cpp:97-135):
  void Client::takeAnObjectOnMap():
    new_map = mapAndPosIfMoveInLookingDirectionJumpColision(map_index,x,y)
              (LocalClientHandler.cpp:380) -- resolves the tile the player is
              FACING (default look direction is Direction_look_at_bottom,
              MoveOnTheMap.cpp:11).
    if(new_map==nullptr)                 -> errorOutput("Can't move ...")      -> KICK
    if(no item at that (x,y))            -> errorOutput("Not on map item ...") -> KICK
    if(player already has this map item) -> errorOutput("Have already ...")    -> KICK
    item = new_map->items.at(x,y)
    if(!item.infinite){ mapData.items.insert(x,y); syncDatabaseItemOnMap(); }
    addObject(item.item)                 -> gives the item, persists

  errorOutput() (Client.cpp:524) for a CharacterSelected client ALWAYS kicks
  (disconnectClient()): there is NO error-reply-without-kick path here.

REACHABILITY (documented partial -> reachability_limited=True):
  The VALID, state-mutating path (actually picking up an item and gaining it in
  the inventory) requires the player to be FACING a cell that the datapack marks
  as an item-on-map (new_map->items has (x,y)). In the 'test' maincode a freshly
  handshaked player is not standing in front of such an item-on-map cell, and the
  harness exposes no way to teleport/orient onto one. So the gain-the-item branch
  is NOT exercised; it is flagged reachability-limited.

  What IS fully exercised -- and is the security-critical contract for 0x18:
  * VALID-SHAPE / SEMANTIC-INVALID (the same single packet here, because the
    handler takes no arguments): a well-formed [0x18] from an on-map player who
    is NOT facing an item-on-map cell. The server MUST refuse via errorOutput()
    -> KICK (the documented "Not on a map item -> no crash" case), MUST NOT
    crash/hang/leak, and MUST NOT mutate any state (no phantom item/cash added,
    no map-item flagged taken).
  * MALFORMED FRAMING: [0x18] carrying spurious trailing bytes / a fake dynamic
    length / a burst. The fixed-size-0 framer must not wedge; server stays alive.
  * NO STATE CORRUPTION: a SEPARATE untouched legitimate account's persisted
    identity/position is byte-for-byte unchanged by any rejected 0x18.

Verification strategy:
  0x18 is a MESSAGE -> there is no 0x7F reply to the sender for the refusal path
  (errorOutput sends a system-message broadcast, then disconnects). The
  observable contract is the KICK (socket EOF) + server alive + clean, plus NO
  state corruption verified through a SEPARATE legitimate session and
  H.reload_state.
"""

import time
import socket
import _protoharness as H

NAME = "0x18 takeAnObjectOnMap"


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


def _is_kicked(sess, timeout=1.5):
    """Return True iff the offending client's connection was closed by the
    server (a KICK). Detection: read the raw socket for EOF (recv -> b''),
    then confirm with a follow-up byte that the peer is really gone (broken
    pipe / further EOF). A still-open socket that keeps accepting our bytes
    and never EOFs == NOT kicked (contract violation for this handler)."""
    sk = sess.sock
    deadline = time.time() + timeout
    saw_eof = False
    sk.settimeout(0.3)
    while time.time() < deadline:
        try:
            d = sk.recv(4096)
            if d == b"":          # orderly shutdown from the server == kick
                saw_eof = True
                break
            # any bytes here are the kick system-message / drain; keep reading
        except socket.timeout:
            # nothing pending yet; poke the peer to force a verdict
            try:
                sk.sendall(b"\x18")   # a 2nd take byte onto a (maybe) dead peer
            except OSError:
                saw_eof = True        # broken pipe == peer gone == kicked
                break
        except OSError:
            saw_eof = True            # connection reset == kicked
            break
    if not saw_eof:
        # Final confirmation: one more send + read; a live peer stays
        # readable/writable, a kicked one yields EOF/error.
        try:
            sk.sendall(b"\x18")
            sk.settimeout(0.5)
            try:
                if sk.recv(4096) == b"":
                    saw_eof = True
            except socket.timeout:
                saw_eof = False
            except OSError:
                saw_eof = True
        except OSError:
            saw_eof = True
    return saw_eof


def run(server):
    try:
        # ----------------------------------------------------------------
        # A SEPARATE, untouched legitimate session: its persisted state is the
        # baseline we assert is NOT corrupted by any rejected 0x18 from cheaters.
        # ----------------------------------------------------------------
        vlogin = _uniq("vic")
        vpassw = _uniq("vpw")
        try:
            victim = H.Session(server, login=vlogin, passh=vpassw, pseudo="Victim")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)
        victim_start = (victim.x, victim.y, victim.mapIndex)
        # Persist the victim's clean baseline (disconnect-save) and read it back.
        victim.close()
        time.sleep(0.35)
        try:
            base_snap = H.reload_state(server, vlogin, vpassw)
        except Exception as e:
            return (False, "baseline reload_state raised: %s" % e)
        if base_snap.get("character_id") is None:
            return (False, "victim did not persist (no character id)")

        valid_cases = 0
        semantic_invalid_cases = 0
        malformed_cases = 0
        verifies_kick = True
        reachability_limited = True  # the gain-the-item branch is not driveable here

        # ----------------------------------------------------------------
        # VALID-SHAPE + SEMANTIC-INVALID (the key contract): a well-formed [0x18]
        # from an on-map session that is NOT facing an item-on-map cell. The
        # handler has no arguments, so the well-formed packet IS the semantically
        # illegal one in the 'test' maincode (player not on/at an item tile).
        # takeAnObjectOnMap() -> errorOutput("Not on map item ..." / "Can't move
        # ...") -> KICK. This is the documented "Not on an item tile -> no crash"
        # case and is the only form reachable from a single session, so it doubles
        # as the reachability-limited valid-shape "refuse gracefully" check.
        # ----------------------------------------------------------------
        try:
            cheat = H.Session(server, login=_uniq("c1"), passh=_uniq("p1"),
                              pseudo="Cheat1")
        except H.HandshakeError as e:
            return (False, "cheat1 handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after cheat1 handshake: %s" % why)
        cheat.m(0x18)  # well-formed: fixed size 0, just the opcode byte
        valid_cases += 1                 # well-formed packet shape exercised
        semantic_invalid_cases += 1      # semantically illegal (not on an item)
        kicked = _is_kicked(cheat)
        cheat.close()
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x18 not on an item tile: %s" % why)
        if not kicked:
            verifies_kick = False
            return (False, "0x18 not on an item tile did NOT kick the client "
                           "(server silently ignored a contract-violating "
                           "takeAnObjectOnMap -> CONTRACT VIOLATION)")

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID #2: DOUBLE-TAKE. Send two [0x18] back-to-back from a
        # fresh session. The first kicks; the second is delivered against (or
        # after) a closed connection. Per the handler notes this must NOT crash.
        # ----------------------------------------------------------------
        try:
            cheat2 = H.Session(server, login=_uniq("c2"), passh=_uniq("p2"),
                               pseudo="Cheat2")
        except H.HandshakeError as e:
            return (False, "cheat2 handshake failed: %s" % e)
        try:
            cheat2.send_raw(b"\x18\x18")  # two takes in one write
        except OSError:
            pass  # kicked mid-write is acceptable
        semantic_invalid_cases += 1
        kicked2 = _is_kicked(cheat2)
        cheat2.close()
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after double 0x18: %s" % why)
        if not kicked2:
            verifies_kick = False
            return (False, "double 0x18 did NOT kick the client")

        # ----------------------------------------------------------------
        # MALFORMED FRAMING cases. 0x18 is fixed-size-0; spurious trailing bytes,
        # a fake dynamic length, or a burst must not wedge the framer. Each uses a
        # disposable session; after each we assert server alive + clean.
        # ----------------------------------------------------------------
        def fresh():
            return H.Session(server, login=_uniq("bad"), passh=_uniq("bp"),
                             pseudo="Bad")

        # (M1) 0x18 followed immediately by a stray data byte. The fixed-0 framer
        # consumes only the opcode; the 0xFF that follows is an unknown/blocked
        # code -> the offending client is kicked, server survives.
        try:
            s = fresh()
            s.send_raw(bytes([0x18, 0xFF]))
            time.sleep(0.1)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x18+stray byte: %s" % why)

        # (M2) 0x18 with a bogus 4-byte length prefix as if it were a dynamic
        # packet (it is NOT dynamic). The server frames it as fixed-0, so the
        # length bytes are interpreted as following packets -> garbage codes ->
        # kick. Must not crash/hang.
        try:
            s = fresh()
            s.send_raw(bytes([0x18]) + H.u32(0xFFFFFFFF))
            time.sleep(0.1)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x18+bogus length: %s" % why)

        # (M3) A burst of 0x18 opcodes in one write (rapid repeated take). The
        # first kicks; the rest land on a torn-down connection. No crash/hang.
        try:
            s = fresh()
            s.send_raw(bytes([0x18]) * 8)
            time.sleep(0.1)
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x18 burst: %s" % why)

        # (M4) 0x18 then a truncated dynamic header (a half-written 4-byte len of
        # an unrelated dynamic code) to probe partial-frame handling. Server must
        # not wedge waiting forever; the offending client is kicked, server lives.
        try:
            s = fresh()
            s.send_raw(bytes([0x18, 0x03, 0x02, 0x00]))  # 0x18 + start of dyn 0x03 chat, truncated len
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF]))              # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x18+truncated dynamic: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: re-read the untouched victim's persisted state via
        # a SEPARATE legitimate path. None of the rejected 0x18 attempts (from
        # other accounts) may have changed it. Identity + position must be
        # byte-for-byte the baseline. (cash/items/monsters are not in the reload
        # header per the foundation's documented limit; the gain-item branch was
        # never reached so no item/cash could be created for any account anyway.)
        # ----------------------------------------------------------------
        try:
            after_snap = H.reload_state(server, vlogin, vpassw)
        except Exception as e:
            return (False, "post-abuse reload_state raised: %s" % e)
        checks_db = (after_snap.get("character_id") is not None)
        verifies_no_state_corruption = True
        for k in ("character_id", "pseudo", "x", "y", "mapIndex"):
            if after_snap.get(k) != base_snap.get(k):
                verifies_no_state_corruption = False
                return (False, "victim state changed by a rejected 0x18: %s "
                               "baseline=%r after=%r"
                               % (k, base_snap.get(k), after_snap.get(k)))

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after corruption re-check: %s" % why)

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session still reaches
        # CharacterSelected after all the abuse.
        # ----------------------------------------------------------------
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.drain(timeout=0.2)
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid(shape)=%d semantic_invalid=%d malformed=%d kick_verified=%s "
            "no_corruption=%s db_checked=%s reachability_limited=%s "
            "victim_start=%s victim_persisted=(%s,%s,%s); 0x18 from a player not "
            "facing an item-on-map cell kicked the cheater (the documented "
            "'Not on an item tile -> no crash' path), double/burst/malformed-framed "
            "0x18 left the server alive+clean, victim identity/position unchanged. "
            "NOTE: the gain-the-item branch (player FACING a datapack item-on-map "
            "cell -> addObject + persist) is not driveable from the 'test' maincode "
            "(no way to orient onto an item tile here) -> reachability-limited."
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               verifies_kick, verifies_no_state_corruption, checks_db,
               reachability_limited, victim_start,
               after_snap.get("x"), after_snap.get("y"), after_snap.get("mapIndex"))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
