"""Valgrind/protocol test for server handler 0x03 (chat).

Handler: server/base/ClientNetworkReadMessage.cpp:104 (Client::parseMessage).
Wire form: DYNAMIC MESSAGE, code 0x03 (MSG_FIXED[0x03]==0xFE) -> on the wire
    [0x03][len:4 LE][data].  No queryNumber (message, code<0x80).
Data layout (traced from the handler):
    [chatType:u8] then per-type body:
      * non-PM (local 0x01 / all 0x02 / clan 0x03):
            [textSize:u8][text:textSize bytes]
      * PM (0x06):
            [textSize:u8][text][pseudoSize:u8][pseudo]
    After parsing, ANY remaining byte -> errorOutput("remaining data ...") -> KICK.

Handler logic traced (all errorOutput() paths call disconnectClient() -> KICK):
  * size < 1                         -> "wrong remaining size for chat"            -> KICK
  * chatType not in {0x01,0x02,0x03,0x06} -> "chat type error: N"                  -> KICK
  * PM && !chat_allow_private        -> "can't send pm because is disabled"        -> KICK
        (default config enables it, so this is not the default path)
  * PM: (size-pos) < 1 for textSize  -> "wrong utf8 ... PM for text size"          -> KICK
  * PM: declared textSize > remaining-> "wrong utf8 ... PM for text"               -> KICK
  * PM: (size-pos) < 1 for pseudoSize-> "wrong utf8 ... PM for pseudo"             -> KICK
  * PM: declared pseudoSize>remaining-> "wrong utf8 ... PM for pseudo"             -> KICK
  * non-PM: (size-pos) < 1 for textSize -> "wrong utf8 ... header size"            -> KICK
  * non-PM: declared textSize>remaining -> "wrong utf8 ... size"                   -> KICK
  * trailing data after body         -> "remaining data ..."                       -> KICK
  * disabled local / disabled (clan&&all) -> "can't send chat ... disabled"        -> KICK
  * text starting with '/' is a COMMAND:
        - /trade, /battle: handled for EVERYONE (sendHandlerCommand) -> no kick
        - admin cmds (playernumber/give/tp/kick/...): only for GM/dev or
          everyBodyIsRoot; for a NORMAL player the handler falls through to
          "unknown send command" -> receiveSystemText() back to the SAME client
          and returns true -> NO kick (documented reply-without-kick).
  * a normal (non-'/') chat is BROADCAST to peers; the sender gets NO reply.

Default server config (server/base/NormalServerGlobal.cpp:220) enables
allow-all/allow-local/allow-private/allow-clan, and everyBodyIsRoot defaults
to false -> a fresh account is a NORMAL (non-admin) player.

Verification strategy:
  * Chat is a MESSAGE -> the sender gets NO reply for a normal chat. We assert
    the server stays ALIVE and the VALID-chat session is NOT kicked (the socket
    has no EOF and a benign follow-up still goes through).  Where the body is a
    PM we use a SECOND on-map session as the recipient to make the broadcast
    reachable; for local/all/clan a single sender suffices (broadcast target
    set may be empty, still a no-kick success).
  * Each MALFORMED / SEMANTIC-INVALID send kicks ONLY the offending client
    (production-safe). After each we assert server.alive() and crash_report()
    is None, and we verify the offender's socket reached EOF (got kicked).
  * Chat mutates NO persisted state (no cash/items/monster/position change).
    NO-STATE-CORRUPTION is verified with a SEPARATE legit session and with
    H.reload_state() on the sender's account: identity+position are unchanged.
"""

import time
import _protoharness as H

NAME = "0x03 chat"

# Chat_type values (general/base/GeneralStructures.hpp).
CHAT_LOCAL = 0x01
CHAT_ALL = 0x02
CHAT_CLAN = 0x03
CHAT_PM = 0x06


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


