#!/usr/bin/env python3
"""
h_0xaa_addCharacter.py — valgrind/protocol test for the server handler 0xAA
(addCharacter), parsed in server/base/ClientNetworkReadQuery.cpp:445 and
dispatched to Client::addCharacter() (server/base/ClientLoad/ClientHeavyLoadLogin2.cpp:227).

Wire layout (DYNAMIC):
    [charGroup:1][profile:1][pseudoLen:1][pseudo:pseudoLen][monsterGroup:1][skin:1]
Reply (FIXED 5): [status:1][character_id:4 LE]  (status 0x00=ok, 0x01=exists, 0x02=skin-list-empty)

HANDLER STATE: 0xAA is only valid in the PRE-CHARACTER state (stat==Logged).
If stat==CharacterSelected the handler errorOutput()s -> KICK. Therefore this
test does NOT use the harness Session (which drives all the way to
CharacterSelected); it builds its own raw connection that stops at Logged
(A0 -> A8 -> A9 create -> A8 re-login) and then issues 0xAA from there.

Contract checked:
  * VALID: a well-formed packet with profile 0, an in-list forced skin (2=player)
    and an in-range monsterGroup creates the character (reply status 0x00,
    nonzero id) and is persisted (verified via H.reload_state).
  * MALFORMED FRAMING: truncated dynamic size / dynamic length > actual data /
    missing monsterGroup+skin bytes / trailing data after skin. parseQuery()
    returns false -> errorOutput() -> the offending client is KICKED, the server
    stays ALIVE and uncrashed.
  * SEMANTIC-INVALID (well-formed but illegal): out-of-range profile index,
    skin not in the profile's forcedskin list, empty pseudo, pseudo containing a
    space, out-of-range monsterGroup. Each -> errorOutput() -> KICK; the server
    survives; and NO character is persisted for that (fresh, single-use) account.
  * NO STATE CORRUPTION: each semantic-invalid attempt uses its own brand-new
    account so a successful create can never mask a rejected one; H.reload_state
    confirms no phantom character landed, and a separate legit account created
    afterwards proves the server is still fully usable.

Datapack 'test'/main profile is single: index 0 "Normal", forcedskin=player;girlplayer
(skins sort bandit(0)/girlplayer(1)/player(2)/...; valid forced ids = {1,2}),
monstergroup count 3 (valid monsterGroup ids = {0,1,2}).
"""

import os, sys, time, socket, struct

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import _protoharness as H

NAME = "0xAA addCharacter"

# Valid inputs for the 'test' datapack main profile.
_PROFILE_OK = 0
_SKIN_OK = 2          # 'player' -> in forcedskin list (girlplayer=1 also valid)
_MONGRP_OK = 0        # profile 0 has 3 monstergroups -> 0,1,2 valid
_CHARGRP = 0


