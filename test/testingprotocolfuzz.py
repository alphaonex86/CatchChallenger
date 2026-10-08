#!/usr/bin/env python3
"""testingprotocolfuzz.py — stateful protocol fuzzing of the REAL handlers.

test/fuzz/fuzz_protocol_parser.cpp fuzzes the PARSER in-process; the curated
_prototests walk ONE handler at a time. Logic bugs that need a STATE SEQUENCE
(repeat a packet across a kick boundary, cancel-then-finish a trade, act
before select, two players racing on the same object) are invisible to both.
This stage drives REAL sessions over TCP against the same production server
binary and mutates the SEQUENCE, never the framing: every packet is a
correctly-framed request a client could send, but the ORDER and CONTEXT are
adversarial.

Verdict = the SERVER crashing or wedging only. Handler semantics stay the
curated tests' job; a kick, a refusal or a wrong-looking reply is NOT a
finding here (this build tolerates cheaters by design). A hang is judged as:
process alive but a fresh minimal handshake probe no longer completes.

Reproducibility: every iteration is generated from (master_seed, iter) so a
hit is re-runnable with --seed/--iter; a crash also dumps a SELF-CONTAINED
reproducer JSON (exact frames + connect/disconnect points) to
<_WORK>/fuzz-reproducers/.

Modes:
  (default)   plain production binary, fast crash/hang scan.
  --valgrind  whole corpus under valgrind; prints the leak/error fingerprint
              at the end (informational: no per-iteration attribution).

Env:  CC_FUZZ_SECONDS (default 1500)  wall budget for the scan.
Usage: python3 testingprotocolfuzz.py [--seed N] [--iter K] [--valgrind]
"""

import json
import os
import random
import socket
import struct
import sys
import time

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import _protoharness as H

FUZZ_SECONDS = int(os.environ.get("CC_FUZZ_SECONDS", "1500"))
REPRO_DIR = os.environ.get("CC_FUZZ_REPRO", os.path.join(H._WORK, "fuzz-reproducers"))

# Spawn every character directly below the house2 Seller bot so the shop
# handlers (0x87/0x88/0x89/0x8a-0x8c) are REACHABLE in the corpus (source
# datapack untouched: the run-local profile is rewritten, see H.Server).
START_OVERRIDE = {"map": "house2", "x": 0, "y": 23, "cash": 1000000000}

# ---------------------------------------------------------------------------
# Action pool: (name, n_players) -> (kind, code, payload, dynamic). `kind` is
# "q" (query, gets a queryNumber) or "m" (message). Payloads are ALWAYS
# well-framed (exact FIXED sizes or correct dynamic length prefixes) — the
# fuzzing dimension here is the sequence, not the parser.
# ---------------------------------------------------------------------------
SHOP_IDS = (1, 5, 9, 13, 14, 15, 1234, 0xFFFF)
PRICES = (0, 1, 45, 200, 510, 999999, 0xFFFFFFFF)
QTYS = (1, 2, 100, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF)


def _rng_bytes(rng, n):
    return bytes(rng.randrange(256) for _ in range(n))


def act_move(rng):
    return ("m", 0x02, bytes([rng.choice([0, 1, 2])]) + bytes([rng.randint(1, 8)]), False)

def act_chat(rng):
    t = _rng_bytes(rng, rng.randrange(0, 21))
    return ("m", 0x03, bytes([rng.randint(1, 3), len(t)]) + t, True)

def act_shoplevel(rng):
    return ("q", 0x87, b"", False)

def act_buy(rng):
    return ("q", 0x88, H.u16(rng.choice(SHOP_IDS)) + H.u32(rng.choice(QTYS))
            + H.u32(rng.choice(PRICES)), False)

def act_sell(rng):
    return ("q", 0x89, H.u16(rng.choice(SHOP_IDS)) + H.u32(rng.choice(QTYS))
            + H.u32(rng.choice(PRICES)), False)

def act_factory_get(rng):
    return ("q", 0x8A, H.u16(rng.choice(SHOP_IDS)), False)

def act_factory_buy(rng):
    return ("q", 0x8B, H.u16(rng.choice(SHOP_IDS)) + H.u32(rng.choice(QTYS))
            + H.u32(rng.randint(0, 3)) + H.u16(rng.choice(PRICES)), False)

def act_useobject(rng):
    return ("q", 0x86, H.u16(rng.choice(SHOP_IDS)) + H.u8(rng.randint(0, 6)), False)

def act_destroy(rng):
    return ("m", 0x13, H.u16(rng.choice(SHOP_IDS)) + H.u32(rng.choice(QTYS)), False)

def act_take_object(rng):
    return ("m", 0x18, b"", False)