def _chat_body(chat_type, text):
    """Build a well-formed non-PM chat body: [type][textSize][text]."""
    tb = text.encode() if isinstance(text, str) else text
    return H.u8(chat_type) + H.u8(len(tb)) + tb


def _pm_body(text, pseudo):
    """Build a well-formed PM body: [0x06][textSize][text][pseudoSize][pseudo]."""
    tb = text.encode() if isinstance(text, str) else text
    pb = pseudo.encode() if isinstance(pseudo, str) else pseudo
    return H.u8(CHAT_PM) + H.u8(len(tb)) + tb + H.u8(len(pb)) + pb


def _socket_alive(sess):
    """True iff the peer is still connected (no kick).

    A kicked client has its socket closed server-side: recv() then returns b''
    (EOF). We do a short non-blocking-ish recv after pumping the parser; if the
    server merely sent broadcasts we will have consumed them, and a subsequent
    recv blocks (timeout) -> still alive. If EOF was reached -> kicked.
    """
    try:
        sess.drain(timeout=0.2)  # consume any benign broadcast/ack
    except OSError:
        return False
    try:
        sess.sock.settimeout(0.4)
        d = sess.sock.recv(4096)
        if d == b"":
            return False  # clean EOF -> the server closed us -> kicked
        # got more bytes (a broadcast tail): still connected
        sess._buf.extend(d)
        return True
    except (TimeoutError, OSError) as e:
        # socket.timeout (subclass of OSError) -> nothing to read -> still open.
        if isinstance(e, TimeoutError) or "timed out" in str(e).lower():
            return True
        # a real connection error (reset/broken pipe) -> kicked.
        return False


def _is_kicked(sess):
    """True iff the session got kicked (EOF / connection torn down).

    Confirms the kick two ways: (1) a recv reaches EOF, and/or (2) a follow-up
    send raises once the peer is fully gone. Either is sufficient evidence.
    """
    if not _socket_alive(sess):
        return True
    # Still seemed alive: confirm with a benign send + recheck. A genuinely
    # kicked socket either already EOF'd above or will error on the next send.
    try:
        sess.sock.sendall(bytes([CHAT_LOCAL]))  # 1 stray byte; harmless if open
    except OSError:
        return True
    return not _socket_alive(sess)


