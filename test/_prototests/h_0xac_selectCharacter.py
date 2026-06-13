"""Valgrind/protocol test for server handler 0xAC (selectCharacter).

Handler: server/base/ClientNetworkReadQuery.cpp:546 (Client::parseQuery, case 0xAC).
Wire form: FIXED-size QUERY, code 0xAC, queryNumber, exactly 9 data bytes:
    [charactersGroupIndex:u8][serverUniqueKey/?:u32][characterId:u32]
0xAC >= 0x80 so it carries a queryNumber; packetFixedSize[0xAC]==9 so it is
FIXED size (NO 4-byte dynamic length on the wire). Wire bytes:
    [0xAC][qnum][grp:1][key:4 LE][characterId:4 LE]
parseQuery reads characterId at data+1+4 (loadLe32), skips the group byte and
the 4-byte key. (server/base/ClientNetworkReadQuery.cpp:566)

Handler logic traced
(parseQuery case 0xAC -> Client::selectCharacter(query_id, characterId)
 in server/base/ClientLoad/ClientHeavyLoadSelectCharCommon.cpp:21, which under
 FILE_DB runs selectCharacter_object()->selectCharacter_return() synchronously):

  parseQuery (case 0xAC):
    * stat == CharacterSelected  -> errorOutput("charaters is logged, deny ...")
          -> KICK, NO reply.  (this is the DOUBLE-SELECT guard)
    * stat != Logged             -> errorOutput("charaters is logged, deny ...")
          -> KICK, NO reply.  (must be in ClientStat::Logged to select)
    * HARDENED: size != 1+4+4 (i.e. != 9)
          -> errorOutput("wrong size with the main ident: 172, ...") -> KICK.

  selectCharacter_return (FILE_DB path):
    The characterId is looked up in the account's persisted CharacterEntry list
    (database/common/accounts/<account_id>). If the id is NOT present in THIS
    account's list (nonexistent id OR an id that belongs to another account):
          -> characterSelectionIsWrong(query_id, 0x02, "Result return query
             wrong") which (a) sends a 0xAC reply whose 1-byte payload == 0x02
             then (b) errorOutput(...) -> KICK.
       *** THE NOT-OWNED / UNKNOWN-ID GUARD ***: the lookup is scoped to the
       connected account's own character list, so a characterId that exists for
       a DIFFERENT account AND a characterId that exists nowhere BOTH funnel
       through this same guard (reply 0x02 + kick). No OOB / no cross-account
       leak: a cheater can never select a character it does not own.
    If the id IS owned + valid: the block loads, a 0xAC reply with payload[0]==
    0x01 followed by the server/character/map block is sent, stat advances to
    CharacterSelected and the session STAYS connected. (SUCCESS branch.)
    Other refusal codes the same path can emit: 0x03 "Already logged" (the
    characterId is already in playerById_db), 0x04 (profile/starter/data issue).

  characterSelectionIsWrong (server/base/ClientLoad/ClientHeavyLoadSelectChar.cpp
  :17): sends the 0x7F reply [returnCode] AND THEN errorOutput(debugMessage).
  Client::errorOutput (server/base/Client.cpp:524) -> disconnectClient(). So a
  SEMANTIC-INVALID select both REPLIES (with the non-0x01 error byte) AND KICKS
  the offending client. This is the documented "reply-with-error then kick"
  variant: we accept the reply (it carries the rejection code) but STILL require
  the kick. The DOUBLE-SELECT / wrong-stat guards in parseQuery kick with NO
  reply (they return false directly, never reaching characterSelectionIsWrong).

REACHABILITY (the precondition is fully reachable):
  0xAC must be sent while the connection is in ClientStat::Logged (post-A8,
  pre-select). A harness Session auto-selects to CharacterSelected during its
  handshake, so it is the WRONG state to drive 0xAC from. We therefore drive a
  MANUAL handshake (A0 -> A8 [-> A9 -> A8]) that stops at Logged, using the
  harness primitives (_creds/blake3/PROTOCOL_HEADER_LOGIN/_RawConn), and send
  0xAC from there. This makes BOTH the valid select AND the semantic-invalid
  selects directly observable. reachability_limited=False.

Verification strategy:
  * VALID: create an account WITH a character via a full Session (adds+selects),
    close it, let FILE_DB persist. Then a fresh MANUAL Logged connection selects
    that owned characterId -> assert reply[0]==0x01, position block present, and
    the socket stays OPEN (NOT kicked). DB identity/position confirmed via
    H.reload_state.
  * SEMANTIC-INVALID (well-formed, illegal): from fresh MANUAL Logged
    connections select (a) a characterId that exists nowhere, (b) a characterId
    that belongs to a DIFFERENT account (not owned), (c) id 0. For EACH assert:
      (a) reply[0] is a NON-0x01 error byte (0x02 not-found / not-owned) AND the
          server KICKS the connection (socket EOF). A 0x01 success reply here, or
          a connection that stays open, is a CONTRACT VIOLATION.
      (b) server.alive() + crash_report() None after each.
      (c) no state corruption (checked at the end on a separate victim account).
  * DOUBLE-SELECT: from a manual Logged connection select a valid owned id
    (-> CharacterSelected), then send 0xAC AGAIN on the SAME socket. The
    parseQuery stat==CharacterSelected guard must KICK with NO reply.
  * MALFORMED-FRAMING: wrong-length / truncated / over-long / bogus-dynamic-len
    0xAC; after each assert server alive+clean.
  * NO STATE CORRUPTION: a separate victim account's persisted identity/position
    (H.reload_state) is byte-identical before and after all the abuse, and a
    brand-new full session still reaches CharacterSelected -> server unharmed.
"""

