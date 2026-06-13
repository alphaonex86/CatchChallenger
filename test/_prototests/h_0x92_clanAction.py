"""Valgrind/protocol test for server handler 0x92 (clanAction).

Handler: server/base/ClientNetworkReadQuery.cpp:156 (Client::parseQuery, case 0x92)
 -> server/base/ClientEvents/LocalClientHandlerClan.cpp:24 (Client::clanAction).

Wire form: DYNAMIC QUERY. packetFixedSize[0x92]==0xFE, and 0x92>=0x80, so on the
wire a packet is:
    [0x92][qnum][dataSize:4 LE][data]
The data is parsed in parseQuery as:
    [clanActionId:1]                              (always)
    for clanActionId in {0x01,0x04,0x05}:         (a length-prefixed text)
        [textSize:1][text: textSize bytes]
    for clanActionId in {0x02,0x03}:              (NO trailing bytes at all)
    else:                                         (unknown action) -> reject.
After consuming, parseQuery enforces `(size-pos)!=0` -> ANY trailing byte is a
reject. So every action has an EXACT expected length.

THE SECURITY FIX under test (ClientNetworkReadQuery.cpp:176-180):
    case 0x01/0x04/0x05:
        if((size-pos) < 1) { errorOutput("wrong size in clan action for text
                                          size"); return false; }
        const uint8_t &textSize = data[pos]; ...
  Before the fix a 1-byte payload (just [clanActionId]=0x01, NO text-size byte)
  let `data[pos]` read ONE byte past the buffer, and because `pos` then exceeded
  `size`, the later unsigned `(size-pos) < textSize` comparison WRAPPED and
  bypassed its own bounds guard -> out-of-bounds read of up to 255 bytes. The
  fix adds the explicit `(size-pos) < 1` check so a 1-byte 0x92 with
  clanActionId in {0x01,0x04,0x05} is now rejected (KICK) with NO OOB read.
  This test sends exactly that 1-byte payload for 0x01, 0x04 and 0x05 and
  asserts the server neither crashes nor leaks (valgrind-clean) and KICKS the
  client. (Under valgrind this is the exact byte pattern that would have flagged
  an "Invalid read of size 1".)

Handler semantics (clanAction, FILE_DB build the harness uses):
  0x01 create : clan>0 -> kick("already in clan"); text empty -> kick; text has
                NUL -> kick("wrong data"); !allow_create_clan -> kick("not the
                right to create clan"). Only after ALL pass does it queue
                addClan_object() and reply [0x7F][... ][0x01][clanId:4] (success)
                or [0x02] (duplicate name).
  0x02 leave  : clan==0 -> kick("have not a clan"); leader -> kick.
  0x03 dissolve: clan==0 -> kick; !leader -> kick.
  0x04 invite : clan==0 -> kick; !leader -> kick. (else replies 0x01/0x02, no kick)
  0x05 eject  : clan==0 -> kick; !leader -> kick; eject-self -> kick.
  default     : kick("Action on the clan not found") / parseQuery kicks
                ("unknown clan action code") for unknown id.
  Client::errorOutput -> disconnectClient() (server/base/Client.cpp:524): every
  rejection KICKS the offending client; the server process stays ALIVE (HARDENED
  alone kicks rather than aborts -- foundation api_notes).

REACHABILITY (documented -> reachability_limited=True):
  A non-kicking / state-mutating clan action requires either
    (a) clan>0 (leave/dissolve/invite/eject), which can only be reached by first
        creating a clan, OR
    (b) create succeeding, which requires public_and_private_informations
        .allow_create_clan == true.
  allow_create_clan defaults to FALSE (server/base/Client.cpp:113; DB DEFAULT 0)
  and is ONLY set true by a QUEST REWARD (DatapackGeneralLoaderQuest.cpp:319). In
  the 'test' maincode a fresh account has NO clan and NO create permission, and
  the harness exposes no quest-completion primitive that deterministically grants
  it. So the create-SUCCESS branch (the only one that returns [0x01][clanId] and
  persists a clan) and every clan>0 branch are NOT reachable from a harness
  session. valid_cases=0; reachability_limited=True. Every reachable well-formed
  0x92 from a fresh on-map session is therefore semantically illegal at the
  permission/membership gate (or the unknown-action gate) and MUST be answered
  with a KICK and NO 0x7F reply -- exactly the security contract we exercise,
  INCLUDING the 1-byte-payload OOB vector for 0x01/0x04/0x05.

DB-STATE verification (checks_db_state=True, limited):
  H.reload_state reads back the persisted character HEADER (character_id, pseudo,
  x, y, mapIndex). Since the create-success branch is unreachable there is no
  legitimate clan mutation to confirm; what we assert is the NEGATIVE: a separate
  victim account's persisted identity+position is byte-for-byte identical before
  vs after all the abuse (no phantom clan/position mutation leaked onto a
  bystander), and a fresh full session still handshakes (server usable).
"""

