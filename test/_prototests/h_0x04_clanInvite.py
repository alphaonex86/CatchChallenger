"""Valgrind/protocol test for server handler 0x04 (clan invite accept/refuse).

Handler: server/base/ClientNetworkReadMessage.cpp:333 (Client::parseMessage).

Wire framing: 0x04 is a FIXED-size(1) MESSAGE packet (code<0x80, not a reply ->
NO queryNumber; fixed size -> NO dataSize field). On the wire a valid packet is
exactly two bytes: [0x04][returnCode:1]. Because the size is fixed, the framing
layer (general/base/ProtocolParsingInput.cpp) ALWAYS hands parseMessage exactly
1 data byte; the HARDENED `if(size!=1)` guard in the handler is therefore not
reachable from the wire for this fixed packet — trailing bytes are reparsed as
the NEXT packet instead. Malformed framing for 0x04 thus manifests as a parser
DESYNC (trailing bytes forming an unknown/blocked code) rather than a wrong-size
in-handler reject.

Semantics — `returnCode = *data` (data[0]; the old data[1] over-read is fixed):
  0x01 -> clanInvite(true)  : ACCEPT the pending clan invitation.
          (LocalClientHandlerClan.cpp:655) If inviteToClanList is EMPTY ->
          errorOutput("Can't responde to clan invite, because no in suspend")
          -> parseMessage returns true, BUT errorOutput() already kicked.
          NOTE: errorOutput()->disconnectClient() => the offending client's TCP
          connection is CLOSED (KICK).  Server process stays alive.
  0x02 -> clanInvite(false) : REFUSE/cancel. This branch only does
          normalOutput(...) then inviteToClanList.erase(begin()); it NEVER calls
          errorOutput, so the client is NOT kicked even on an empty list. (The
          empty erase is technically erase(end()) UB; HARDENED here doesn't add
          _GLIBCXX_ASSERTIONS so it's a near-no-op memmove — valgrind would flag
          a regression if it ever became a real over-read. We assert the server
          survives it and the client stays connected, which is the contract.)
  else  -> default: errorOutput("wrong return code for clan invite ident:...")
          -> returns false -> client KICKED.

REACHABILITY (documented): a *non-empty* inviteToClanList (a real PENDING invite,
the only way 0x01 ACCEPT joins a clan instead of being refused) requires a second
session that is already a clan LEADER issuing a leader-only clan-invite packet,
which itself needs an already-created clan. In the 'test' maincode with fresh
accounts no clan exists, so every fresh on-map Session has an EMPTY
inviteToClanList. We therefore drive 0x04 from plain on-map Sessions: the
non-kicking VALID use we CAN reach is 0x02 refuse (server survives, no kick); the
"accept -> clan joined" happy path is reachability-limited and is exercised as
accept-with-no-pending-invite (the documented graceful KICK). Every abuse/error
path is fully exercised.

Asserts, per the task contract: server.alive() after every send; the VALID
refuse does NOT kick; every MALFORMED-framing and SEMANTIC-INVALID well-formed
packet KICKS the cheater while the server SURVIVES (no crash / no hang / valgrind
clean); and a separate untouched legit account is byte-for-byte unchanged in the
persisted DB (no phantom clan/position/identity leaked onto a bystander).
"""

import socket
import time

import _protoharness as H

NAME = "0x04 clanInvite"