import time
import struct
import socket
import _protoharness as H

NAME = "0xAC selectCharacter"


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


def _sel_payload(character_id, group=0, key=0):
    """The 9-byte 0xAC fixed payload: [grp:1][key:4 LE][characterId:4 LE]."""
    return bytes([group & 0xFF]) + struct.pack("<I", key & 0xFFFFFFFF) + \
        struct.pack("<I", character_id & 0xFFFFFFFF)


class _Logged:
    """A raw connection driven MANUALLY through A0 -> A8 [-> A9 -> A8] so it
    stops at ClientStat::Logged WITHOUT selecting a character. From here we can
    send 0xAC ourselves and observe the real handler contract (a normal harness
    Session auto-selects to CharacterSelected, the wrong state for 0xAC).

    Built on the harness's own framing primitives so the wire bytes are exactly
    what the server expects.
    """

    def __init__(self, server, login, passh, create=True):
        self.server = server
        self.account_chars = []   # list of (character_id, pseudo) owned by this account
        self.sock = socket.create_connection(("127.0.0.1", server.port), timeout=5)
        self.sock.settimeout(0.4)
        self._conn = H._RawConn(self.sock)
        loginHash, passHash = H._creds(login, passh)
        # A0 protocol header
        pa = self._conn.query(0xA0, H.PROTOCOL_HEADER_LOGIN, False, timeout=3.0)
        if pa is None or len(pa) < 17:
            raise H.HandshakeError("A0: no/short reply")
        if pa[0] != 0x04:
            raise H.HandshakeError("A0: compression %#x != 0x04" % pa[0])
        token = pa[1:17]
        # A8 login
        a8 = loginHash + H.blake3(passHash + token)
        pa = self._conn.query(0xA8, a8, False, timeout=3.0)
        if pa is None or len(pa) < 1:
            raise H.HandshakeError("A8: no reply")
        if pa[0] == 0x07:
            if not create:
                raise H.HandshakeError("A8: account does not exist (0x07)")
            a9 = H.blake3(loginHash) + passHash
            pa9 = self._conn.query(0xA9, a9, False, timeout=3.0)
            if pa9 is None or pa9[0] != 0x01:
                raise H.HandshakeError("A9 create failed")
            pa = self._conn.query(0xA8, a8, False, timeout=3.0)
            if pa is None or pa[0] != 0x01:
                raise H.HandshakeError("A8 re-login failed")
        elif pa[0] != 0x01:
            raise H.HandshakeError("A8: status %#x" % pa[0])
        # Now in ClientStat::Logged with the char-list block in pa.
        self._a8reply = pa
        self.account_chars = self._parse_all_chars(pa)
        # Server-log byte offset at the moment this connection went live. is_kicked()
        # greps ": Kicked by:" only past here so a prior sequential connection's kick
        # can't false-match (sequential connections per test).
        try:
            with open(server.log_path, "rb") as _f:
                self._log_base = len(_f.read())
        except OSError:
            self._log_base = 0

    @staticmethod
    def _parse_all_chars(a8reply):
        """Return every (character_id, pseudo) in the A8(0x01) char-list block."""
        out = []
        try:
            d = a8reply
            pos = 1 + 4 + 1 + 1 + 1 + 1 + 2 + 32
            mir = d[pos]; pos += 1 + mir
            grp = d[pos]; pos += 1
            g = 0
            while g < grp:
                cnt = d[pos]; pos += 1
                c = 0
                while c < cnt:
                    cid = struct.unpack("<I", d[pos:pos+4])[0]; pos += 4
                    pl = d[pos]; pos += 1
                    pseudo = d[pos:pos+pl].decode(errors="replace"); pos += pl
                    pos += 1            # skin
                    pos += 4            # delete_time_left
                    out.append((cid, pseudo))
                    c += 1
                g += 1
        except Exception:
            pass
        return out

    def select(self, character_id, group=0, key=0, timeout=5.0):
        """Send a well-formed 0xAC select and return its 0x7F reply payload (or
        None on timeout / socket close)."""
        return self._conn.query(0xAC, _sel_payload(character_id, group, key),
                                 False, timeout=timeout)

    def send_raw(self, raw):
        self.sock.sendall(raw)

    def is_kicked(self, settle=0.5):
        """True iff the server CLOSED this connection (client kicked).

        Detection: read from the socket; disconnectClient() shuts the connection
        so recv() returns b'' (EOF). We then poke the peer with a COMPLETE 0x02
        move [0x02,0x01,0x05]: against a closed peer it raises OSError or the next
        recv sees EOF (-> kicked); against a still-open peer no EOF is seen
        (-> NOT kicked). The poke MUST NOT be another 0xAC select: a session that
        just VALIDLY selected is now CharacterSelected, so a second select would
        trip the double-select guard and KICK it -- the poke would manufacture the
        very kick we are testing for (false positive). A move never kicks
        (moveThePlayer()'s result is discarded) and a move in the wrong state is
        silently dropped, so it is a safe universal poke.
        """
        # Authoritative for a pre-select kick: errorOutput() logs ": Kicked by:"
        # (socketString prefix -- no character pseudo yet). A logged-in
        # disconnectClient() DEFERS the socket close, so the socket probe below can
        # miss it; the log does not. Scoped to >= _log_base (this connection's
        # lifetime) so an earlier connection's kick can't false-match.
        try:
            with open(self.server.log_path, "r", errors="replace") as _f:
                _f.seek(self._log_base)
                if ": Kicked by:" in _f.read():
                    return True
        except OSError:
            pass
        sk = self.sock
        deadline = time.time() + settle
        while time.time() < deadline:
            try:
                sk.settimeout(0.2)
                d = sk.recv(65536)
                if d == b"":
                    return True
                # trailing bytes (e.g. the error reply / system message); keep reading
            except OSError as e:
                if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                    break
                return True
        try:
            sk.sendall(bytes([0x02, 0x01, 0x05]))  # benign move poke (never kicks)
        except OSError:
            return True
        try:
            sk.settimeout(0.4)
            d = sk.recv(65536)
            if d == b"":
                return True
            try:
                d2 = sk.recv(65536)
                if d2 == b"":
                    return True
            except OSError as e:
                if not (isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout"):
                    return True
        except OSError as e:
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                return False     # socket still open -> NOT kicked
            return True
        return False

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def _make_account_with_char(server, tag):
    """Create an account that OWNS one character (via a full Session that adds +
    selects), then close it and let FILE_DB persist. Returns
    (login, passh, character_id)."""
    login = _uniq(tag)
    passh = _uniq(tag + "p")
    sess = H.Session(server, login=login, passh=passh, pseudo="Owner")
    cid = sess.character_id
    sess.close()
    time.sleep(0.35)            # let the disconnect-save complete
    return login, passh, cid


def run(server):
    try:
        # ----------------------------------------------------------------
        # Victim account whose persisted state must stay intact through all the
        # abuse. Create it (owns one character), snapshot it, keep its creds.
        # ----------------------------------------------------------------
        try:
            victim_login, victim_pass, victim_cid = _make_account_with_char(server, "vic")
        except H.HandshakeError as e:
            return (False, "victim setup failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim setup: %s" % why)

        checks_db = False
        try:
            before = H.reload_state(server, victim_login, victim_pass)
            if before.get("character_id") is None:
                return (False, "victim did not persist (no character_id)")
            checks_db = True
        except Exception as e:
            return (False, "reload_state(before) raised: %s" % e)

        # A SECOND owned account (its character belongs to nobody-but-itself);
        # used as the "not owned by me" cross-account id for the cheater.
        try:
            other_login, other_pass, other_cid = _make_account_with_char(server, "oth")
        except H.HandshakeError as e:
            return (False, "other-account setup failed: %s" % e)
        try:
            other_before = H.reload_state(server, other_login, other_pass)
        except Exception as e:
            return (False, "reload_state(other before) raised: %s" % e)

        valid_cases = 0
        semantic_invalid_cases = 0
        malformed_cases = 0

        # ----------------------------------------------------------------
        # VALID select: a fresh MANUAL Logged connection on the VICTIM account
        # selects the victim's OWN, existing characterId. Must reply 0x01 + a
        # position block and STAY connected (advance to CharacterSelected).
        # We use a fresh login (not the victim's, to avoid touching it twice)
        # that we create-with-character here.
        # ----------------------------------------------------------------
        try:
            good_login, good_pass, good_cid = _make_account_with_char(server, "good")
            conn = _Logged(server, good_login, good_pass, create=False)
        except H.HandshakeError as e:
            return (False, "valid-case Logged handshake failed: %s" % e)
        if good_cid is None or not conn.account_chars:
            try: conn.close()
            except OSError: pass
            return (False, "valid-case: account has no owned character to select")
        owned_cid = conn.account_chars[0][0]
        try:
            pa = conn.select(owned_cid)
        except OSError:
            pa = None
        if pa is None or len(pa) < 1:
            try: conn.close()
            except OSError: pass
            return (False, "valid select: no reply for owned characterId %d" % owned_cid)
        if pa[0] != 0x01:
            try: conn.close()
            except OSError: pass
            return (False,
                    "valid select of OWNED id %d returned error byte %#x "
                    "(expected 0x01 success)" % (owned_cid, pa[0]))
        if len(pa) < 2:
            try: conn.close()
            except OSError: pass
            return (False, "valid select 0x01 but no position/character block")
        # Must NOT be kicked: a successful select keeps the session connected.
        if conn.is_kicked():
            try: conn.close()
            except OSError: pass
            return (False, "valid select succeeded (0x01) but the server KICKED "
                           "the connection (success must stay connected)")
        valid_cases += 1
        try:
            conn.close()
        except OSError:
            pass
        time.sleep(0.3)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after valid select: %s" % why)

        # DB confirms the selected character persisted with stable identity.
        try:
            good_after = H.reload_state(server, good_login, good_pass)
            if good_after.get("character_id") != good_cid:
                return (False, "valid-case character_id changed in DB: %r -> %r"
                        % (good_cid, good_after.get("character_id")))
        except Exception as e:
            return (False, "reload_state(good after) raised: %s" % e)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID well-formed 0xAC selects, each from a FRESH manual
        # Logged connection (it gets kicked). Every one must: reply with a
        # NON-0x01 error byte (0x02) AND kick the connection. A 0x01 here, or a
        # connection left open, is a contract violation. Cases:
        #   * characterId far above any persisted id (0xFFFFFFFE) -> not-found.
        #   * characterId = other account's OWN char (NOT owned by us)  ->
        #       cross-account select must be refused (security-critical).
        #   * characterId = 0 (no such character) -> not-found.
        # ----------------------------------------------------------------
        sem_cases = [
            (0xFFFFFFFE, "characterId=0xFFFFFFFE (no such character)"),
            (other_cid,  "characterId=%d (belongs to ANOTHER account, NOT owned)"
                         % (other_cid if other_cid is not None else -1)),
            (0,          "characterId=0 (no such character)"),
        ]
        for cid, label in sem_cases:
            if cid is None:
                # other_cid unknown -> skip just this sub-case but don't fail the
                # whole test on a setup gap; substitute another nonexistent id.
                cid, label = 0x7FFFFFFF, "characterId=0x7FFFFFFF (no such character)"
            # each cheater needs its OWN logged-in account so that the target id
            # is genuinely not in ITS list.
            try:
                cl = _uniq("ch")
                cp = _uniq("chp")
                cheat = _Logged(server, cl, cp, create=True)
            except H.HandshakeError as e:
                return (False, "semantic#%d Logged handshake failed: %s"
                        % (semantic_invalid_cases + 1, e))
            pa = None
            try:
                pa = cheat.select(cid, timeout=3.0)
            except OSError:
                pa = None  # kicked mid/before-reply is acceptable
            if pa is not None and len(pa) >= 1 and pa[0] == 0x01:
                try: cheat.close()
                except OSError: pass
                return (False,
                        "CONTRACT VIOLATION: %s returned 0x01 SUCCESS "
                        "(selecting a not-owned/unknown character must be refused)"
                        % label)
            kicked = cheat.is_kicked()
            try:
                cheat.close()
            except OSError:
                pass
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(illegal select must disconnect the cheater)" % label)
            semantic_invalid_cases += 1

        # ----------------------------------------------------------------
        # DOUBLE-SELECT: a manual Logged connection selects its OWN id (success,
        # -> CharacterSelected), then sends 0xAC AGAIN on the SAME socket. The
        # parseQuery stat==CharacterSelected guard must KICK with NO reply.
        # ----------------------------------------------------------------
        try:
            dl, dp, dcid = _make_account_with_char(server, "dbl")
            dconn = _Logged(server, dl, dp, create=False)
        except H.HandshakeError as e:
            return (False, "double-select Logged handshake failed: %s" % e)
        if not dconn.account_chars:
            try: dconn.close()
            except OSError: pass
            return (False, "double-select: account has no owned character")
        dcid = dconn.account_chars[0][0]
        try:
            pa = dconn.select(dcid)
        except OSError:
            pa = None
        if pa is None or len(pa) < 1 or pa[0] != 0x01:
            try: dconn.close()
            except OSError: pass
            return (False, "double-select: first (valid) select did not succeed: %r"
                    % (pa[:4] if pa else None))
        # second select on the same, now-CharacterSelected connection -> kick, no reply
        second_reply = None
        try:
            second_reply = dconn.select(dcid, timeout=1.5)
        except OSError:
            second_reply = None
        if second_reply is not None and len(second_reply) >= 1 and second_reply[0] == 0x01:
            try: dconn.close()
            except OSError: pass
            return (False, "CONTRACT VIOLATION: double-select returned 0x01 "
                           "(a second selectCharacter while CharacterSelected "
                           "must be refused)")
        kicked = dconn.is_kicked()
        try:
            dconn.close()
        except OSError:
            pass
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after double-select: %s" % why)
        if not kicked:
            return (False, "CONTRACT VIOLATION: double-select did NOT kick the "
                           "client (CharacterSelected re-select must disconnect)")
        semantic_invalid_cases += 1

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. 0xAC is a FIXED-size QUERY: on the wire it is
        # [0xAC][qnum][9 data bytes], NO 4-byte length. Wrong framing must never
        # crash/hang the server (HARDENED size!=9 guard kicks; bad framing is
        # mis-parsed and kicked). Each from a fresh manual Logged connection;
        # after each assert alive+clean. Use raw bytes via send_raw.
        # ----------------------------------------------------------------
        def fresh_logged(tag):
            return _Logged(server, _uniq(tag), _uniq(tag + "p"), create=True)

        # (1) Truncated: code+qnum + only 4 data bytes (< 9). The fixed-size
        # framer waits for the remaining bytes; we follow with garbage so it
        # can't wedge, and the server must stay alive.
        try:
            s = fresh_logged("m1")
            s.send_raw(bytes([0xAC, 0x00]) + struct.pack("<I", 0))  # only 4 data bytes
            time.sleep(0.15)
            s.send_raw(b"\xFF\xFF\xFF\xFF\xFF")                      # garbage trailer
            time.sleep(0.15)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated (4-data-byte) 0xAC: %s" % why)

        # (2) Over-long: code+qnum + 10 data bytes (> 9). The framer reads 9 as
        # the packet, leaving 1 stray byte to mis-frame the next packet -> the
        # parser must reject/kick, never crash.
        try:
            s = fresh_logged("m2")
            s.send_raw(bytes([0xAC, 0x00]) + _sel_payload(1) + b"\x00")  # 9 + 1 stray
            time.sleep(0.15)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long (10-data-byte) 0xAC: %s" % why)

        # (3) code+qnum, ZERO data bytes. The framer blocks waiting for the 9
        # fixed data bytes; send garbage to ensure no wedge, server stays alive.
        try:
            s = fresh_logged("m3")
            s.send_raw(bytes([0xAC, 0x00]))                # code + qnum, no data
            time.sleep(0.15)
            s.send_raw(b"\xAA\xBB\xCC\xDD\xEE\xFF\x11\x22\x33")  # complete then trailer
            time.sleep(0.15)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare code+qnum 0xAC: %s" % why)

        # (4) FIXED-size query 0xAC sent WITH a spurious 4-byte dynamic length
        # (as if it were a dynamic query). 0xAC is FIXED, so the would-be length
        # bytes are mis-read as the 9 data bytes + next packet -> the parser must
        # reject/kick and the server must survive.
        try:
            s = fresh_logged("m4")
            s.send_raw(bytes([0xAC, 0x00]) + struct.pack("<I", 9) + _sel_payload(1))
            time.sleep(0.15)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus-dynamic-length 0xAC: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim and the other account's persisted
        # identity/position must be byte-for-byte what they were before any of
        # the abuse, and a brand-new full session must still reach
        # CharacterSelected. None of the rejected selects may have leaked across
        # accounts or mutated the server.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "reload_state(victim after) raised: %s" % e)
        if after.get("character_id") != before.get("character_id"):
            return (False, "victim character_id changed: %r -> %r"
                    % (before.get("character_id"), after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo"):
            if after.get(k) != before.get(k):
                return (False, "victim state corrupted: %s %r -> %r"
                        % (k, before.get(k), after.get(k)))

        try:
            other_after = H.reload_state(server, other_login, other_pass)
        except Exception as e:
            return (False, "reload_state(other after) raised: %s" % e)
        if other_after.get("character_id") != other_before.get("character_id"):
            return (False, "other-account character_id changed (cross-account "
                           "select leaked!): %r -> %r"
                    % (other_before.get("character_id"),
                       other_after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo"):
            if other_after.get(k) != other_before.get(k):
                return (False, "other-account state corrupted: %s %r -> %r"
                        % (k, other_before.get(k), other_after.get(k)))

        # Leave the server usable: a fresh full session still handshakes+selects.
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d (owned existing characterId from a Logged connection -> "
            "0x01 + position block, session stays connected) "
            "semantic_invalid=%d (each kicked: unknown id 0xFFFFFFFE, "
            "cross-account NOT-owned id %s, id 0 -> reply error byte then kick; "
            "plus double-select on a CharacterSelected connection -> kick no "
            "reply) malformed=%d db_checked=%s victim_pos=%s other_pos=%s "
            "(server stayed alive+clean; no cross-account leak / state corruption)"
            % (valid_cases, semantic_invalid_cases,
               other_cid, malformed_cases, checks_db,
               (before.get("x"), before.get("y"), before.get("mapIndex")),
               (other_before.get("x"), other_before.get("y"),
                other_before.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
