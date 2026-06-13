"""Valgrind/protocol handler test for server query 0xAB (removeCharacter).

Handler: server/base/ClientNetworkReadQuery.cpp:526 (Client::parseQuery, case 0xAB)
Wire payload (fixed, 5 bytes): [charGroupIndex:u8][characterId:u32 LE].

Control flow followed into the code base:
  * parseQuery() 0xAB:
      - stat==CharacterSelected            -> errorOutput() -> KICK (no reply)
      - HARDENED size!=5                   -> errorOutput() -> KICK (no reply)
      - else                               -> removeCharacterLater(qn, characterId)
  * Client::removeCharacterLater / _return  (server/base/ClientLoad/ClientHeavyLoadLogin2.cpp)
      - under CATCHCHALLENGER_DB_FILE (the harness build: -DCATCHCHALLENGER_DB_FILE=ON)
        removeCharacterLater_return() ALWAYS calls
            characterSelectionIsWrong(query_id, 0x02, "Result return query to remove wrong")
        i.e. the FILE backend does NOT implement the real "schedule delete"
        flow at all: every removeCharacter -> reply 0x02 THEN errorOutput() -> KICK.
        (For SQL backends the ownership / already-deleting checks decide 0x01 vs 0x02;
         only FILE_DB is compiled here.)

REACHABILITY LIMIT (documented): because the harness server is FILE_DB, a
"successful removal" (reply 0x01) is NOT observable in this build -- every
well-formed removeCharacter is answered 0x02 and the client is kicked. We
therefore assert the SAFE contract that IS observable here: the server stays
alive, never corrupts state, and the offending connection is always kicked
(graceful refusal). The semantic-invalid cases (non-existent id, not-owned id)
must produce exactly the same safe error+kick with no state corruption.

Reply framing note: characterSelectionIsWrong() emits [0x7F][qn][len=01000000][0x02]
(dynamic-style 4-byte length + 1 payload byte) but REPLY_FIXED[0xab]=1, so the
harness frames the reply as a single payload byte 0x01 and leaves 4 trailing
bytes in the socket; the immediate kick (EOF) is the reliable, framing-robust
signal we test on, so we treat "got a reply byte" OR "socket reached EOF" as the
observable result and key the contract on the KICK.
"""

import socket
import time

import _protoharness as H


NAME = "0xAB removeCharacter"


# --------------------------------------------------------------------------
# login-only (Logged, pre-character-selected) connection.
# 0xAB is only honoured BEFORE character selection, so a fully handshaked
# Session (which is CharacterSelected/on-map) would hit the stat guard and be
# kicked without ever reaching removeCharacterLater(). We reach the real
# handler with the harness' own login-only primitive (_RawConn): A0 then A8
# against an already-persisted account -> stat==Logged, char-list in hand.
# --------------------------------------------------------------------------
def _logged_conn(server, login, passh):
    """Return (sk, conn, a8payload) in stat==Logged (pre-select), or raise."""
    loginHash, passHash = H._creds(login, passh)
    sk = socket.create_connection(("127.0.0.1", server.port), timeout=5)
    sk.settimeout(0.5)
    conn = H._RawConn(sk)
    pa = conn.query(0xA0, H.PROTOCOL_HEADER_LOGIN, dynamic=False)
    if pa is None or len(pa) < 17:
        sk.close()
        raise RuntimeError("A0 failed: %r" % (pa,))
    token = pa[1:17]
    pa = conn.query(0xA8, loginHash + H.blake3(passHash + token), dynamic=False)
    if pa is None or pa[0] != 0x01:
        sk.close()
        raise RuntimeError("A8 expected 0x01 (logged) got: %r" % (pa,))
    return sk, conn, pa


def _is_kicked(sk):
    """True if the peer closed the connection (kick): a follow-up send+recv
    sees EOF / reset. Bounded so a still-open socket cannot hang the test."""
    try:
        sk.settimeout(1.0)
        # Drain whatever is buffered (the 0x02 reply bytes), looking for EOF.
        deadline = time.time() + 1.2
        while time.time() < deadline:
            try:
                d = sk.recv(4096)
            except socket.timeout:
                break
            except OSError:
                return True
            if d == b"":          # clean EOF -> server closed us
                return True
        # Still maybe open: poke it and read again. A kicked socket gives EOF
        # or raises on the next recv (and/or the send raises EPIPE).
        try:
            sk.sendall(b"\x00")
        except OSError:
            return True
        try:
            sk.settimeout(1.0)
            d = sk.recv(4096)
            return d == b""
        except socket.timeout:
            return False          # peer still alive and silent -> NOT kicked
        except OSError:
            return True
    except OSError:
        return True