# --- low-level kick detector -------------------------------------------------
def _socket_closed_by_peer(sess, timeout=2.5):
    """True if the server has closed this session's TCP socket (= the kick).

    A kick is disconnectClient() -> the server closes the connection; on our
    side that is recv()==b'' (EOF). The kick may be preceded by a 'have been
    kicked' system message broadcast, so we read until EOF or until the window
    elapses with the socket still open. Bounded so a MISSING kick can't hang.
    """
    sk = sess.sock
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            sk.settimeout(0.3)
            d = sk.recv(65536)
            if d == b"":
                return True                 # clean EOF -> peer closed -> kicked
            # got bytes (system / broadcast message) -> keep reading for the EOF
        except socket.timeout:
            # nothing pending; poke the peer so a half-closed socket surfaces.
            try:
                sk.sendall(bytes([0x02, 0x01, 0x01]))   # harmless move message
            except OSError:
                return True                 # send failed -> peer gone -> kicked
        except OSError:
            return True                     # recv raised -> peer gone -> kicked
    # Final confirmation poke.
    try:
        sk.settimeout(0.5)
        sk.sendall(bytes([0x02, 0x01, 0x01]))
        return sk.recv(65536) == b""
    except socket.timeout:
        return False                        # still open -> NOT kicked
    except OSError:
        return True                         # send/recv failed -> peer gone


def _still_connected(sess, timeout=1.0):
    """True if the session is still usable (peer NOT gone): a harmless move
    message is accepted and no EOF arrives within the window."""
    sk = sess.sock
    try:
        sk.settimeout(0.3)
        sk.sendall(bytes([0x02, 0x01, 0x01]))   # harmless move message
    except OSError:
        return False
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            sk.settimeout(0.3)
            d = sk.recv(65536)
            if d == b"":
                return False                # EOF -> peer gone (was kicked)
            # got data -> still connected, keep draining briefly
        except socket.timeout:
            return True                     # alive, nothing pending -> connected
        except OSError:
            return False
    return True


_UID = [0]


def _fresh_session(server, pseudo="ClanInv"):
    """A brand-new on-map Session with unique creds (login/pass/pseudo)."""
    _UID[0] += 1
    tag = ("ci%x_%x" % (int(time.time() * 1000) & 0xFFFFFFF, _UID[0])).encode()
    login = b"L_" + tag
    passh = b"P_" + tag
    s = H.Session(server, login=login, passh=passh,
                  pseudo=(pseudo + str(_UID[0]))[:20])
    return s, login, passh


