"""Valgrind/protocol test for server handler 0x85 (useRecipe).

Handler: server/base/ClientNetworkReadQuery.cpp:44 (Client::parseQuery, case 0x85).
Wire form: FIXED-size QUERY, code 0x85, queryNumber, exactly 2 data bytes:
    [recipe_id:u16]
Because 0x85 >= 0x80 it carries a queryNumber; because packetFixedSize[0x85]==2
it is FIXED size (NO 4-byte dynamic length on the wire). Wire bytes are:
    [0x85][qnum][lo][hi]

Handler logic traced
(server/base/ClientNetworkReadQuery.cpp:44 -> server/base/crafting/LocalClientHandlerCrafting.cpp:32
 Client::useRecipe(query_id, recipe_id)):

  parseQuery (HARDENED):
    * size != sizeof(uint16_t) (i.e. != 2)
          -> errorOutput("wrong size with the main ident: 133, ...") -> KICK.

  useRecipe(query_id, recipe_id):
    1. recipe_id > CommonDatapack::get_craftingRecipesMaxId()
          -> errorOutput("recipe_id out of range: ...") -> KICK.
          *** THE SECURITY FIX ***: recipe_id comes straight off the wire as a
          full uint16 (0..65535) but the per-character recipes[] bitmap is only
          craftingRecipesMaxId/8+1 bytes, so an id above the max would index the
          bitmap out of bounds. The guard rejects BEFORE the bitmap access.
          With the guard in place this must NOT crash and must NOT leak under
          valgrind.
    2. recipe NOT owned: bit (recipes[id/8] & (1<<(7-id%8))) is 0
          -> errorOutput("The player have not this recipe registred: ...") -> KICK.
          A fresh character owns ZERO recipes (its bitmap is all-zero), so every
          in-range id 1..maxId is "not owned" and funnels through this kick.
    3. owned but insufficient material  -> errorOutput(...) -> KICK.
    4. owned + materials present        -> craft, removeObject(...), addObject(...),
          send 0x7F reply (RecipeUsage_ok / RecipeUsage_failed). (success branch)

  Client::errorOutput -> disconnectClient() (server/base/Client.cpp:524). So EVERY
  rejection of useRecipe KICKS the offending client; the server stays alive (the
  HARDENED-only parse-fail path here kicks rather than abort — see foundation
  api_notes). NO reply (0x7F) is sent on a rejection.

REACHABILITY (documented -> reachability_limited=True):
  The VALID success branch (step 4) needs the player to have LEARNED a recipe
  first. A recipe is learned only by USING the matching recipe-ITEM
  (server/base/ClientEvents/LocalClientHandlerObject.cpp:217-228 sets the
  recipes[] bit), and only if the player already owns that item and meets the
  reputation requirement. The 'test' maincode does not hand a fresh on-map
  character any recipe-item, and there is no reachable handler to grant items in
  this harness, so a learned-recipe state is NOT deterministically reachable.
  THEREFORE every WELL-FORMED 0x85 packet we can send from a reachable session is
  semantically ILLEGAL (out-of-range id, OR an in-range but not-owned id) and MUST
  be answered with a KICK and NO reply. We exercise exactly that security
  contract: the OOB-guard (id>maxId) and the not-owned guard. This is the
  security-relevant behaviour the handler note asks for ("recipe_id >
  craftingRecipesMaxId -> assert no crash/leak; recipe not owned -> reject").
  valid_cases=0 (success branch unreachable); reachability_limited=True.

Verification strategy:
  * useRecipe is a QUERY but a rejected one sends NO reply and disconnects the
    client. We confirm the contract by detecting the offending socket is CLOSED
    by the server (EOF) after the illegal send -> cheater kicked. A silent ignore
    (socket stays open, OR a 0x7F reply arrives) would be a contract violation.
  * After EACH send we assert server.alive() and crash_report() is None.
  * NO STATE CORRUPTION is checked with a SEPARATE legit account: its persisted
    identity/position (H.reload_state) is identical before and after all the
    abuse, and a fresh full session still handshakes -> the server is unharmed.
"""

import time
import _protoharness as H

NAME = "0x85 useRecipe"


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