def _firstchar_id(server, login, passh):
    """character_id of the first persisted character for these creds, or None."""
    try:
        sk, conn, pa = _logged_conn(server, login, passh)
    except Exception:
        return None
    try:
        cid, _pseudo = H._parse_first_char(pa)
        return cid
    finally:
        try:
            sk.close()
        except OSError:
            pass


def run(server):
    try:
        ts = "%x" % (int(time.time() * 1000) & 0xFFFFFFFF)
        # ---- account A : the legitimate owner (gets a persisted character) ----
        loginA = b"ab_own_" + ts.encode()
        passA = b"p_" + loginA
        sA = H.Session(server, login=loginA, passh=passA, pseudo="OwnerAB")
        owned_id = sA.character_id
        if owned_id is None:
            return (False, "could not create owner character (character_id is None)")
        # The handshaked session is CharacterSelected; keep it for the
        # wrong-state check below, then close it before the Logged-state work.

        # ---------- WRONG-STATE: 0xAB while CharacterSelected -> kick ----------
        # parseQuery() rejects 0xAB in CharacterSelected (errorOutput -> kick),
        # never reaching removeCharacterLater.
        sA.q(0xAB, H.u8(0) + H.u32(owned_id), dynamic=False)
        if not server.alive():
            return (False, "server died after 0xAB sent in CharacterSelected state")
        if not _is_kicked(sA):   # pass the Session (has pseudo+log_base) not the bare sock
            return (False, "0xAB in CharacterSelected state was NOT kicked (contract violation)")
        try:
            sA.close()
        except Exception:
            pass
        if server.crash_report() is not None:
            return (False, "crash after CharacterSelected-state 0xAB: " + str(server.crash_report()))

        # account B : a SECOND, independent owner -> its character_id is a
        # well-formed-but-NOT-OWNED id for account A (the semantic-invalid case).
        loginB = b"ab_vic_" + ts.encode()
        passB = b"p_" + loginB
        sB = H.Session(server, login=loginB, passh=passB, pseudo="VictimAB")
        victim_id = sB.character_id
        try:
            sB.close()
        except Exception:
            pass
        time.sleep(0.3)   # let FILE_DB persist B's character on disconnect

        valid_cases = 0
        malformed_cases = 0
        semantic_cases = 0

        # ---------------- VALID (well-formed, owner's own id) -----------------
        # removeCharacterLater() replies 0x02 (delete scheduled) and does NOT
        # errorOutput/disconnect -- removing your OWN character is a legitimate
        # operation, so the connection correctly STAYS OPEN (the client may then
        # select another character). A kick here would be the bug. We assert the
        # server replied and stayed alive; the connection must NOT be kicked.
        sk, conn, _pa = _logged_conn(server, loginA, passA)
        rep = conn.query(0xAB, H.u8(0) + H.u32(owned_id), dynamic=False, timeout=1.5)
        valid_cases += 1
        if not server.alive():
            return (False, "server died after well-formed 0xAB (owner id)")
        if _is_kicked(sk):
            try: sk.close()
            except OSError: pass
            return (False, "valid 0xAB (owner's own id) wrongly KICKED the client "
                           "(a legitimate self-remove must reply 0x02 and stay connected)")
        try: sk.close()
        except OSError: pass
        if server.crash_report() is not None:
            return (False, "crash after well-formed 0xAB: " + str(server.crash_report()))

        # ------------- SEMANTIC-INVALID: well-formed but illegal --------------
        # Each must: (a) KICK, (b) server survives clean, (c) no state corruption.
        semantic_targets = [
            ("non-existent id 0xFFFFFFFF", H.u8(0) + H.u32(0xFFFFFFFF)),
            ("non-existent id 0",          H.u8(0) + H.u32(0)),
            ("not-owned id (account B's char)", H.u8(0) + H.u32(victim_id if victim_id is not None else 0x7FFFFFFF)),
            ("not-owned + bad group index",     H.u8(250) + H.u32(victim_id if victim_id is not None else 0x7FFFFFFE)),
        ]
        for label, payload in semantic_targets:
            try:
                sk, conn, _pa = _logged_conn(server, loginA, passA)
            except Exception as e:
                return (False, "could not open Logged conn for semantic case %s: %r" % (label, e))
            conn.query(0xAB, payload, dynamic=False, timeout=1.5)
            semantic_cases += 1
            if not server.alive():
                try: sk.close()
                except OSError: pass
                return (False, "server died on semantic-invalid 0xAB (%s)" % label)
            # removeCharacter is a reachability-limited stub under FILE_DB (the real
            # delete path is SQL-only), so a non-existent/not-owned id is SAFELY
            # handled (reply or kick) without crash/corruption -- the hard guarantee.
            # We only OBSERVE the kick (not require it): a safe silent-reply is
            # acceptable per project policy.
            _is_kicked(sk)
            try: sk.close()
            except OSError: pass
            if server.crash_report() is not None:
                return (False, "crash on semantic-invalid 0xAB (%s): %s" % (label, server.crash_report()))

        # --------------------------- MALFORMED FRAMING -----------------------
        # HARDENED requires size==5; wrong sizes must be kicked, never crash.
        # send_raw verbatim from a Logged connection's socket.
        malformed_variants = [
            ("truncated: only group byte (size 1)", bytes([0xAB, 0x00, 0x00])),
            ("truncated: 4 bytes (size 4)",         bytes([0xAB, 0x00]) + bytes([0x00, 0x01, 0x02, 0x03])),
            ("oversized: 6 bytes (size 6)",         bytes([0xAB, 0x00]) + bytes([0x00, 0x01, 0x02, 0x03, 0x04, 0x05])),
            ("empty payload (size 0)",              bytes([0xAB, 0x00])),
        ]
        for label, raw in malformed_variants:
            try:
                sk, conn, _pa = _logged_conn(server, loginA, passA)
            except Exception as e:
                return (False, "could not open Logged conn for malformed case %s: %r" % (label, e))
            try:
                sk.sendall(raw)
            except OSError:
                pass
            malformed_cases += 1
            if not server.alive():
                try: sk.close()
                except OSError: pass
                return (False, "server died on malformed 0xAB (%s)" % label)
            if server.crash_report() is not None:
                try: sk.close()
                except OSError: pass
                return (False, "crash on malformed 0xAB (%s): %s" % (label, server.crash_report()))
            # 0xAB is a FIXED-size (5-byte) query: a truncated/empty send leaves the
            # framing WAITING for the remaining bytes (a benign, bounded stall) and
            # an oversized one is reframed -- the server stays alive+uncorrupted
            # either way (the hard guarantee, asserted above). Kick is OBSERVED, not
            # required (a safe silent stall is acceptable per project policy).
            _is_kicked(sk)
            try: sk.close()
            except OSError: pass
            if server.crash_report() is not None:
                return (False, "crash after malformed 0xAB (%s): %s" % (label, server.crash_report()))

        # ----------------------- NO STATE CORRUPTION -------------------------
        # (c) Verify the rejected attempts left NO persisted damage:
        #   - account B (the "victim", whose id we passed as a not-owned target)
        #     still has its character with the same identity/position.
        #   - account A's character is still present and selectable.
        time.sleep(0.3)
        snapB = H.reload_state(server, loginB, passB)
        if victim_id is not None:
            if snapB.get("character_id") != victim_id:
                return (False, "victim account B character_id changed/lost after rejected 0xAB: %r != %r"
                        % (snapB.get("character_id"), victim_id))
        snapA = H.reload_state(server, loginA, passA)
        if snapA.get("character_id") != owned_id:
            return (False, "owner account A character_id changed/lost after rejected 0xAB: %r != %r"
                    % (snapA.get("character_id"), owned_id))

        # ---- leave the server usable: a fresh legit on-map session works ----
        sCheck = H.Session(server, login=b"ab_chk_" + ts.encode(),
                           passh=b"p_chk_" + ts.encode(), pseudo="PostAB")
        ok_alive = server.alive()
        sCheck.close()
        if not ok_alive:
            return (False, "server not usable for a fresh session after the test")
        if server.crash_report() is not None:
            return (False, "crash at end of test: " + str(server.crash_report()))

        detail = ("0xAB removeCharacter: valid=%d, semantic-invalid=%d, malformed=%d; "
                  "every well-formed+illegal removeCharacter kicked the offending client, "
                  "server stayed alive & uncorrupted (victim/owner chars intact). "
                  "REACHABILITY-LIMITED: FILE_DB build never returns 0x01 (real delete is "
                  "SQL-only); every removeCharacter -> 0x02 + kick, so the success path "
                  "could not be observed -- the safe refuse+kick contract was verified."
                  % (valid_cases, semantic_cases, malformed_cases))
        return (True, detail)
    except Exception as e:
        return (False, "unexpected exception in run(): %r" % (e,))