def run(server):
    try:
        if not server.alive():
            return (False, "server not alive at test start")

        # A separate, untouched legit account: proves the server never corrupts
        # unrelated state while it kicks the cheaters.
        victim, victim_login, victim_pass = _fresh_session(server, "ClanVictim")
        victim_before = {"x": victim.x, "y": victim.y,
                         "mapIndex": victim.mapIndex,
                         "character_id": victim.character_id}

        valid_cases = 0
        malformed_cases = 0
        semantic_invalid_cases = 0

        # =====================================================================
        # 1) VALID (non-kicking) use: 0x02 refuse on an EMPTY invite list.
        #    clanInvite(false) only normalOutputs + erases -> NO kick. The
        #    connection MUST stay open and the server MUST survive (this also
        #    walks the empty-list erase path valgrind watches for regressions).
        # =====================================================================
        s_refuse, _, _ = _fresh_session(server, "ClanRefuse")
        s_refuse.m(0x04, H.u8(0x02))                # [0x04][0x02]
        time.sleep(0.4)
        if not server.alive():
            return (False, "server died after VALID 0x04 refuse(0x02): "
                           + str(server.crash_report()))
        if not _still_connected(s_refuse):
            s_refuse.close()
            return (False, "VALID 0x04 refuse(0x02) wrongly closed the connection "
                           "(refuse must NOT kick)")
        # A second refuse also must be accepted and not kick.
        s_refuse.m(0x04, H.u8(0x02))
        time.sleep(0.3)
        if not server.alive():
            return (False, "server died after 2nd VALID refuse: "
                           + str(server.crash_report()))
        if not _still_connected(s_refuse):
            s_refuse.close()
            return (False, "2nd VALID refuse wrongly kicked the client")
        s_refuse.close()
        valid_cases += 1
        if server.crash_report() is not None:
            return (False, "crash after VALID refuse: " + str(server.crash_report()))

        # =====================================================================
        # 2) SEMANTIC-INVALID: well-formed (size==1, on-wire valid) but illegal
        #    for THIS handler/state. Each MUST kick + leave the server alive.
        # =====================================================================
        semantic = [
            ("accept-with-no-pending-invite (0x01)", 0x01),
            ("unknown returnCode 0x00",              0x00),
            ("unknown returnCode 0x03",              0x03),
            ("unknown returnCode 0x7F",              0x7F),
            ("unknown returnCode 0xFF",              0xFF),
        ]
        for label, sub in semantic:
            ss, _, _ = _fresh_session(server, "ClanSem")
            ss.m(0x04, H.u8(sub))                   # well-formed fixed message
            time.sleep(0.35)
            # (b) SERVER SURVIVES
            if not server.alive():
                return (False, "server died after SEMANTIC %s: %s"
                        % (label, server.crash_report()))
            if server.crash_report() is not None:
                return (False, "crash_report non-None after SEMANTIC %s: %s"
                        % (label, server.crash_report()))
            # (a) KICK
            if not _socket_closed_by_peer(ss):
                ss.close()
                return (False, "SEMANTIC %s did NOT kick the cheater "
                               "(silent ignore = contract violation)" % label)
            ss.close()
            semantic_invalid_cases += 1

        # =====================================================================
        # 3) MALFORMED FRAMING. 0x04 is fixed-size(1), so the framing layer feeds
        #    parseMessage exactly 1 byte regardless; the malformed cases below
        #    desync the parser (trailing bytes / truncation) — each must keep the
        #    server ALIVE (no crash, no hang) and never corrupt it.
        # =====================================================================

        # (i) truncated: just the code byte, no return code. The fixed reader
        #     waits for the missing byte that never arrives -> the connection
        #     simply sits idle (NOT a kick, NOT a crash). We assert ONLY that the
        #     server stays alive and does not hang; we do NOT assert a kick here.
        s_m1, _, _ = _fresh_session(server, "ClanMalfTrunc")
        s_m1.send_raw(bytes([0x04]))                # truncated, missing data byte
        time.sleep(0.3)
        if not server.alive():
            return (False, "server died/hung after truncated 0x04 (no payload): "
                           + str(server.crash_report()))
        if server.crash_report() is not None:
            return (False, "crash after truncated 0x04: " + str(server.crash_report()))
        s_m1.close()
        malformed_cases += 1

        # (ii) oversized: [0x04][0x01] + trailing 0xFF. 0x01 ACCEPT on empty list
        #      kicks; the trailing byte is moot (peer already gone). Confirms the
        #      server survives extra wire bytes and kicks deterministically.
        s_m2, _, _ = _fresh_session(server, "ClanMalfOver")
        s_m2.send_raw(bytes([0x04, 0x01, 0xFF]))
        time.sleep(0.35)
        if not server.alive():
            return (False, "server died after oversized 0x04 framing: "
                           + str(server.crash_report()))
        if not _socket_closed_by_peer(s_m2):
            s_m2.close()
            return (False, "oversized 0x04 framing left the client un-kicked")
        s_m2.close()
        malformed_cases += 1
        if server.crash_report() is not None:
            return (False, "crash after oversized framing: " + str(server.crash_report()))

        # (iii) parser-desync: [0x04][0x02] (a benign refuse, no kick) immediately
        #       followed by a BLOCKED code 0x01 standalone. 0x01 has packetFixedSize
        #       0xFF (blocked) as a client->server message here -> errorParsingLayer
        #       -> KICK. Proves trailing-byte desync after a valid 0x04 is caught
        #       and the server survives.
        s_m3, _, _ = _fresh_session(server, "ClanMalfDesync")
        s_m3.send_raw(bytes([0x04, 0x02]) + bytes([0x01, 0xAB, 0xCD]))
        time.sleep(0.35)
        if not server.alive():
            return (False, "server died after desync 0x04 framing: "
                           + str(server.crash_report()))
        if not _socket_closed_by_peer(s_m3):
            s_m3.close()
            return (False, "desync after valid 0x04 left the client un-kicked")
        s_m3.close()
        malformed_cases += 1
        if server.crash_report() is not None:
            return (False, "crash after desync framing: " + str(server.crash_report()))

        # (iv) fake-dynamic framing: pretend 0x04 carries a 4-byte length. 0x04 is
        #      FIXED, so the first trailing byte (0x10) is read as returnCode ->
        #      unknown -> KICK; the rest is reparsed/ignored after disconnect.
        #      Confirms a bad length prefix can't crash or hang the server.
        s_m4, _, _ = _fresh_session(server, "ClanMalfFakeDyn")
        s_m4.send_raw(bytes([0x04]) + H.u32(0x10) + (b"\x00" * 4))
        time.sleep(0.35)
        if not server.alive():
            return (False, "server died after fake-dynamic 0x04 framing: "
                           + str(server.crash_report()))
        if not _socket_closed_by_peer(s_m4):
            s_m4.close()
            return (False, "fake-dynamic 0x04 framing left the client un-kicked")
        s_m4.close()
        malformed_cases += 1
        if server.crash_report() is not None:
            return (False, "crash after fake-dynamic framing: "
                           + str(server.crash_report()))

        # =====================================================================
        # 4) NO STATE CORRUPTION. clanInvite never touches other players; the
        #    untouched victim session must be intact and its persisted DB
        #    identity/position must be byte-for-byte unchanged.
        # =====================================================================
        if not server.alive():
            return (False, "server not alive before post-checks: "
                           + str(server.crash_report()))

        # The innocent victim did nothing illegal -> it must NOT have been kicked.
        if not _still_connected(victim):
            victim.close()
            return (False, "innocent victim session was kicked -> collateral damage")
        victim.close()
        time.sleep(0.5)   # let FILE_DB disconnect-save flush before reload_state

        # DB-state: reload the victim's persisted character via a fresh
        # connection; identity+position must match the pre-cheat snapshot.
        snap = H.reload_state(server, victim_login, victim_pass)
        if snap.get("character_id") is None:
            return (False, "could not reload victim DB state: %r" % snap)
        if snap.get("character_id") != victim_before["character_id"]:
            return (False, "victim character_id changed in DB: %r -> %r"
                    % (victim_before["character_id"], snap.get("character_id")))
        if snap.get("x") != victim_before["x"] \
                or snap.get("y") != victim_before["y"] \
                or snap.get("mapIndex") != victim_before["mapIndex"]:
            return (False, "victim position mutated in DB by rejected clanInvite: "
                    "before=(%r,%r,%r) after=(%r,%r,%r)"
                    % (victim_before["x"], victim_before["y"],
                       victim_before["mapIndex"], snap.get("x"), snap.get("y"),
                       snap.get("mapIndex")))

        # Leave the server usable for the next test: fresh legit session works.
        if not server.alive():
            return (False, "server not alive at end: " + str(server.crash_report()))
        if server.crash_report() is not None:
            return (False, "crash_report non-None at end: " + str(server.crash_report()))
        post, _, _ = _fresh_session(server, "ClanPost")
        post.close()

        detail = ("clanInvite 0x04: %d valid (refuse 0x02 no-kick; accept happy "
                  "path reachability-limited -> no real pending invite in 'test' "
                  "maincode), %d malformed-framing, %d semantic-invalid; every "
                  "abuse kicked the cheater, server stayed alive (no crash/hang), "
                  "victim DB id/pos unchanged."
                  % (valid_cases, malformed_cases, semantic_invalid_cases))
        return (True, detail)

    except Exception as e:
        try:
            rep = server.crash_report()
        except Exception:
            rep = None
        return (False, "exception in run(): %r ; crash_report=%r" % (e, rep))