def _is_kicked(sess, settle=0.4):
    """Return True iff the server CLOSED this session's socket (client kicked).

    Detection: read from the raw socket; the server's disconnectClient() shuts
    the connection, so recv() returns b'' (EOF). We poke the peer once more to
    force the FIN to be observed even if the first recv only drained buffered
    bytes (the server may push a 'kicked' system message just before the FIN).
    A still-open socket (timeout, no EOF, send succeeds) means NOT kicked.
    """
    sk = sess.sock
    deadline = time.time() + settle
    while time.time() < deadline:
        try:
            sk.settimeout(0.2)
            d = sk.recv(65536)
            if d == b"":
                return True  # clean EOF -> server closed us -> kicked
            # bytes arrived (system "kicked" message or trailing data); keep
            # reading until EOF or timeout.
        except OSError as e:
            if isinstance(e, TimeoutError) or e.__class__.__name__ == "timeout":
                break
            return True  # ECONNRESET / EBADF -> peer gone -> kicked
    # Force the issue: a write to a half-closed peer eventually errors, and a
    # follow-up recv on a server-closed socket returns EOF. We send a benign,
    # well-formed (but still illegal) useRecipe query as the poke.
    try:
        sk.sendall(bytes([0x85, 0x00, 0x09, 0x00]))  # qnum 0, recipe_id 9 (OOB)
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
        # ----------------------------------------------------------------
        # Baseline legit account whose persisted state must stay intact
        # through all the abuse. Create it, record its persisted snapshot,
        # then close it so reload_state can read it back cleanly.
        # ----------------------------------------------------------------
        victim_login = _uniq("vic")
        victim_pass = _uniq("vpw")
        try:
            victim = H.Session(server, login=victim_login, passh=victim_pass,
                               pseudo="Victim")
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
                             pseudo="Cheat")

        # ----------------------------------------------------------------
        # SEMANTIC-INVALID well-formed useRecipe packets. From a fresh on-map
        # session (owns ZERO recipes) every one of these is illegal:
        #   * recipe_id 9 / 0xFFFF  -> OOB guard (id > craftingRecipesMaxId,
        #                              which is 8 in the test datapack) -> KICK.
        #                              This is the OOB-bitmap security fix: the
        #                              guard must reject without crash/leak.
        #   * recipe_id 0 / 1 / 8   -> in range but NOT owned -> KICK.
        # For EACH we assert:
        #   (a) the offending client is KICKED (socket closed by server) and NO
        #       0x7F reply is sent,
        #   (b) the SERVER survives (alive + no crash),
        #   (c) it leaves no corrupt state (checked once at the end).
        # Each uses a FRESH disposable session because it gets kicked.
        # ----------------------------------------------------------------
        semantic_invalid_cases = 0
        for recipe_id, label in (
                (0xFFFF, "recipe_id=0xFFFF (far out of range -> OOB-bitmap guard)"),
                (9,      "recipe_id=9 (just above craftingRecipesMaxId=8 -> OOB guard)"),
                (0,      "recipe_id=0 (in range but not owned)"),
                (1,      "recipe_id=1 (in range, exists in datapack, but not owned)"),
                (8,      "recipe_id=8 (max id, in range, but not owned)")):
            try:
                s = fresh("cr")
            except H.HandshakeError as e:
                return (False, "semantic#%d handshake failed: %s"
                        % (semantic_invalid_cases + 1, e))
            qn = None
            try:
                qn = s.q(0x85, H.u16(recipe_id))  # well-formed query, illegal id
            except OSError:
                pass  # kicked mid-send is acceptable evidence
            # A rejected useRecipe sends NO reply: a 0x7F reply here would be a
            # contract violation (the cheat must be refused, not crafted).
            got_reply = None
            if qn is not None:
                try:
                    got_reply = s.reply_to(qn, timeout=0.5)
                except OSError:
                    got_reply = None  # socket closed -> kicked, no reply
            if got_reply is not None:
                return (False,
                        "CONTRACT VIOLATION: %s produced a 0x7F reply %r "
                        "(illegal recipe must be refused, never crafted)"
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
                        "(illegal useRecipe must disconnect the cheater)"
                        % label)
            semantic_invalid_cases += 1

        # VALID-case note: a successful craft (recipe owned + materials present
        # -> 0x7F RecipeUsage_ok/failed, inventory mutated) is NOT
        # deterministically reachable here (needs a learned recipe, which needs
        # a recipe-item the test maincode does not grant). We assert the
        # security-relevant refusal+kick for every reachable well-formed packet
        # instead; reachability_limited=True.
        valid_cases = 0

        # ----------------------------------------------------------------
        # MALFORMED-FRAMING cases. 0x85 is a FIXED-size QUERY: on the wire it is
        # [0x85][qnum][2 data bytes], NO 4-byte length. Wrong framing must never
        # crash/hang the server (HARDENED size!=2 guard kicks; bad framing is
        # mis-parsed and kicked). Each from a fresh disposable session; after
        # each assert alive+clean. Use send_raw for raw bytes.
        # ----------------------------------------------------------------
        malformed_cases = 0

        # (1) ONE data byte after qnum (size 1, < 2). The fixed-size framer waits
        # for the 2nd data byte; we follow with garbage so the parser can't wedge,
        # and the server must stay alive.
        try:
            s = fresh("m1")
            s.send_raw(bytes([0x85, 0x00, 0x07]))         # code + qnum + ONE data byte
            time.sleep(0.15)
            s.send_raw(bytes([0xFF, 0xFF, 0xFF, 0xFF]))   # garbage trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#1 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after truncated 1-data-byte useRecipe: %s" % why)

        # (2) THREE data bytes after qnum (size 3, > 2). The framer reads code +
        # qnum + 2 bytes as the packet, leaving 1 stray byte to mis-frame the
        # next packet -> server should reject/kick, never crash.
        try:
            s = fresh("m2")
            s.send_raw(bytes([0x85, 0x00, 0x01, 0x00, 0x00]))  # code + qnum + 3 data bytes
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#2 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after over-long 3-data-byte useRecipe: %s" % why)

        # (3) code + qnum, ZERO data bytes. The framer blocks waiting for the 2
        # fixed data bytes; we send garbage to ensure no wedge, server stays alive.
        try:
            s = fresh("m3")
            s.send_raw(bytes([0x85, 0x00]))               # code + qnum, no data
            time.sleep(0.15)
            s.send_raw(bytes([0xAA, 0xBB]))               # complete then trailer
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#3 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bare code+qnum useRecipe: %s" % why)

        # (4) FIXED-size query 0x85 sent WITH a spurious 4-byte dynamic length (as
        # if it were a dynamic query). 0x85 is FIXED, so the would-be length bytes
        # are mis-read as data + next packet -> the parser must reject/kick and
        # the server must survive.
        try:
            s = fresh("m4")
            s.send_raw(bytes([0x85, 0x00]) + H.u32(2) + H.u16(1))  # bogus len prefix
            s.drain(timeout=0.3)
            s.close()
        except H.HandshakeError as e:
            return (False, "malformed#4 handshake failed: %s" % e)
        except OSError:
            pass
        malformed_cases += 1
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after bogus-dynamic-length useRecipe: %s" % why)

        # ----------------------------------------------------------------
        # NO STATE CORRUPTION: the victim account's persisted identity/position
        # must be byte-for-byte what it was before any of the abuse, and a
        # brand-new full session must still reach CharacterSelected. None of the
        # rejected useRecipe attempts may have crafted/added/removed anything.
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
                return (False,
                        "victim state corrupted: %s %r -> %r"
                        % (k, before.get(k), after.get(k)))

        # Leave the server usable: a fresh full session still handshakes.
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
            "valid=%d (craft-success branch unreachable: needs a learned recipe, "
            "no recipe-item granted in test maincode) semantic_invalid=%d "
            "(all kicked, no 0x7F reply: OOB-id-guard 0xFFFF/9 + not-owned 0/1/8) "
            "malformed=%d db_checked=%s victim_persisted=%s "
            "(server stayed alive+clean; no state corruption)"
            % (valid_cases, semantic_invalid_cases, malformed_cases,
               checks_db, (before.get("x"), before.get("y"),
                           before.get("mapIndex")))
        )
        return (True, detail)

    except Exception as e:  # never raise out of run()
        return (False, "unexpected exception: %r" % e)
