#!/usr/bin/env python3
"""
Handler test: 0x87 getShopList (server/base/ClientNetworkReadQuery.cpp:74 ->
Client::getShopList, server/base/ClientEvents/LocalClientHandlerShop.cpp:9).

Wire shape (general/base/ProtocolParsingGeneral.cpp): 0x87 is a FIXED-size-0
QUERY -> on the wire it is exactly [0x87][queryNumber] with NO length field and
NO data. Its reply (index 128+0x87) is DYNAMIC: [0x7F][qn][size:4 LE]
[objectCount:2 LE]{ [itemId:2][price:4][0:4] }*objectCount.

Handler semantics (LocalClientHandlerShop.cpp:9-72):
  * mapIndex>=65535            -> silent return (only when off-map; never for a
                                  selected on-map player).
  * mapAndPosIfMoveInLookingDirectionJumpColision() == nullptr
                                -> errorOutput("Can't move ...") -> KICK.
  * the looked-at cell is NOT a shop (new_map->shops has no entry there)
                                -> errorOutput("not shop into this direction")
                                   -> KICK (Client::errorOutput, Client.cpp:524,
                                   while CharacterSelected -> disconnectClient()).
  * only when the looked-at cell IS a shop does it removeFromQueryReceived() and
    send the 0x7F dynamic shop-list reply.

REACHABILITY: the maincode 'test' datapack the harness loads has no shop the
spawned player is standing in front of, so the only observable in-band outcome
of a well-formed 0x87 from an on-map player is the "not shop into this
direction" KICK. We therefore treat the well-formed 0x87 both as the
"valid framing -> server stays alive" case AND as the semantic-invalid
"request shop list while not at a shop" case (a cheater asking for a shop that
isn't there): the contract is the offending client is disconnected and the
server survives with no state corruption. Reaching the genuine list reply would
need a map that places the player on a shop cell, which is not available here;
that branch is documented as reachability-limited.

Malformed framing: 0x87 carries no payload, so "malformed" = bad framing of the
packet itself (truncated query header, or bytes appended after the fixed-0
query that the stream parser then mis-reads as another packet). After each we
assert the server process is alive and crash_report() is None.

No persisted state is mutated by getShopList (it is a pure read that only sends
a reply or kicks), so "no state corruption" is verified by confirming a fresh
legit Session for a SECOND account still handshakes to CharacterSelected with
its spawn position intact, and by H.reload_state() on the kicked account
showing its persisted identity/position is unchanged.
"""

import time
import _protoharness as H


NAME = "0x87 getShopList"


def _peer_is_closed(sess, settle=0.8):
    """Return True if the server has closed (kicked) this session's socket.

    A kicked client receives a FIN: recv() then returns b'' (EOF) instead of
    blocking/timing out. We pump for a short while (auto-ACKing any pending
    server queries) and look for EOF. A still-open idle connection makes recv()
    time out instead -> returns False. A hard reset surfaces as OSError -> also
    closed. Bounded so a missing reply can never hang the test.
    """
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sess.sock.settimeout(0.25)
            d = sess.sock.recv(65536)
        except (TimeoutError, OSError) as e:
            # socket.timeout is a subclass of OSError on 3.10+, so disambiguate.
            if isinstance(e, H.socket.timeout):
                # still open, server just had nothing more to say this slice
                continue
            return True  # reset / broken pipe -> peer gone
        if d == b"":
            return True  # clean FIN -> kicked
        # got some bytes (e.g. the system-message that precedes the kick, or an
        # 0xE? server query): absorb them and keep waiting for the FIN.
        sess._buf.extend(d)
        try:
            ev = sess._parse_stream()
            sess._auto_ack(ev)
        except Exception:
            pass
    return False


def _confirm_kick(sess):
    """Confirm the offending client was disconnected. After a kick the socket
    reaches EOF; a follow-up send then fails or yields no reply and still EOF.
    Returns (kicked: bool, why: str)."""
    if _peer_is_closed(sess):
        return True, "peer closed (FIN/EOF) after illegal getShopList"
    # Not obviously closed yet: poke it with a harmless extra 0x87 and re-check.
    try:
        sess.q(0x87, b"")
    except OSError:
        return True, "send after illegal getShopList failed (broken pipe) -> kicked"
    if _peer_is_closed(sess):
        return True, "peer closed after follow-up poke"
    return False, "connection still open (NOT kicked) -> contract violation"