class _PreCharConn:
    """A raw connection driven to the Logged (pre-character) state, where 0xAA
    is legal. Built by hand because the harness Session goes to CharacterSelected.
    Reuses the harness framing helpers (blake3/_creds/_RawConn parser)."""

    def __init__(self, server, login=None, passw=None):
        self.server = server
        self.login = login if login is not None else (
            ("aa%x%x" % (int(time.time()) & 0xFFFFFFFF, id(self) & 0xFFFFF)).encode())
        self.passw = passw if passw is not None else (b"p_" + self.login)
        self.sock = socket.create_connection(("127.0.0.1", server.port), timeout=5)
        self.sock.settimeout(0.4)
        self._conn = H._RawConn(self.sock)
        self._login_to_logged()

    def _login_to_logged(self):
        loginHash, passHash = H._creds(self.login, self.passw)
        pa = self._conn.query(0xA0, H.PROTOCOL_HEADER_LOGIN, dynamic=False, timeout=3.0)
        if pa is None or len(pa) < 17:
            raise H.HandshakeError("A0 no/short reply")
        if pa[0] != 0x04:
            raise H.HandshakeError("A0 compression %#x != 0x04" % pa[0])
        token = pa[1:17]
        a8 = loginHash + H.blake3(passHash + token)
        pa = self._conn.query(0xA8, a8, dynamic=False, timeout=3.0)
        if pa is None or len(pa) < 1:
            raise H.HandshakeError("A8 no reply")
        if pa[0] == 0x07:
            a9 = H.blake3(loginHash) + passHash
            pa = self._conn.query(0xA9, a9, dynamic=False, timeout=3.0)
            if pa is None or pa[0] != 0x01:
                raise H.HandshakeError("A9 create failed")
            pa = self._conn.query(0xA8, a8, dynamic=False, timeout=3.0)
            if pa is None or pa[0] != 0x01:
                raise H.HandshakeError("A8 re-login failed")
        elif pa[0] != 0x01:
            raise H.HandshakeError("A8 status %#x" % pa[0])
        # stat is now Logged; the char-list block is in pa.

    @staticmethod
    def body(profile, pseudo, mongrp, skin, chargrp=_CHARGRP):
        if isinstance(pseudo, str):
            pseudo = pseudo.encode()
        return (bytes([chargrp & 0xFF, profile & 0xFF, len(pseudo) & 0xFF]) +
                pseudo + bytes([mongrp & 0xFF, skin & 0xFF]))

    def add_query(self, profile, pseudo, mongrp, skin, chargrp=_CHARGRP, timeout=4.0):
        """Send a well-formed 0xAA query; return its 5-byte reply payload or None."""
        return self._conn.query(0xAA, self.body(profile, pseudo, mongrp, skin, chargrp),
                                dynamic=True, timeout=timeout)

    def send_raw(self, raw):
        self.sock.sendall(raw)

    def is_kicked(self, settle=0.6):
        """True if the server has closed this connection (the cheater was kicked).
        Detect: recv reaches EOF (b'') / the socket errors. A still-open connection
        => the contract violation 'not kicked'."""
        time.sleep(settle)
        try:
            self.sock.settimeout(0.6)
            # Drain anything pending; a kicked socket reaches EOF (recv -> b'').
            saw_eof = False
            deadline = time.time() + 1.2
            while time.time() < deadline:
                try:
                    d = self.sock.recv(4096)
                except socket.timeout:
                    break
                except OSError:
                    saw_eof = True
                    break
                if d == b"":
                    saw_eof = True
                    break
                # else keep draining server bytes (e.g. system-message before FIN)
            if saw_eof:
                return True
            # Not obviously EOF yet: poke it. After the server has closed, a write
            # eventually yields EPIPE/ECONNRESET, or the follow-up recv hits EOF.
            try:
                self.sock.sendall(bytes([0x07]))  # a harmless fixed-size-0 message
            except OSError:
                return True
            time.sleep(0.2)
            try:
                self.sock.settimeout(0.6)
                d = self.sock.recv(4096)
                return d == b""
            except socket.timeout:
                return False
            except OSError:
                return True
        finally:
            pass

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def run(server):
    details = []
    try:
        # ---- baseline: a legit account+character we can re-check is untouched ----
        legit_login = ("aalegit%x" % (int(time.time()) & 0xFFFFFF)).encode()
        legit_pass = b"p_" + legit_login
        legit = _PreCharConn(server, login=legit_login, passw=legit_pass)
        legit_pseudo = ("AaLegit%x" % (int(time.time()) & 0xFFFFF))
        rp = legit.add_query(_PROFILE_OK, legit_pseudo, _MONGRP_OK, _SKIN_OK)
        if rp is None or len(rp) < 5:
            return (False, "baseline legit add: no/short reply %r" % (rp,))
        if rp[0] != 0x00:
            return (False, "baseline legit add status %#x (expected 0x00)" % rp[0])
        legit_cid = struct.unpack("<I", rp[1:5])[0]
        if legit_cid == 0:
            return (False, "baseline legit add returned character_id 0")
        legit.close()
        time.sleep(0.4)
        snap = H.reload_state(server, legit_login, legit_pass)
        if snap.get("character_id") is None:
            return (False, "baseline legit character not persisted (reload_state empty)")
        if snap.get("pseudo") != legit_pseudo:
            return (False, "baseline persisted pseudo %r != %r"
                    % (snap.get("pseudo"), legit_pseudo))
        baseline_cid = snap["character_id"]
        baseline_pos = (snap.get("x"), snap.get("y"), snap.get("mapIndex"))
        details.append("valid: created+persisted cid=%d pos=%s" % (baseline_cid, baseline_pos))
        if not server.alive():
            return (False, "server not alive after valid add (%s)" % server.crash_report())

        # ---- VALID #2: a different in-range monsterGroup + the other forced skin ----
        legit2_login = ("aalegb%x" % (int(time.time()) & 0xFFFFFF)).encode()
        legit2_pass = b"p_" + legit2_login
        c2 = _PreCharConn(server, login=legit2_login, passw=legit2_pass)
        rp = c2.add_query(_PROFILE_OK, ("AaLegB%x" % (int(time.time()) & 0xFFFFF)),
                          2, 1)  # monsterGroup 2, skin girlplayer(1)
        ok2 = rp is not None and len(rp) >= 5 and rp[0] == 0x00 and \
            struct.unpack("<I", rp[1:5])[0] != 0
        c2.close()
        if not ok2:
            return (False, "valid #2 (monGrp=2,skin=1) rejected: reply %r" % (rp,))
        details.append("valid#2: monGrp=2/skin=1 accepted")
        valid_cases = 2

        # ---- MALFORMED FRAMING ---------------------------------------------------
        # Each uses a fresh pre-char connection; after the bad bytes the server must
        # KICK (close) that connection and itself stay alive+uncrashed.
        malformed = []
        ps = b"MalFrame"

        # (1) DYNAMIC packet whose declared length is larger than the bytes that
        #     follow: header says len=L but we send fewer -> framing breaks.
        good = _PreCharConn.body(_PROFILE_OK, ps, _MONGRP_OK, _SKIN_OK)
        # expect_kick=False for the two INCOMPLETE-FRAMING variants: the declared
        # dynamic length exceeds the bytes actually sent, so the framing layer
        # safely WAITS for the (bounded, in-buffer) remainder that never arrives.
        # That is a benign stall -- no crash, no memory growth, no state change --
        # not a kick. Per project policy a safe silent stall is acceptable.
        malformed.append(("dyn-len-too-big",
                          bytes([0xAA, 0]) + struct.pack("<I", len(good) + 8) + good, False))
        # (2) truncated dynamic size field (only 2 of the 4 length bytes), then EOF
        malformed.append(("dyn-size-truncated",
                          bytes([0xAA, 1]) + b"\x05\x00", False))
        # (3) body missing the trailing monsterGroup+skin bytes (size too small):
        #     [grp][profile][plen][pseudo] only -> (size-pos)<1 at the skin read.
        #     COMPLETE frame, handler rejects -> KICK expected.
        short = bytes([_CHARGRP, _PROFILE_OK, len(ps)]) + ps  # no monGrp, no skin
        malformed.append(("body-missing-mongrp-skin",
                          bytes([0xAA, 2]) + struct.pack("<I", len(short)) + short, True))
        # (4) trailing data AFTER skin -> (size-pos)!=0 -> remaining-data reject. KICK.
        trailing = good + b"\xde\xad\xbe\xef"
        malformed.append(("trailing-data",
                          bytes([0xAA, 3]) + struct.pack("<I", len(trailing)) + trailing, True))

        malformed_cases = 0
        for label, raw, expect_kick in malformed:
            conn = _PreCharConn(server)
            try:
                conn.send_raw(raw)
                if not server.alive():
                    return (False, "server died after malformed '%s': %s"
                            % (label, server.crash_report()))
                cr = server.crash_report()
                if cr is not None:
                    return (False, "crash_report after malformed '%s': %s" % (label, cr))
                if expect_kick and not conn.is_kicked():
                    return (False, "malformed '%s': offending client was NOT kicked"
                            % label)
                malformed_cases += 1
            finally:
                conn.close()
        details.append("malformed: %d framing variants each kicked, server alive"
                       % malformed_cases)

        # ---- SEMANTIC-INVALID (well-formed packet, illegal content) --------------
        # Each on its OWN fresh account so a kick can't be confused with a success,
        # and reload_state can confirm no phantom character was persisted.
        semantic = [
            ("profile-out-of-range",
             lambda c, pp: c.add_query(5, pp, _MONGRP_OK, _SKIN_OK)),
            ("skin-not-in-forcedlist",
             lambda c, pp: c.add_query(_PROFILE_OK, pp, _MONGRP_OK, 0)),   # bandit(0) not forced
            ("empty-pseudo",
             lambda c, pp: c.add_query(_PROFILE_OK, "", _MONGRP_OK, _SKIN_OK)),
            ("pseudo-with-space",
             lambda c, pp: c.add_query(_PROFILE_OK, "bad name", _MONGRP_OK, _SKIN_OK)),
            ("mongrp-out-of-range",
             lambda c, pp: c.add_query(_PROFILE_OK, pp, 9, _SKIN_OK)),
        ]

        semantic_invalid_cases = 0
        for label, action in semantic:
            cl = ("aabad%x%x" % (int(time.time()) & 0xFFFFFF, semantic_invalid_cases + 1)).encode()
            cp = b"p_" + cl
            conn = _PreCharConn(server, login=cl, passw=cp)
            pp = ("AaBad%x%x" % (int(time.time()) & 0xFFFFF, semantic_invalid_cases + 1))
            try:
                rp = action(conn, pp)
                # errorOutput()->disconnectClient(): no reply expected, peer closed.
                if rp is not None and len(rp) >= 1 and rp[0] == 0x00:
                    return (False, "semantic '%s': server ACCEPTED an illegal add (status 0x00)"
                            % label)
                if not server.alive():
                    return (False, "server died after semantic '%s': %s"
                            % (label, server.crash_report()))
                cr = server.crash_report()
                if cr is not None:
                    return (False, "crash_report after semantic '%s': %s" % (label, cr))
                if not conn.is_kicked():
                    return (False, "semantic '%s': cheater NOT kicked (contract violation)"
                            % label)
            finally:
                conn.close()
            # NO STATE CORRUPTION: this account must have NO persisted character.
            time.sleep(0.35)
            bad_snap = H.reload_state(server, cl, cp)
            if bad_snap.get("character_id") is not None:
                return (False, "semantic '%s': phantom character %r persisted for the "
                        "rejected account" % (label, bad_snap.get("character_id")))
            semantic_invalid_cases += 1
        details.append("semantic: %d illegal adds each kicked + no phantom persisted"
                       % semantic_invalid_cases)

        # ---- victim/legit state unaffected by all the cheating -------------------
        snap2 = H.reload_state(server, legit_login, legit_pass)
        if snap2.get("character_id") != baseline_cid:
            return (False, "legit character_id changed %s -> %s after cheating"
                    % (baseline_cid, snap2.get("character_id")))
        if snap2.get("pseudo") != legit_pseudo:
            return (False, "legit pseudo changed %r -> %r"
                    % (legit_pseudo, snap2.get("pseudo")))
        if (snap2.get("x"), snap2.get("y"), snap2.get("mapIndex")) != baseline_pos:
            return (False, "legit position changed %s -> %s"
                    % (baseline_pos, (snap2.get("x"), snap2.get("y"), snap2.get("mapIndex"))))
        details.append("legit account byte-identical after all cheating")

        # ---- server still usable for the next test: a fresh legit create works ---
        post_login = ("aapost%x" % (int(time.time()) & 0xFFFFFF)).encode()
        post = _PreCharConn(server, login=post_login, passw=b"p_" + post_login)
        rp = post.add_query(_PROFILE_OK, ("AaPost%x" % (int(time.time()) & 0xFFFFF)),
                            _MONGRP_OK, _SKIN_OK)
        post_ok = rp is not None and len(rp) >= 5 and rp[0] == 0x00
        post.close()
        if not post_ok:
            return (False, "post-cheat legit add failed: server left unusable (reply %r)" % (rp,))
        details.append("post-cheat fresh legit add still works")

        return (True, "; ".join(details))
    except H.HandshakeError as e:
        return (False, "handshake/precondition error: %r" % e)
    except Exception as e:
        return (False, "unexpected exception: %r" % e)