def act_trade_add(rng):
    body = H.u16(rng.choice(SHOP_IDS)) + H.u32(rng.choice((1, 2, 5))) + H.u8(0)
    return ("m", 0x14, body, True)

def act_trade_finish(rng):
    return ("m", 0x15, b"", False)

def act_trade_cancel(rng):
    return ("m", 0x16, b"", False)

def act_try_escape(rng):
    return ("m", 0x07, b"", False)

def act_heal(rng):
    return ("m", 0x0B, bytes([rng.randint(0, 2)]), False)

def act_change_monster(rng):
    return ("m", 0x0E, bytes([rng.randint(0, 5)]), False)

def act_quest_start(rng):
    return ("m", 0x1B, H.u16(rng.randint(0, 2)), False)

def act_quest_finish(rng):
    return ("m", 0x1C, H.u16(rng.randint(0, 2)), False)

def act_quest_cancel(rng):
    return ("m", 0x1D, H.u16(rng.randint(0, 2)), False)

def act_quest_next(rng):
    return ("m", 0x1E, H.u16(rng.randint(0, 2)), False)

def act_plant_seed(rng):
    return ("m", 0x19, bytes([rng.randint(0, 1)]), False)

def act_collect_plant(rng):
    return ("m", 0x1A, b"", False)

# two-player actions (need the peer; the fuzzer supplies its pseudo)
def act_clan_invite(rng):
    return ("m", 0x04, bytes([rng.randint(0, 4)]), False)   # invite/cancel flow


SOLO_ACTIONS = [act_move, act_chat, act_shoplevel, act_buy, act_sell,
                act_factory_get, act_factory_buy, act_useobject, act_destroy,
                act_take_object, act_trade_add, act_trade_finish,
                act_trade_cancel, act_try_escape, act_heal,
                act_change_monster, act_quest_start, act_quest_finish,
                act_quest_cancel, act_quest_next, act_plant_seed,
                act_collect_plant]
DUAL_ACTIONS = [act_clan_invite]


class Player:
    """A fuzzed session with on-demand (re)connection. The server is allowed
    to kick us at any point (that is legitimate anti-cheat behaviour); a dead
    socket is therefore NOT a finding, we simply reconnect for later steps."""

    def __init__(self, server, tag):
        self.server = server
        self.tag = tag
        self.sess = None
        self.dead = True
        self.n_connect = 0

    def ensure(self):
        if self.sess is not None and not self.dead:
            return True
        self.n_connect += 1
        try:
            self.sess = H.Session(self.server,
                                  login=("%sf%x%d" % (self.tag, int(time.time()) & 0xFFFF,
                                                      self.n_connect)).encode(),
                                  passh=b"fp_" + self.tag.encode() + str(self.n_connect).encode(),
                                  pseudo=("F%s%d" % (self.tag[:5], self.n_connect)))
            self.dead = False
            return True
        except (H.HandshakeError, OSError):
            self.sess = None
            self.dead = True
            return False

    def send(self, kind, code, payload, dynamic):
        """Send one frame; returns False if the connection died mid-send
        (expected sometimes; never itself a finding)."""
        if not self.ensure():
            return False
        try:
            if kind == "q":
                self.sess.q(code, payload, dynamic=dynamic)
            else:
                self.sess.m(code, payload, dynamic=dynamic)
            return True
        except OSError:
            self.dead = True
            return False

    def drop(self):        # abrupt disconnect (no clean close): mid-sequence drop
        if self.sess is not None:
            self.sess._closed = True
            try:
                self.sess.sock.close()
            except OSError:
                pass
        self.sess = None
        self.dead = True

    def close(self):
        if self.sess is not None:
            try:
                self.sess.close()
            except OSError:
                pass
        self.sess = None
        self.dead = True


def _iter_seed(master_seed, it):
    # random.Random needs a scalar seed; mix (master, iter) deterministically.
    return (master_seed * 1000003 + it) & 0xFFFFFFFFFFFFFFFF


def mutate_payload(rng, payload):
    """Flip ONE byte inside the payload, keeping its exact length: framing
    stays valid, semantics get wrong on purpose."""
    if not payload:
        return payload
    i = rng.randrange(len(payload))
    b = bytearray(payload)
    b[i] ^= 1 << rng.randrange(8)
    return bytes(b)