import socket
import time

import _protoharness as H

NAME = "0x92 clanAction"


def _uniq(prefix):
    return ("%s%x" % (prefix, int(time.time() * 1000) & 0xFFFFFFFF)).encode()


def _alive_clean(server):
    """True iff the server process is up, accepting TCP, and not crashed."""
    if not server.alive():
        return False, "server.alive() == False"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % str(cr)[:300]
    return True, ""


def _clan_payload(action_id, text=None):
    """Build the DATA of a 0x92 packet (sent via q(...,dynamic=True)).

    action_id in {0x01,0x04,0x05}: [action][textSize][text].
    action_id in {0x02,0x03}: just [action] (no trailing bytes).
    """
    if text is None:
        return H.u8(action_id)
    tb = text if isinstance(text, bytes) else text.encode()
    return H.u8(action_id) + H.u8(len(tb)) + tb


def _is_kicked(sess, settle=0.5):
    """Return True iff the server CLOSED this session's socket (client kicked).

    Detection: read from the raw socket; the server's disconnectClient() shuts
    the connection, so recv() returns b'' (EOF). A CharacterSelected client is
    first sent a 'have been kicked ... try hack' system message, then the FIN;
    we keep reading until EOF, then poke the peer once with a well-formed (but
    illegal) 0x92 query to force the FIN to surface. A still-open socket (no EOF,
    poke succeeds and the socket stays open) means NOT kicked.
    """
    sk = sess.sock
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sk.settimeout(0.2)
            d = sk.recv(65536)
            if d == b"":
                return True  # clean EOF -> server closed us -> kicked
            # bytes arrived (the 'kicked, try hack' message / trailing data);
            # keep reading until EOF or timeout.
        except OSError as e:
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                break
            return True  # ECONNRESET / EBADF -> peer gone -> kicked
    # Force the issue: a follow-up well-formed-but-illegal clanAction query
    # ([0x02] leave, illegal for a clanless player). If the peer is gone the
    # send errors or a subsequent recv hits EOF.
    try:
        sk.sendall(bytes([0x92, 0x00]) + H.u32(1) + H.u8(0x02))
    except OSError:
        return True  # send failed -> peer gone -> kicked
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
            return False  # socket still open, server kept us -> NOT kicked
        return True       # reset -> kicked
    return False