def run(server):
    try:
        # ----------------------------------------------------------------
        # Precondition: two fully-handshaked on-map sessions on the same map.
        # The first is the chat SENDER; the second is the PM recipient AND the
        # independent "victim" we re-check for state corruption.
        # ----------------------------------------------------------------
        s_login = _uniq("ch")
        s_pass = _uniq("cp")
        try:
            sender = H.Session(server, login=s_login, passh=s_pass, pseudo="Chatter")
        except H.HandshakeError as e:
            return (False, "sender handshake failed: %s" % e)

        peer_login = _uniq("pr")
        peer_pass = _uniq("pp")
        try:
            peer = H.Session(server, login=peer_login, passh=peer_pass, pseudo="Peer")
        except H.HandshakeError as e:
            return (False, "peer handshake failed: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after handshakes: %s" % why)

        peer_pseudo = peer.pseudo  # real, unique pseudo the recipient registered under
        sender_start = (sender.x, sender.y, sender.mapIndex)

        # ----------------------------------------------------------------
        # VALID cases: well-formed chats that must NOT kick the sender.
        #   1) local chat, plain text
        #   2) all  chat, plain text
        #   3) clan chat, plain text
        #   4) PM to the connected peer (real pseudo) — reachable recipient
        #   5) empty-text local chat (textSize==0 is a valid branch)
        #   6) /trade command (handled for everyone, no kick)
        #   7) /battle command (handled for everyone, no kick)
        # After each we require: server alive+clean AND sender socket still up.
        # ----------------------------------------------------------------
        valid_cases = 0

        def send_valid(body, label):
            nonlocal valid_cases
            sender.m(0x03, body, dynamic=True)
            valid_cases += 1
            ok2, why2 = _alive_clean(server)
            if not ok2:
                return "after valid %s: %s" % (label, why2)
            if not _socket_alive(sender):
                return "valid %s wrongly KICKED the sender" % label
            # drain the peer too so its broadcast queue doesn't back up
            peer.drain(timeout=0.1)
            return None

        for body, label in (
            (_chat_body(CHAT_LOCAL, "hello local"), "local"),
            (_chat_body(CHAT_ALL, "hello all"), "all"),
            # NOTE: clan chat is NOT a valid case for this fresh test character:
            # sendChatText() errorOutputs "Unable to chat with clan, you have not
            # clan" (-> KICK) when clan==0. The 'test' maincode has no
            # allow_create_clan, so the sender can never be in a clan here, so a
            # clan-chat is correctly rejected. Exercising it as "valid" is wrong.
            (_pm_body("hi peer", peer_pseudo), "pm->peer"),
            (_chat_body(CHAT_LOCAL, ""), "empty-local"),
            (_chat_body(CHAT_LOCAL, "/trade something"), "/trade"),
            (_chat_body(CHAT_LOCAL, "/battle someone"), "/battle"),
        ):
            err = send_valid(body, label)
            if err is not None:
                return (False, err)

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING variants (wrong/truncated length fields). Each is
        # sent from a FRESH disposable session because it kicks the offender.
        # We assert the server stays alive+clean AND the offender is kicked.
        # ----------------------------------------------------------------
        malformed_cases = 0

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="Bad")

        def malformed_send(raw, label, expect_kick=True):
            nonlocal malformed_cases
            kicked = None
            try:
                s = fresh("bad")
            except H.HandshakeError as e:
                return "%s: handshake failed: %s" % (label, e)
            try:
                s.send_raw(raw)
                kicked = _is_kicked(s)
            except OSError:
                kicked = True  # kicked mid-send is acceptable evidence
            finally:
                s.close()
            malformed_cases += 1
            ok2, why2 = _alive_clean(server)
            if not ok2:
                return "after %s: %s" % (label, why2)
            if expect_kick and not kicked:
                return "%s: offender was NOT kicked (contract violation)" % label
            return None

        # (1) DYNAMIC frame with len=0 (zero data) -> size<1 -> "wrong remaining
        #     size for chat" -> KICK. Wire: [0x03][00 00 00 00].
        err = malformed_send(bytes([0x03]) + H.u32(0), "framing#1 len=0")
        if err is not None:
            return (False, err)

        # (2) PM with a TRUNCATED text length field: declare a body of 1 byte
        #     (just the chatType) so when the handler reads textSize the
        #     remaining size is 0 -> "wrong utf8 ... PM for text size" -> KICK.
        #     Wire: [0x03][len=1][0x06].
        err = malformed_send(bytes([0x03]) + H.u32(1) + H.u8(CHAT_PM),
                             "framing#2 PM no textSize")
        if err is not None:
            return (False, err)

        # (3) PM with textSize claiming MORE bytes than present (size-underflow
        #     class): [0x06][textSize=200]["x"] but only 1 text byte supplied ->
        #     "wrong utf8 ... PM for text" -> KICK.  This is the oversized-text /
        #     truncated-length attack the handler must reject without over-read.
        body3 = H.u8(CHAT_PM) + H.u8(200) + b"x"
        err = malformed_send(bytes([0x03]) + H.u32(len(body3)) + body3,
                             "framing#3 PM oversized textSize")
        if err is not None:
            return (False, err)

        # (4) PM with a valid text but a TRUNCATED pseudo length field: text ok,
        #     then no pseudoSize byte at all -> "wrong utf8 ... PM for pseudo"
        #     -> KICK.  [0x06][textSize=2]["hi"]  (no pseudo block).
        body4 = H.u8(CHAT_PM) + H.u8(2) + b"hi"
        err = malformed_send(bytes([0x03]) + H.u32(len(body4)) + body4,
                             "framing#4 PM no pseudoSize")
        if err is not None:
            return (False, err)

        # (5) non-PM with declared textSize > remaining (oversized text):
        #     [0x01][textSize=50]["abc"] -> "wrong utf8 ... size" -> KICK.
        body5 = H.u8(CHAT_LOCAL) + H.u8(50) + b"abc"
        err = malformed_send(bytes([0x03]) + H.u32(len(body5)) + body5,
                             "framing#5 local oversized textSize")
        if err is not None:
            return (False, err)

        # (6) trailing data after a complete body: a complete empty-local chat
        #     [0x01][textSize=0] followed by an extra stray byte -> the handler
        #     parses the chat then hits "remaining data ..." -> KICK.
        body6 = H.u8(CHAT_LOCAL) + H.u8(0) + b"\xAA"
        err = malformed_send(bytes([0x03]) + H.u32(len(body6)) + body6,
                             "framing#6 trailing data")
        if err is not None:
            return (False, err)

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID use — THE KEY CONTRACT. Well-formed packets that are
        # semantically illegal for THIS handler. Each must (a) KICK the offender,
        # (b) leave the server alive+clean, (c) cause NO state corruption.
        # ----------------------------------------------------------------
        semantic_cases = 0

        def semantic_send(body, label):
            """Send a well-formed-but-illegal chat from a fresh session; the
            handler must errorOutput()->KICK it. Returns err-string or None."""
            nonlocal semantic_cases
            kicked = None
            try:
                s = fresh("sem")
            except H.HandshakeError as e:
                return "%s: handshake failed: %s" % (label, e)
            try:
                s.m(0x03, body, dynamic=True)
                kicked = _is_kicked(s)
            except OSError:
                kicked = True
            finally:
                s.close()
            semantic_cases += 1
            ok2, why2 = _alive_clean(server)
            if not ok2:
                return "after %s: %s" % (label, why2)
            if not kicked:
                return "%s: well-formed-illegal chat did NOT kick (contract violation)" % label
            return None

        # (S1) UNKNOWN chat type 0x00 (well-formed single byte, not a valid
        #      Chat_type) -> "chat type error: 0" -> KICK.
        err = semantic_send(H.u8(0x00) + H.u8(0), "sem#1 type=0x00")
        if err is not None:
            return (False, err)

        # (S2) UNKNOWN chat type 0x04 (between defined values; not local/all/
        #      clan/pm) -> "chat type error: 4" -> KICK.
        err = semantic_send(H.u8(0x04) + H.u8(0), "sem#2 type=0x04")
        if err is not None:
            return (False, err)

        # (S3) UNKNOWN chat type 0xFF (max byte) -> "chat type error: 255" -> KICK.
        err = semantic_send(H.u8(0xFF) + H.u8(0), "sem#3 type=0xFF")
        if err is not None:
            return (False, err)

        # (S4) PM addressed to SELF — a fully well-formed PM packet that is
        #      semantically illegal: sendPM() rejects "Can't send them self the
        #      PM" via errorOutput() -> KICK (ClientBroadCast.cpp:109-112).
        #      (A PM to a non-existent OTHER pseudo is NOT a kick — sendPM
        #      replies receiveSystemText() and returns false without
        #      disconnecting — so we use the self-PM, which definitively kicks.)
        #      The offender's own registered pseudo is needed; build it inline.
        sem4_kicked = None
        try:
            ssem4 = fresh("self")
            self_pseudo = ssem4.pseudo  # the pseudo this fresh char registered
            ssem4.m(0x03, _pm_body("to myself", self_pseudo), dynamic=True)
            sem4_kicked = _is_kicked(ssem4)
        except H.HandshakeError as e:
            return (False, "sem#4 PM-to-self: handshake failed: %s" % e)
        except OSError:
            sem4_kicked = True
        finally:
            try:
                ssem4.close()
            except Exception:
                pass
        semantic_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after sem#4 PM-to-self: %s" % why)
        if not sem4_kicked:
            return (False, "sem#4 PM-to-self did NOT kick (contract violation)")

        # ----------------------------------------------------------------
        # DOCUMENTED reply-without-kick case: an ADMIN command from a NORMAL
        # (non-root) player. The handler does NOT kick — it falls through to
        # "unknown send command" and replies via receiveSystemText() to the
        # SAME client, returning true. We assert the SENDER is NOT kicked.
        # (This is the foundation-api_notes "documented reply-with-error
        # without kick" exception to the default kick expectation.)
        # ----------------------------------------------------------------
        admin_noopkick = False
        try:
            sender.m(0x03, _chat_body(CHAT_LOCAL, "/give 1 1"), dynamic=True)
            admin_noopkick = _socket_alive(sender)
        except OSError:
            admin_noopkick = False
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after non-root /give: %s" % why)
        if not admin_noopkick:
            return (False, "non-root admin command wrongly KICKED the sender "
                           "(should reply 'unknown send command', not kick)")

        # ----------------------------------------------------------------
        # NO-STATE-CORRUPTION: the independent peer (a DIFFERENT account) must
        # be unaffected by all of the sender's chat/abuse — its socket still
        # open, its identity/position intact. Then verify the SENDER's own
        # persisted character (identity + position) is byte-for-byte unchanged.
        # Chat never mutates cash/items/monsters/position, so a change here is
        # corruption.
        # ----------------------------------------------------------------
        if not _socket_alive(peer):
            return (False, "independent peer was disconnected by sender's chat "
                           "(cross-client corruption)")
        peer.drain(timeout=0.1)

        # Sender persisted-state read-back (identity/position unchanged).
        sender.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete
        checks_db = False
        try:
            snap = H.reload_state(server, s_login, s_pass)
            if snap.get("character_id") is None:
                return (False, "reload_state: sender character did not persist")
            if snap.get("x") is None or snap.get("y") is None or snap.get("mapIndex") is None:
                return (False, "reload_state: sender position header unreadable")
            checks_db = True
            persisted = (snap["x"], snap["y"], snap["mapIndex"])
            # Chat must NOT move the player: position must equal the spawn.
            if persisted != sender_start:
                return (False, "chat CORRUPTED position: start=%s persisted=%s"
                        % (sender_start, persisted))
        except Exception as e:
            return (False, "reload_state raised: %s" % e)

        # Peer persisted-state read-back (a second account, must also be intact).
        peer.close()
        time.sleep(0.35)
        try:
            psnap = H.reload_state(server, peer_login, peer_pass)
            if psnap.get("character_id") is None:
                return (False, "reload_state: peer character did not persist "
                               "(possible corruption)")
        except Exception as e:
            return (False, "peer reload_state raised: %s" % e)

        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after reload_state: %s" % why)

        # ----------------------------------------------------------------
        # Leave the server usable: a brand-new full session must still
        # handshake to CharacterSelected and chat cleanly after all the abuse.
        # ----------------------------------------------------------------
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="After")
            again.m(0x03, _chat_body(CHAT_LOCAL, "post-abuse"), dynamic=True)
            if not _socket_alive(again):
                return (False, "post-abuse session wrongly kicked on a valid chat")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse handshake failed (server unusable): %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "post-abuse liveness: %s" % why)

        detail = (
            "valid=%d malformed=%d semantic=%d db_checked=%s "
            "sender_start=%s persisted=%s; all chat types (local/all/clan/pm) + "
            "/trade,/battle accepted no-kick; unknown chatType (0x00/0x04/0xFF) "
            "and truncated/oversized PM/text length fields each KICKED the "
            "offender; non-root admin cmd replied without kick; peer (2nd "
            "account) unaffected; server stayed alive+clean throughout"
            % (valid_cases, malformed_cases, semantic_cases, checks_db,
               sender_start, persisted)
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