def gen_iteration(rng):
    """Return a list of steps: dicts
       {player:'A'|'B', kind, code, payload, dynamic, pre_select, drop_after}
       fully materialized (self-contained for the reproducer)."""
    steps = []
    n = rng.randint(2, 8)
    pre_select = rng.random() < 0.15
    dual = rng.random() < 0.35
    for i in range(n):
        pool = SOLO_ACTIONS + (DUAL_ACTIONS if dual else [])
        name_fn = rng.choice(pool)
        kind, code, payload, dynamic = name_fn(rng)
        if rng.random() < 0.25:
            payload = mutate_payload(rng, payload)
        player = "A"
        if dual and rng.random() < 0.4:
            player = "B"
        step = {"player": player, "kind": kind, "code": code,
                "payload": payload.hex(), "dynamic": dynamic,
                "pre_select": pre_select if i == 0 else False,
                "drop_after": False}
        steps.append(step)
        # structural mutation: repeat the last request immediately
        if rng.random() < 0.2:
            steps.append({"player": player, "kind": kind, "code": code,
                          "payload": payload.hex(), "dynamic": dynamic,
                          "pre_select": False, "drop_after": False})
        # structural mutation: cancel then REPLAY the cancelled action
        if code in (0x14, 0x1B) and rng.random() < 0.4:
            steps.append({"player": player, "kind": "m", "code":
                          0x16 if code == 0x14 else 0x1D,
                          "payload": "", "dynamic": False,
                          "pre_select": False, "drop_after": False})
            steps.append({"player": player, "kind": kind, "code": code,
                          "payload": payload.hex(), "dynamic": dynamic,
                          "pre_select": False, "drop_after": False})
    if rng.random() < 0.15 and steps:
        steps[-1]["drop_after"] = True      # vanish mid-conversation
    return steps


def run_iteration(server, steps):
    """Execute one generated sequence. pre_select steps go out on a raw
    connection that never selected a character (the illegal-context angle);
    the rest run as logged-in players."""
    a = Player(server, "fa")
    b = Player(server, "fb")
    raw = None
    try:
        for st in steps:
            payload = bytes.fromhex(st["payload"])
            if st["pre_select"] and raw is None:
                # A0+A8 only: an authenticated-ish link WITHOUT selectCharacter;
                # then in-game requests in the wrong state.
                raw = _raw_conn_unselected(server)
                if raw is not None:
                    _raw_send(raw, st)
                    continue
            if st["pre_select"]:
                continue
            p = a if st["player"] == "A" else b
            p.send(st["kind"], st["code"], payload, st["dynamic"])
            if st["drop_after"]:
                p.drop()
        # give the server a beat to react (kick / process / crash)
        time.sleep(0.15)
    finally:
        for p in (a, b):
            p.close()
        if raw is not None:
            try:
                raw.sock.close()
            except OSError:
                pass


def _raw_conn_unselected(server):
    """Connect + A0 + A8 (create/login) but NO selectCharacter; returns the
    socket (caller closes). In-game packets sent here hit handlers in the
    pre-selection state — a class of illegal context the curated tests
    barely touch."""
    try:
        sk = socket.create_connection(("127.0.0.1", server.port), timeout=3)
        sk.settimeout(1.0)
        login = ("fu%x" % (int(time.time() * 1000) & 0xFFFFFFFF)).encode()
        loginHash, passHash = H._creds(login, b"fu_pass")
        qn = H._RawConn(sk)
        pa = qn.query(0xA0, H.PROTOCOL_HEADER_LOGIN, dynamic=False, timeout=3.0)
        if pa is None or len(pa) < 17:
            sk.close()
            return None
        token = pa[1:17]
        pa = qn.query(0xA8, loginHash + H.blake3(passHash + token),
                      dynamic=False, timeout=3.0)
        if pa is not None and pa[0] == 0x07:      # account does not exist yet
            qn.query(0xA9, H.blake3(loginHash) + passHash, dynamic=False, timeout=3.0)
            qn.query(0xA8, loginHash + H.blake3(passHash + token),
                     dynamic=False, timeout=3.0)
        return qn
    except OSError:
        try:
            sk.close()
        except Exception:
            pass
        return None


def _raw_send(rc, st):
    """Send one recorded frame on the unselected _RawConn (framing by kind)."""
    payload = bytes.fromhex(st["payload"])
    try:
        if st["kind"] == "q":
            rc.qn = (rc.qn + 1) % 16
            frame = bytes([st["code"], rc.qn])
            if st["dynamic"]:
                frame += struct.pack("<I", len(payload))
            rc.sock.sendall(frame + payload)
        else:
            frame = bytes([st["code"]])
            if st["dynamic"]:
                frame += struct.pack("<I", len(payload))
            rc.sock.sendall(frame + payload)
    except OSError:
        pass