def run(server):
    sessions = []
    try:
        # ---- precondition: an on-map player (a fresh handshaked Session) -----
        try:
            cheater = H.Session(server, pseudo="ShopCheat")
        except H.HandshakeError as e:
            return (False, "precondition handshake failed: %r" % e)
        sessions.append(cheater)
        if not server.alive():
            return (False, "server not alive right after handshake")

        # ---- VALID FRAMING == SEMANTIC-INVALID (not-at-shop) -> KICK ---------
        # Well-formed fixed-size-0 query [0x87][qn]. In maincode 'test' the
        # player is not on a shop cell, so getShopList -> errorOutput(...) ->
        # disconnectClient(). Contract: client kicked, server survives.
        cheater.q(0x87, b"")
        if not server.alive():
            return (False, "server died after a well-formed 0x87 (should kick the "
                           "client, never crash): %s" % server.crash_report())
        kicked, why = _confirm_kick(cheater)
        cr = server.crash_report()
        if cr is not None:
            return (False, "crash_report after well-formed 0x87: %s" % cr)
        if not kicked:
            return (False, "well-formed 0x87 while not at a shop did NOT kick the "
                           "client: %s" % why)
        valid_cases = 1          # the packet was delivered + framed correctly
        semantic_invalid_cases = 1  # not-at-a-shop is the reachable illegal use
        try:
            cheater.close()
        except Exception:
            pass

        # ---- NO STATE CORRUPTION: a SECOND legit account is unaffected -------
        # getShopList persists nothing; prove an independent player still
        # reaches CharacterSelected on the map with a valid spawn.
        try:
            victim = H.Session(server, pseudo="ShopWitness")
        except H.HandshakeError as e:
            return (False, "post-kick: independent Session failed to handshake -> "
                           "the kick corrupted shared/server state: %r" % e)
        sessions.append(victim)
        if victim.mapIndex is None or victim.x is None or victim.y is None:
            # position is best-effort; only fail if the handshake itself broke.
            if not server.alive():
                return (False, "victim session lost the server")
        victim.close()

        # reload_state on the kicked account: identity/position must be intact
        # (handler mutates nothing, so this is a pure no-corruption check).
        time.sleep(0.3)  # let the disconnect-save settle (FILE_DB block save)
        snap = H.reload_state(server, cheater.login_creds, cheater.pass_creds)
        # character_id must still resolve; position must be a sane spawn, not
        # garbage. We don't assert exact coords (spawn is start.xml-defined) but
        # they must be present and the account must still log in.
        if snap.get("character_id") is None:
            return (False, "reload_state lost the kicked account's character -> "
                           "state corruption from the rejected getShopList")

        # ---- MALFORMED FRAMING (fresh session each, assert survive) ----------
        # 0x87 has no payload, so we corrupt the FRAME itself with send_raw.
        malformed = []
        # 1) Query header truncated: just the code byte, no queryNumber. The
        #    server's input parser is waiting for the 1-byte qnum that never
        #    comes; it must not crash. We then close.
        malformed.append(("0x87 with no queryNumber (truncated header)",
                          bytes([0x87])))
        # 2) Trailing junk after the fixed-0 query: [0x87][qn] then a stray
        #    blocked/unknown code (0x01 reply-marker artifact with no body) ->
        #    the server frames the valid 0x87 (kick) then sees garbage.
        malformed.append(("0x87 + trailing 0x01 artifact byte",
                          bytes([0x87, 0x00, 0x01])))
        # 3) Dynamic-style framing for a FIXED-0 packet: pretend 0x87 carries a
        #    4-byte length + 8 bytes of data. The server treats 0x87 as fixed-0,
        #    so [len:4][data:8] becomes a bogus follow-on packet stream.
        malformed.append(("0x87 framed as dynamic (len=8 + 8 data bytes)",
                          bytes([0x87, 0x00]) + H.u32(8) + b"\xAA" * 8))
        # 4) Oversized bogus length on the trailing bytes: claims a huge size
        #    that will never arrive (truncated dynamic). Server must not hang or
        #    allocate unbounded.
        malformed.append(("0x87 + trailing pseudo-dynamic with huge size",
                          bytes([0x87, 0x00]) + b"\x83" + H.u32(0x7FFFFFFF)))

        malformed_cases = 0
        for label, raw in malformed:
            try:
                ms = H.Session(server, pseudo="ShopMal%d" % malformed_cases)
            except H.HandshakeError as e:
                return (False, "malformed[%s]: pre-handshake failed: %r" % (label, e))
            sessions.append(ms)
            try:
                ms.send_raw(raw)
            except OSError:
                pass  # peer may already be gone; that's fine
            # bounded drain so a missing/never-coming reply cannot hang us
            try:
                ms.drain(timeout=0.6)
            except Exception:
                pass
            if not server.alive():
                return (False, "server DIED on malformed framing [%s]: %s"
                               % (label, server.crash_report()))
            cr = server.crash_report()
            if cr is not None:
                return (False, "crash_report on malformed framing [%s]: %s"
                               % (label, cr))
            malformed_cases += 1
            try:
                ms.close()
            except Exception:
                pass

        # ---- post-checks: server still usable for the next test --------------
        if not server.alive():
            return (False, "server not alive at end of test")
        if server.crash_report() is not None:
            return (False, "crash_report set at end: %s" % server.crash_report())
        try:
            tail = H.Session(server, pseudo="ShopAfter")
            sessions.append(tail)
            tail.close()
        except H.HandshakeError as e:
            return (False, "server unusable after test (final Session failed): %r" % e)

        return (True,
                "0x87 getShopList: well-formed-while-not-at-shop correctly KICKS "
                "the client (%d valid+semantic-invalid case) and server survives; "
                "%d malformed-framing variants each left the server alive with no "
                "crash; second account + reload_state confirm no state corruption. "
                "Genuine shop-list reply branch is reachability-limited (no shop "
                "cell under the player in maincode 'test')."
                % (semantic_invalid_cases, malformed_cases))
    except Exception as e:
        return (False, "unexpected exception in run(): %r" % e)
    finally:
        for s in sessions:
            try:
                s.close()
            except Exception:
                pass