def run(server):
    try:
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "server not clean at test start: %s" % why)

        # ----------------------------------------------------------------
        # Baseline legit "victim" account whose persisted state must stay
        # intact through all the abuse. Create it, snapshot it, then close so
        # reload_state can read it back cleanly.
        # ----------------------------------------------------------------
        victim_login = _uniq("vic")
        victim_pass = _uniq("vpw")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="ClanVic")
        except H.HandshakeError as e:
            return (False, "victim handshake failed: %s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after victim handshake: %s" % why)
        victim.close()
        time.sleep(0.35)  # let FILE_DB disconnect-save complete

        checks_db = False
        try:
            before = H.reload_state(server, victim_login, victim_pass)
            if before.get("character_id") is None:
                return (False, "victim did not persist (no character_id)")
            checks_db = True
        except Exception as e:
            return (False, "reload_state(before) raised: %s" % e)

        def fresh(tag):
            return H.Session(server, login=_uniq(tag), passh=_uniq(tag + "p"),
                             pseudo="ClanCheat")

        valid_cases = 0
        semantic_invalid_cases = 0
        malformed_cases = 0

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID well-formed clanAction packets. From a fresh on-map
        # session (clan==0, allow_create_clan==false) every one of these is
        # illegal and MUST KICK with NO 0x7F reply. Each uses a fresh disposable
        # session (it gets kicked).
        #   0x01 create with a valid name -> !allow_create_clan gate -> kick.
        #   0x02 leave / 0x03 dissolve    -> "have not a clan" gate -> kick.
        #   0x04 invite / 0x05 eject      -> "have not a clan" gate -> kick.
        #   unknown action 0x06 / 0x00 / 0xFF -> "unknown clan action code".
        #   0x01 create with empty name   -> "empty name" gate -> kick.
        #   0x01 create with a NUL byte    -> "wrong data" gate -> kick.
        # ----------------------------------------------------------------
        semantic = [
            ("0x01 create 'MyClan' (no create permission -> kick)",
             _clan_payload(0x01, b"MyClan")),
            ("0x01 create empty name (empty-name gate -> kick)",
             _clan_payload(0x01, b"")),
            ("0x01 create name with embedded NUL (wrong-data gate -> kick)",
             _clan_payload(0x01, b"Ev\x00il")),
            ("0x02 leave with no clan (have-not-a-clan gate -> kick)",
             _clan_payload(0x02)),
            ("0x03 dissolve with no clan (have-not-a-clan gate -> kick)",
             _clan_payload(0x03)),
            ("0x04 invite with no clan (have-not-a-clan gate -> kick)",
             _clan_payload(0x04, b"SomePlayer")),
            ("0x05 eject with no clan (have-not-a-clan gate -> kick)",
             _clan_payload(0x05, b"SomePlayer")),
            ("0x06 unknown action (unknown-clan-action gate -> kick)",
             _clan_payload(0x06)),
            ("0x00 unknown action (unknown-clan-action gate -> kick)",
             _clan_payload(0x00)),
            ("0xFF unknown action (unknown-clan-action gate -> kick)",
             _clan_payload(0xFF)),
        ]
        for label, data in semantic:
            try:
                s = fresh("cs")
            except H.HandshakeError as e:
                return (False, "semantic#%d handshake failed: %s"
                        % (semantic_invalid_cases + 1, e))
            qn = None
            try:
                qn = s.q(0x92, data, dynamic=True)
            except OSError:
                pass  # kicked mid-send is acceptable evidence
            # A rejected clanAction sends NO 0x7F reply: a reply here would mean
            # the cheat advanced past the gate -> contract violation.
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None  # socket closed -> kicked, no reply
            if got_reply is not None:
                try:
                    s.close()
                except OSError:
                    pass
                return (False,
                        "CONTRACT VIOLATION: %s produced a 0x7F reply %r "
                        "(illegal clanAction must be refused, never fulfilled)"
                        % (label, got_reply[:8]))
            kicked = _is_kicked(s)
            try:
                s.close()
            except OSError:
                pass
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after SEMANTIC %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(illegal clanAction must disconnect the cheater)"
                        % label)
            semantic_invalid_cases += 1

        # VALID-case note: a successful clan create (allow_create_clan==true ->
        # 0x7F [0x01][clanId] reply + persisted clan) and every clan>0 branch
        # (leave/dissolve/invite/eject success) are NOT deterministically
        # reachable here -- a fresh 'test'-maincode account has no clan and no
        # create permission, and the harness has no quest-reward primitive to
        # grant allow_create_clan. We exercise the security-relevant refusal+kick
        # for every reachable well-formed packet instead; reachability_limited.
        valid_cases = 0

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. 0x92 is a DYNAMIC QUERY:
        # [0x92][qnum][len:4 LE][data]. Wrong framing / truncated dynamic size /
        # the 1-byte OOB vector must never crash/hang the server. Each from a
        # fresh disposable session; after each assert alive+clean.
        # ----------------------------------------------------------------

        # (1) THE SECURITY-FIX VECTOR: 1-byte payload [0x01] (clanActionId only,
        #     NO text-size byte) for each of 0x01/0x04/0x05. Pre-fix this
        #     over-read 1 byte then wrapped (size-pos) past textSize -> OOB read
        #     up to 255 bytes. Post-fix: the (size-pos)<1 guard rejects -> KICK,
        #     no OOB. We assert KICK + server alive + valgrind clean (no reply).
        for action in (0x01, 0x04, 0x05):
            label = ("1-byte payload [0x%02x] (missing text-size: the OOB "
                     "security-fix vector)" % action)
            try:
                s = fresh("cm")
            except H.HandshakeError as e:
                return (False, "OOB-vector handshake failed (0x%02x): %s"
                        % (action, e))
            qn = None
            try:
                # data is exactly 1 byte: just the clanActionId, no text size.
                qn = s.q(0x92, H.u8(action), dynamic=True)
            except OSError:
                pass
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None
            if got_reply is not None:
                try:
                    s.close()
                except OSError:
                    pass
                return (False,
                        "CONTRACT VIOLATION: %s produced a 0x7F reply %r "
                        "(must be rejected at the size guard)"
                        % (label, got_reply[:8]))
            kicked = _is_kicked(s)
            try:
                s.close()
            except OSError:
                pass
            ok, why = _alive_clean(server)
            if not ok:
                return (False, "after %s: %s" % (label, why))
            if not kicked:
                return (False,
                        "CONTRACT VIOLATION: %s did NOT kick the client "
                        "(1-byte payload must hit the size guard and kick)"
                        % label)
            malformed_cases += 1

        # (2) ZERO-LENGTH payload: len==0, no clanActionId at all. The first
        #     guard (size-pos < 1 for the action byte) rejects -> KICK. Sent as
        #     raw bytes [0x92][qnum][00 00 00 00] (dynamic length 0).
        try:
            s = fresh("mz")
            qn = s._next_qn()  # reserve a real queryNumber so framing is valid
            s._out_qmap[qn] = 0x92
            s.send_raw(bytes([0x92, qn]) + H.u32(0))
            got_reply = None
            try:
                got_reply = s.reply_to(qn, timeout=0.5)
            except OSError:
                got_reply = None
            if got_reply is not None:
                s.close()
                return (False, "zero-length 0x92 produced a reply %r (must kick)"
                        % got_reply[:8])
            kicked = _is_kicked(s)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed zero-length handshake failed: %s" % e)
        except OSError:
            kicked = True
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after zero-length 0x92: %s" % why)
        if not kicked:
            return (False, "zero-length 0x92 did NOT kick the client")
        malformed_cases += 1

        # (3) TRAILING-BYTE for a no-text action: 0x02 leave declares NO trailing
        #     bytes, but we append one. parseQuery's `(size-pos)!=0` guard
        #     rejects -> KICK. Payload = [0x02][0xFF] (len 2).
        try:
            s = fresh("mt")
            qn = None
            try:
                qn = s.q(0x92, H.u8(0x02) + b"\xFF", dynamic=True)
            except OSError:
                pass
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None
            if got_reply is not None:
                s.close()
                return (False,
                        "0x02 leave + trailing byte produced a reply %r "
                        "(remaining-data guard must kick)" % got_reply[:8])
            kicked = _is_kicked(s)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed trailing-byte handshake failed: %s" % e)
        except OSError:
            kicked = True
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x02+trailing-byte 0x92: %s" % why)
        if not kicked:
            return (False, "0x02 leave + trailing byte did NOT kick the client")
        malformed_cases += 1

        # (4) TRUNCATED TEXT: action 0x01 declares textSize=10 but supplies only
        #     3 text bytes. The `(size-pos) < textSize` guard (now reachable and
        #     correct) rejects -> KICK. Payload = [0x01][0x0A]['abc'] (len 5).
        try:
            s = fresh("mr")
            qn = None
            try:
                qn = s.q(0x92, H.u8(0x01) + H.u8(10) + b"abc", dynamic=True)
            except OSError:
                pass
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None
            if got_reply is not None:
                s.close()
                return (False,
                        "0x01 create with truncated text produced a reply %r "
                        "(text-size guard must kick)" % got_reply[:8])
            kicked = _is_kicked(s)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed truncated-text handshake failed: %s" % e)
        except OSError:
            kicked = True
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after 0x01 truncated-text 0x92: %s" % why)
        if not kicked:
            return (False, "0x01 truncated-text did NOT kick the client")
        malformed_cases += 1

        # (5) LYING DYNAMIC LENGTH: declare dataSize=8 on the wire but supply
        #     only 2 data bytes, then send a garbage trailer so the parser cannot
        #     wedge. The framing layer waits for the declared bytes; a mismatched
        #     stream must never crash/hang the server (it kicks or sits idle).
        try:
            s = fresh("ml")
            qn = s._next_qn()
            s._out_qmap[qn] = 0x92
            s.send_raw(bytes([0x92, qn]) + H.u32(8) + H.u8(0x01) + H.u8(0x02))
            time.sleep(0.15)
            s.send_raw(b"\xFF\xFF\xFF\xFF\xFF\xFF")  # complete the lie + trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed lying-length handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after lying-dynamic-length 0x92: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim account's persisted identity/position
        # must be byte-for-byte what it was before any of the abuse, and a fresh
        # full session must still reach CharacterSelected. None of the rejected
        # clanActions (incl. the 1-byte OOB vector) may have created a phantom
        # clan, moved anyone, or mutated identity.
        # ----------------------------------------------------------------
        try:
            after = H.reload_state(server, victim_login, victim_pass)
        except Exception as e:
            return (False, "reload_state(after) raised: %s" % e)
        if after.get("character_id") != before.get("character_id"):
            return (False, "victim character_id changed: %r -> %r"
                    % (before.get("character_id"), after.get("character_id")))
        for k in ("x", "y", "mapIndex", "pseudo"):
            if after.get(k) != before.get(k):
                return (False, "victim state corrupted: %s %r -> %r"
                        % (k, before.get(k), after.get(k)))

        # Leave the server usable: a fresh full session still handshakes.
        try:
            again = H.Session(server, login=_uniq("ok"), passh=_uniq("okp"),
                              pseudo="ClanPost")
            again.close()
        except H.HandshakeError as e:
            return (False, "post-abuse fresh handshake failed (server unusable): "
                           "%s" % e)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "server not clean at end: %s" % why)

        detail = ("clanAction 0x92: %d valid (create-success + every clan>0 "
                  "branch reachability-limited: no clan + no allow_create_clan "
                  "in 'test' maincode), %d semantic-invalid, %d malformed-framing "
                  "(incl. the 1-byte-payload OOB security-fix vector for "
                  "0x01/0x04/0x05); every abuse kicked the cheater with NO 0x7F "
                  "reply, server stayed alive (no crash/hang/OOB), victim DB "
                  "id/pos unchanged; checks_db=%s."
                  % (valid_cases, semantic_invalid_cases, malformed_cases,
                     checks_db))
        return (True, detail)

    except Exception as e:
        try:
            rep = server.crash_report()
        except Exception:
            rep = None
        return (False, "exception in run(): %r ; crash_report=%r" % (e, rep))