def probe_alive(server):
    """Crash OR hang check. alive()/crash_report() catch a dead process; the
    A0 roundtrip catches a live-but-wedged process (accept loop or parser
    stuck, no reply to the most basic query)."""
    if not server.alive():
        return False, (server.crash_report() or "not accepting TCP")[:400]
    if server.crash_report() is not None:
        return False, server.crash_report()[:400]
    try:
        sk = socket.create_connection(("127.0.0.1", server.port), timeout=2)
        sk.settimeout(3.0)
        rc = H._RawConn(sk)
        pa = rc.query(0xA0, H.PROTOCOL_HEADER_LOGIN, dynamic=False, timeout=3.0)
        sk.close()
        if pa is None or len(pa) < 17:
            return False, "server HUNG: A0 protocol probe unanswered"
    except OSError as e:
        return False, "server HUNG/unreachable on probe: %r" % e
    return True, ""


def save_reproducer(master_seed, it, steps, why):
    try:
        os.makedirs(REPRO_DIR, exist_ok=True)
    except OSError:
        return
    path = os.path.join(REPRO_DIR, "repro_s%d_i%d.json" % (master_seed, it))
    try:
        with open(path, "w") as f:
            json.dump({"master_seed": master_seed, "iter": it,
                       "start_override": START_OVERRIDE,
                       "why": why, "steps": steps}, f, indent=1)
        sys.stderr.write("[fuzz] REPRODUCER written: %s\n" % path)
    except OSError:
        pass


def replay(master_seed, it):
    """Re-run one recorded iteration from the master seed (deterministic)."""
    binary = H.build_server(valgrind=False)
    server = H.Server(binary, os.path.join(H._WORK, "run-fuzz-replay"),
                      start_override=START_OVERRIDE)
    try:
        rng = random.Random(_iter_seed(master_seed, it))
        steps = gen_iteration(rng)
        sys.stderr.write("[fuzz] replaying seed=%s iter=%s (%d steps)\n"
                         % (master_seed, it, len(steps)))
        run_iteration(server, steps)
        ok, why = probe_alive(server)
        sys.stderr.write("[fuzz] replay result: %s %s\n"
                         % ("CLEAN" if ok else "REPRODUCED", why))
        if not ok:
            save_reproducer(master_seed, it, steps, why)
        return 0 if ok else 1
    finally:
        server.stop()


def main():
    args = sys.argv[1:]
    valgrind = "--valgrind" in args
    master_seed = None
    only_iter = None
    for a in args:
        if a.startswith("--seed"):
            master_seed = int(a.split("=", 1)[1]) if "=" in a else master_seed
        if a.startswith("--iter"):
            only_iter = int(a.split("=", 1)[1])
    if master_seed is None:
        master_seed = int(time.time()) & 0x7FFFFFFF
    if only_iter is not None:
        return replay(master_seed, only_iter)

    binary = H.build_server(valgrind=valgrind)
    server = H.Server(binary, os.path.join(H._WORK, "run-fuzz-main"),
                      start_override=START_OVERRIDE, valgrind=valgrind)
    sys.stderr.write("[fuzz] server up on port %d (%s mode), budget %ds\n"
                     % (server.port, "valgrind" if valgrind else "plain",
                        FUZZ_SECONDS))
    t_end = time.time() + FUZZ_SECONDS
    it = 0
    crashes = 0
    try:
        while time.time() < t_end:
            it += 1
            rng = random.Random(_iter_seed(master_seed, it))
            steps = gen_iteration(rng)
            run_iteration(server, steps)
            ok, why = probe_alive(server)
            if not ok:
                crashes += 1
                sys.stderr.write("[fuzz] ITER %d KILLED/WEDGED THE SERVER: %s\n"
                                 % (it, why))
                save_reproducer(master_seed, it, steps, why)
                server.stop()
                server = H.Server(binary, os.path.join(H._WORK, "run-fuzz-main"),
                                  start_override=START_OVERRIDE, valgrind=valgrind)
    except KeyboardInterrupt:
        sys.stderr.write("[fuzz] interrupted at iter %d\n" % it)
    finally:
        server.stop()
    sys.stderr.write("[fuzz] master_seed=%d iterations=%d crashes=%d\n"
                     % (master_seed, it, crashes))
    if valgrind:
        fp = H.parse_valgrind_fingerprint(
            os.path.join(H._WORK, "run-fuzz-main", "valgrind.log"))
        sys.stderr.write("[fuzz] valgrind fingerprint: %r\n" % fp)
    if crashes:
        print("FUZZ FINDINGS: %d crash/hang(s); reproducers under %s "
              "(replay: python3 testingprotocolfuzz.py --seed=%d --iter=K)"
              % (crashes, REPRO_DIR, master_seed))
        return 1
    print("protocol fuzz clean: %d stateful sequences, seed=%d"
          % (it, master_seed))
    return 0


if __name__ == "__main__":
    sys.exit(main())
