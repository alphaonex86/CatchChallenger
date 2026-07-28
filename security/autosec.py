#!/usr/bin/env python3
"""autosec.py - DETERMINISTIC security vectors that feed the IA audit + exploit
pipeline. Four subcommands. All reuse existing infra (test/_protoharness.py,
test/fuzz/, security/server.py + agentic.py). ZERO production C/C++ change: the
existing test-side seams already expose every hook we need.

  fuzz      Build & run test/fuzz/fuzz_protocol_parser.cpp under libFuzzer +
            AddressSanitizer + UBSan (the harness is pre-wired; this is the
            runner that was missing). Collects crash-* inputs, re-runs each
            under gdb for a backtrace, and emits each crash as a FINDINGS
            candidate + a crash seed for `crash2ia`. (wire-grammar fuzzer)

  pexer     For every Client state x every packet code in the wire grammar,
            send the packet in the WRONG state and observe: a server crash
            (memory bug) or a substantive non-kick reply (auth-bypass
            candidate). Pre-auth states are pexed in depth. (protocol-state
            pexer)

  statediff Snapshot the persisted character HPS blob before vs after a
            scripted trade/drop/craft/shop/catch (driven via _protoharness
            Session, with reconnect_volatile_offsets masking moving stamps).
            Flags a CRASH mid-action or a STATE LEAK (a non-empty masked diff
            after a REJECTED/no-op action = corruption). Rule-conformance
            (cash conservation, overflow caps) is left to the IA exploit phase,
            which proves game-logic exploits via GDB case (c). (integrity
            state-diff harness)

  crash2ia  Collect crash artifacts (from `fuzz`, from security/server.py's
            exploit phase, or a dir of crash-* files), map each crash's top
            repo frame to its function via the codetree call index, and RE-SEED
            the per-function IA audit so the model reasons backward from a
            PROVEN crash to the root-cause bug + a minimal exploit. Mirrors the
            adversarial exploit pass. (crash -> IA feedback)

Output: findings printed to stdout AND written to OUTPUT_ROOT/autosec/
findings.txt in server.py's FINDINGS block format (so `server.py exploit` can
prove them). Crash artifacts, pexer logs, and state dumps land under
OUTPUT_ROOT/autosec/<subcommand>/.

Usage:
  python3 autosec.py fuzz                         # ~10min libFuzzer run
  python3 autosec.py pexer                        # wrong-state sweep
  python3 autosec.py statediff                    # integrity byte-diff
  python3 autosec.py crash2ia                     # re-seed IA from crashes
  python3 autosec.py crash2ia --src=PATH/TO/crash-*
"""

import glob
import os
import re
import shutil
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.realpath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
# Put test/ on sys.path so `import _protoharness` resolves (it lives in test/).
_TEST_DIR = os.path.normpath(os.path.join(HERE, "..", "test"))
if _TEST_DIR not in sys.path:
    sys.path.insert(0, _TEST_DIR)

import server as S  # noqa: E402  (REPO_ROOT, OUTPUT_ROOT, FINDINGS, run_exploit)
import common  # noqa: E402  (chat / chat_with for the IA turns)
import agentic  # noqa: E402  (audit_function, resolve_llms - the per-function engine)
import codetree  # noqa: E402  (call index to map a crash frame -> function)

REPO_ROOT = S.REPO_ROOT
OUTPUT_ROOT = os.path.join(S.OUTPUT_ROOT, "autosec")
FINDINGS = os.path.join(OUTPUT_ROOT, "findings.txt")

# test/fuzz harness (the pre-wired libFuzzer + ASan + UBSan target).
FUZZ_SRC = os.path.join(REPO_ROOT, "test", "fuzz")
FUZZ_BUILD = os.path.join(OUTPUT_ROOT, "fuzz-build")
FUZZ_BIN = os.path.join(FUZZ_BUILD, "fuzz_protocol_parser")
FUZZ_OUT = os.path.join(OUTPUT_ROOT, "fuzz")          # crashes + corpus + logs
FUZZ_DICT = os.path.join(FUZZ_OUT, "protocol.dict")

PEXER_OUT = os.path.join(OUTPUT_ROOT, "pexer")
DIFF_OUT = os.path.join(OUTPUT_ROOT, "statediff")
CRASH2IA_OUT = os.path.join(OUTPUT_ROOT, "crash2ia")

# Wire-grammar source: the packetFixedSize[] table populator. Parsing it gives
# the exact set of packet codes the live parser dispatches, with each code's
# fixed payload size (or 0xFE = dynamic, 4-byte LE length prefix).
PKT_TABLE_SRC = os.path.join(REPO_ROOT, "general", "base",
                             "ProtocolParsingGeneral.cpp")

# Budgets (seconds) - overridable.
try:
    FUZZ_BUDGET = int(os.environ.get("CC_AUTOFUZZ_BUDGET", "600") or "600")
except ValueError:
    FUZZ_BUDGET = 600
try:
    PEXER_PER_CODE_TIMEOUT = float(os.environ.get("CC_PEXER_TIMEOUT", "1.0")
                                   or "1.0")
except ValueError:
    PEXER_PER_CODE_TIMEOUT = 1.0

# Read-only gdb command allowlist (mirror server.py.GDB_ALLOWED so an attach
# can never mutate the inferior). First-token gate.
_GDB_OK = {"print", "p", "bt", "backtrace", "where", "frame", "info", "i",
           "x", "list", "l", "ptype", "whatis", "up", "down", "thread"}
_GDB_ASSIGN = re.compile(r"(^|[^=!<>])=(?!=)")


# ---------------------------------------------------------------------------
# Shared helpers
# ---------------------------------------------------------------------------
def _ts():
    return time.strftime("%H:%M:%S")


def _ensure(dirpath):
    os.makedirs(dirpath, exist_ok=True)
    return dirpath


def _run(cmd, timeout=120, cwd=None, env=None):
    """Capture-stdout/stderr run; returns (rc, out, err). Never raises."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout,
                           cwd=cwd, env=env)
        return (r.returncode, r.stdout, r.stderr)
    except (OSError, subprocess.SubprocessError) as exc:
        return (-1, "", str(exc))


def _packet_table():
    """Parse `packetFixedSize[0xNN]=EXPR;` assignments -> {code:int -> size:int}.
    0xFE means dynamic (4-byte LE length). RHS may be a bare int, an arithmetic
    expression (2+4+4, 2*2+2*4), or a macro (treated as unknown -> 0xFE). Covers
    message codes (<0x80) and query codes (>=0x80); the reply half is ignored
    (a pexer sends client->server codes only)."""
    try:
        text = open(PKT_TABLE_SRC, "r", errors="replace").read()
    except OSError:
        return {}
    out = {}
    arith = re.compile(r"^[0-9+\-*/() xXA-Fa-f]+$")
    for m in re.finditer(
            r"packetFixedSize\[\s*(0x[0-9A-Fa-f]+|\d+)\s*\]\s*=\s*([^;]+);",
            text):
        code = int(m.group(1), 0)
        rhs = m.group(2).strip()
        if not (0 <= code < 0x100) or code in out:
            continue
        size = None
        try:
            size = int(rhs, 0)                       # bare int (0x02, 254, 0xFE)
        except ValueError:
            if arith.match(rhs):                     # 2+4+4, 2*2+2*4 - safe arithmetic
                try:
                    size = int(eval(rhs, {"__builtins__": {}}, {}))
                except Exception:
                    size = None
        if size is None:
            continue                                 # macro / unknown -> skip (pexer covers via raw)
        if 0 <= size <= 0xFF:
            out[code] = size
    return out


def _write_findings(by_file):
    """Write `by_file` {rel: [finding_text,...]} in the FINDINGS block format
    that server.py.parse_findings() expects, to FINDINGS + stdout."""
    _ensure(OUTPUT_ROOT)
    if not by_file:
        sys.stderr.write("[%s] no findings - nothing written\n" % _ts())
        return
    with open(FINDINGS, "w") as fh:
        for rel in by_file:
            for sink in (sys.stdout, fh):
                print("=" * 70, file=sink)
                print("FINDINGS: %s" % rel, file=sink)
                print("=" * 70, file=sink)
                print("\n\n".join(by_file[rel]), file=sink)
                print(file=sink)
    sys.stderr.write("[%s] %d file(s) with findings -> %s\n"
                     % (_ts(), len(by_file), FINDINGS))


def _gdb_cmd_ok(line):
    """Allowlist gate for a single gdb command (read-only). Mirror server.py."""
    s = line.strip()
    if not s or " " not in s:
        return s in _GDB_OK
    tok = s.split(None, 1)[0]
    if tok not in _GDB_OK:
        return False
    if "(" in s and tok not in ("ptype", "whatis", "info", "i"):
        return False                       # no function calls
    if _GDB_ASSIGN.search(s.split(None, 1)[1] if " " in s else ""):
        return False                       # no assignment
    return True


def _gdb_batch(args, cmds, timeout=30):
    """Run gdb in batch mode with `args` (program + args OR -p PID) and a list
    of read-only commands (each gated by _gdb_cmd_ok). Returns (ok, stdout)."""
    safe = [c for c in cmds if _gdb_cmd_ok(c)]
    dropped = len(cmds) - len(safe)
    if dropped:
        sys.stderr.write("    [gdb] dropped %d non-read-only command(s)\n" % dropped)
    cmd = ["gdb", "-q", "--batch"]
    for c in safe:
        cmd += ["-ex", c]
    cmd += ["--args"] if (args and args[0] != "-p") else []
    cmd += args
    rc, out, err = _run(cmd, timeout=timeout)
    return (rc == 0, out + ("\n" + err if err else ""))


# ---------------------------------------------------------------------------
# 1. fuzz - libFuzzer runner for test/fuzz/fuzz_protocol_parser.cpp
# ---------------------------------------------------------------------------
def _have_clang_libfuzzer():
    """True if a clang++ supporting -fsanitize=fuzzer is on PATH."""
    clang = shutil.which("clang++")
    if not clang:
        return False
    rc, out, _ = _run([clang, "-x", "c++", "/dev/null", "-fsanitize=fuzzer,address",
                       "-o", "/dev/null", "-S"], timeout=30)
    return rc == 0


def _build_fuzzer():
    """cmake configure + build the fuzz target (clang++ + libFuzzer + ASan/UBSan)."""
    _ensure(FUZZ_OUT)
    clang = shutil.which("clang++") or "clang++"
    env = dict(os.environ, CC=clang.replace("++", ""), CXX=clang)
    if not os.path.isdir(FUZZ_BUILD) or not os.path.isfile(
            os.path.join(FUZZ_BUILD, "CMakeCache.txt")):
        rc, _out, err = _run(["cmake", "-S", FUZZ_SRC, "-B", FUZZ_BUILD,
                              "-DCMAKE_CXX_COMPILER=" + clang], timeout=120, env=env)
        if rc != 0:
            sys.stderr.write("[fuzz] cmake configure failed:\n%s\n" % err)
            return False
    rc, _out, err = _run(["cmake", "--build", FUZZ_BUILD, "-j"], timeout=300, env=env)
    if rc != 0:
        sys.stderr.write("[fuzz] build failed:\n%s\n" % err)
        return False
    return os.path.isfile(FUZZ_BIN)


def _write_fuzz_dict(table):
    """Write a libFuzzer dictionary of header bytes (one entry per known code)
    so the mutator seeds real packet starts, not random bytes."""
    _ensure(FUZZ_OUT)
    with open(FUZZ_DICT, "w") as fh:
        for code in sorted(table):
            fh.write("\"\\x%02x\"\n" % code)
    return FUZZ_DICT


def _collect_fuzz_crashes(corpus_dir):
    """crash-* and leak-* inputs libFuzzer wrote. Returns sorted [path,...]."""
    return sorted(glob.glob(os.path.join(corpus_dir, "crash-*")) +
                  glob.glob(os.path.join(corpus_dir, "leak-*")) +
                  glob.glob(os.path.join(corpus_dir, "oom-*")))


def _backtrace_crash(crash_input):
    """Re-run one crash input under gdb on the fuzzer binary; return the
    backtrace text (best-effort)."""
    ok, out = _gdb_batch([FUZZ_BIN, crash_input],
                         ["set pagination off", "run", "bt 30", "info registers rip"],
                         timeout=20)
    return out if ok else ("(gdb failed)\n" + out)


def run_fuzz(argv):
    """Build + run the libFuzzer harness; collect + triage crashes; emit FINDINGS."""
    budget = FUZZ_BUDGET
    corpus = os.path.join(FUZZ_OUT, "corpus")
    _ensure(corpus)
    # Options: `--budget=N` (libFuzzer wall seconds); a bare DIR seeds the corpus.
    for a in argv:
        if a.startswith("--budget="):
            try:
                budget = int(a[len("--budget="):])
            except ValueError:
                sys.stderr.write("[fuzz] bad --budget value: %s\n" % a)
                return 2
        elif os.path.isdir(a):
            for f in glob.glob(os.path.join(a, "*")):
                shutil.copy(f, corpus)

    if not _have_clang_libfuzzer():
        sys.stderr.write("[fuzz] clang++ with -fsanitize=fuzzer not found - "
                         "install clang (libFuzzer + ASan) first\n")
        return 2
    sys.stderr.write("[%s] building fuzzer...\n" % _ts())
    if not _build_fuzzer():
        return 2

    table = _packet_table()
    dict_path = _write_fuzz_dict(table) if table else None

    sys.stderr.write("[%s] running libFuzzer for %ds (dict: %d codes)...\n"
                     % (_ts(), budget, len(table)))
    cmd = [FUZZ_BIN, "-max_total_time=%d" % budget,
           "-print_final_stats=1", "-timeout=5"]
    if dict_path:
        cmd += ["-dict=" + dict_path]
    cmd += [corpus]
    # libFuzzer writes crash-* to CWD; run it inside FUZZ_OUT so they land there.
    _run(cmd, timeout=budget + 60, cwd=FUZZ_OUT)

    crashes = _collect_fuzz_crashes(FUZZ_OUT)
    sys.stderr.write("[%s] %d crash/leak/oom input(s) collected\n"
                     % (_ts(), len(crashes)))
    if not crashes:
        sys.stderr.write("[fuzz] clean - no crashes (good news, or raise the budget via "
                         "CC_AUTOFUZZ_BUDGET)\n")
        return 0

    by_file = {}
    for i, crash in enumerate(crashes, 1):
        bt = _backtrace_crash(crash)
        with open(crash, "rb") as fh:
            raw = fh.read()
        outdir = _ensure(os.path.join(FUZZ_OUT, "%02d" % i))
        shutil.copy2(crash, os.path.join(outdir, "input"))
        with open(os.path.join(outdir, "backtrace.gdb"), "w") as fh:
            fh.write(bt)
        with open(os.path.join(outdir, "input.hex"), "w") as fh:
            fh.write(raw.hex())
        repro = "cd %s && %s %s" % (FUZZ_OUT, FUZZ_BIN, os.path.basename(crash))
        with open(os.path.join(outdir, "repro.sh"), "w") as fh:
            fh.write("#!/bin/sh\n%s\n" % repro)
        rel = "general/base/ProtocolParsingInput.cpp"   # the parser entry fuzzed
        snippet = ("## fuzzer crash %d (ASan/UBSan in the wire parser)\n"
                   "Repro: `%s`\nInput bytes (%d): %s\n\nBacktrace:\n```\n%s\n```"
                   % (i, repro, len(raw), raw[:64].hex(), bt.strip()[:2000]))
        by_file.setdefault(rel, []).append(snippet)
    _write_findings(by_file)
    sys.stderr.write("[%s] crash2ia next: python3 autosec.py crash2ia --src=%s\n"
                     % (_ts(), FUZZ_OUT))
    return 0


# ---------------------------------------------------------------------------
# 2. pexer - wrong-state packet sweep over TCP
# ---------------------------------------------------------------------------
# The high-value pre-auth states. _protoharness reaches CharacterSelected for
# in-game tests; the pexer probes the SURFACE BEFORE login, where auth-bypass
# and pre-auth memory bugs live. Each "state" is a (label, handshake_fn).
def _reach_just_connected(sock):
    """State None/ProtocolGood boundary: only a TCP socket, nothing sent."""
    return None


def _reach_protocol_good(sock):
    """Send the protocol-header query 0xA0 and drain the reply -> ProtocolGood."""
    # 0xA0 is a fixed-size query: code + queryNumber + 2B payload (maincode+ver).
    # Minimal: code 0xA0, qn 0x01, then the 2-byte protocol marker the server
    # expects. The server's parseInputBeforeLogin sets ProtocolGood on success.
    sock.sendall(bytes([0xA0, 0x01, 0x01, 0x00]))
    _drain(sock, 0.5)
    return None


def _drain(sock, t=0.4):
    """Read until timeout/EOF; returns the LAST chunk received (b"" on EOF or
    timeout, so a kicked socket and a silent socket are both empty - the caller
    distinguishes via _server_alive)."""
    import socket as _s
    sock.settimeout(t)
    last = b""
    try:
        while True:
            d = sock.recv(4096)
            if not d:
                return b""    # EOF = kicked
            last = d
    except (_s.timeout, OSError):
        pass
    return last


_PEXER_STATES = (
    ("none", _reach_just_connected),
    ("protocol_good", _reach_protocol_good),
)


def _frame(code, size):
    """Build a minimally-valid frame for `code` given its declared `size` from
    the table. 0xFE = dynamic (4-byte LE length + a short payload). Returns the
    raw bytes to send. Payloads are zeroes (we want the handler to run, not pass
    semantic validation - which is exactly where wrong-state bugs hide)."""
    if size == 0xFE:
        body = b"\x00" * 4
        return bytes([code]) + struct.pack("<I", len(body)) + body
    if code >= 0x80:
        # query: code + queryNumber + fixed payload
        return bytes([code, 0x01]) + b"\x00" * size
    return bytes([code]) + b"\x00" * size


def run_pexer(argv):
    """For every (pre-auth state x packet code): connect, reach the state, fire
    the packet; flag a server crash or a substantive non-kick reply."""
    import _protoharness as H
    per_code = PEXER_PER_CODE_TIMEOUT     # `--timeout=S` overrides
    for a in argv:
        if a.startswith("--timeout="):
            try:
                per_code = float(a[len("--timeout="):])
            except ValueError:
                sys.stderr.write("[pexer] bad --timeout value: %s\n" % a)
                return 2
    binary = H.build_server(valgrind=False)
    if not binary:
        sys.stderr.write("[pexer] server build failed\n")
        return 2
    table = _packet_table()
    if not table:
        sys.stderr.write("[pexer] could not parse packet table from %s\n" % PKT_TABLE_SRC)
        return 2
    run_dir = _ensure(os.path.join(PEXER_OUT, "run"))
    server = H.Server(binary, run_dir, maincode="test", every_body_is_root=False)
    try:
        H._wait_listening(server) if hasattr(H, "_wait_listening") else _wait_alive(server)
    except Exception:
        pass
    sys.stderr.write("[%s] pexer: server on port %d, %d packet codes x %d states "
                     "(per-code %.1fs)\n"
                     % (_ts(), server.port, len(table), len(_PEXER_STATES), per_code))

    import socket
    findings = []
    stats = {"crash": 0, "reply": 0, "kick": 0, "eof": 0}
    for state_name, reacher in _PEXER_STATES:
        if not _server_alive(server):
            sys.stderr.write("[%s] server died mid-state-%s; aborting\n"
                             % (_ts(), state_name))
            break
        for code in sorted(table):
            size = table[code]
            sock = None
            try:
                sock = socket.create_connection(("127.0.0.1", server.port), timeout=3)
                reacher(sock)
                sock.sendall(_frame(code, size))
                reply = _drain(sock, per_code)
                eof = reply == b"" and not _server_alive(server)
                kicked = reply == b"" and _server_alive(server)
            except (socket.error, OSError) as exc:
                sys.stderr.write("    [%s/%#04x] sock err: %s\n" % (state_name, code, exc))
                continue
            finally:
                if sock is not None:
                    try:
                        sock.close()
                    except OSError:
                        pass
            if not _server_alive(server):
                stats["crash"] += 1
                crash = server.crash_report()
                findings.append("## PEXER crash: state=%s code=0x%02x size=%d\n"
                                "%s\n(restart server to continue)"
                                % (state_name, code, size, crash or "(no report)"))
                sys.stderr.write("    [!! %s/0x%02x CRASH] %s\n"
                                 % (state_name, code, (crash or "").strip().splitlines()[0]
                                    if crash else ""))
                # restart to pex the rest
                _restart(server)
                break
            elif reply and not kicked:
                # a substantive reply in the WRONG state is an auth-bypass candidate
                stats["reply"] += 1
                findings.append("## PEXER surprise reply: state=%s code=0x%02x size=%d\n"
                                "server replied %d bytes (head=%s) where a kick was "
                                "expected - candidate auth/state bypass.\n"
                                % (state_name, code, size, len(reply), reply[:16].hex()))
            elif kicked:
                stats["kick"] += 1
            else:
                stats["eof"] += 1
    sys.stderr.write("[%s] pexer done: %s\n" % (_ts(), stats))
    by_file = {}
    for f in findings:
        # crashes here are in the wire parser / pre-auth dispatch
        rel = "general/base/ProtocolParsingInput.cpp"
        if "bypass" in f:
            rel = "server/base/ClientNetworkRead.cpp"
        by_file.setdefault(rel, []).append(f)
    _write_findings(by_file)
    try:
        server.proc.terminate()
    except Exception:
        pass
    return 0


def _server_alive(server):
    """True if the supervised server process is still running."""
    return server.proc is not None and server.proc.poll() is None and server.alive()


def _wait_alive(server, timeout=15):
    """Poll until the server reports alive() or timeout."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if server.alive():
            return True
        if server.proc.poll() is not None:
            return False
        time.sleep(0.3)
    return server.alive()


def _restart(server):
    """Best-effort restart of a crashed _protoharness.Server (re-runs the binary)."""
    try:
        server.proc.terminate()
    except Exception:
        pass
    time.sleep(0.5)
    # _protoharness.Server has no public restart; re-exec via Popen like _start.
    import _protoharness as H
    local_bin = os.path.join(server.run_dir, "catchchallenger-server-cli")
    server._logf = open(server.log_path, "wb")
    server.proc = subprocess.Popen([local_bin], cwd=server.run_dir,
                                   stdout=server._logf, stderr=subprocess.STDOUT,
                                   start_new_session=True)
    _wait_alive(server, timeout=10)


# ---------------------------------------------------------------------------
# 3. statediff - integrity byte-diff around scripted actions
# ---------------------------------------------------------------------------
# The actions: each is (label, setup_fn) where setup_fn(sess_a, sess_b_or_None,
# server) drives ONE complete action. A None sess_b means single-client.
# We snapshot each character's HPS blob before (after a disconnect) and after
# (after another disconnect following the action), mask the volatile offsets
# _protoharness.reconnect_volatile_offsets computes, and report a non-empty
# masked diff as a STATE LEAK when the action was a no-op / rejected.
def _char_blob(run_dir, pseudo):
    """Read the persisted HPS character blob for `pseudo` (hex of the raw
    pseudo bytes), or None if absent. Server writes it only at disconnect."""
    pseudo_hex = pseudo.encode().hex() if isinstance(pseudo, str) else pseudo.hex()
    for sub in ("common", "server"):
        p = os.path.join(run_dir, "database", sub, "characters", pseudo_hex)
        if os.path.isfile(p):
            try:
                return open(p, "rb").read()
            except OSError:
                return None
    return None


def _masked_diff(before, after, volatile_offsets):
    """Byte positions where before != after, EXCLUDING the volatile set."""
    vol = set(volatile_offsets or ())
    diffs = []
    n = min(len(before), len(after))
    for i in range(n):
        if i in vol:
            continue
        if before[i] != after[i]:
            diffs.append(i)
    # length change (beyond volatile) is itself a diff
    if len(before) != len(after):
        diffs.append(-1)   # sentinel: size changed
    return diffs


def _snapshot(server, login, passh, pseudo):
    """Disconnect, reconnect to flush, disconnect again; return the character
    blob bytes + the volatile offset list (to mask in the caller). Returns
    (blob_bytes, volatile_offsets) or (None, [])."""
    import _protoharness as H
    try:
        vol = H.reconnect_volatile_offsets(server, login, passh)
    except Exception:
        vol = []
    blob = _char_blob(server.run_dir, pseudo)
    return (blob, vol)


def _action_drop(sess, _b, server):
    """Single-client: destroy 1 of item 1 (the starter item grant_item seeded)."""
    import _protoharness as H
    H.grant_item(sess, 1, 5)         # ensure we own 5 of item 1
    sess.m(0x13, struct.pack("<HI", 1, 1))   # destroyObject itemId=1 qty=1
    sess.drain(0.6)


def _action_trade_cancel(sess_a, sess_b, server):
    """Two-client: open a trade A<->B, then CANCEL it. After cancel, NO state
    should move - any byte diff is a dupe/leak bug."""
    import _protoharness as H
    # B must be on the same map (start.xml city); the default spawn is.
    sess_a.m(0x03, b"\x01" + sess_b.pseudo.encode())  # chat /trade <B>
    sess_b.drain(0.5)                                 # B gets the request
    sess_b.m(0x03, b"\x01\x01")                       # /accept (returnCode 0x01)
    sess_a.drain(0.5)
    sess_a.m(0x16)                                    # tradeCanceled
    sess_a.drain(0.5)
    sess_b.drain(0.5)


_ACTIONS = (
    ("drop_object", _action_drop, False),
    ("trade_cancel", _action_trade_cancel, True),
)


def run_statediff(argv):
    """For each scripted action: snapshot before, run the action, snapshot after;
    flag a crash mid-action or a non-empty masked diff (state leak)."""
    import _protoharness as H
    binary = H.build_server(valgrind=False)
    if not binary:
        sys.stderr.write("[statediff] server build failed\n")
        return 2
    run_dir = _ensure(os.path.join(DIFF_OUT, "run"))
    server = H.Server(binary, run_dir, maincode="test", every_body_is_root=True)
    _wait_alive(server, timeout=15)
    sys.stderr.write("[%s] statediff: server port %d, %d action(s)\n"
                     % (_ts(), server.port, len(_ACTIONS)))

    findings = []
    for label, fn, needs_two in _ACTIONS:
        if not _server_alive(server):
            sys.stderr.write("[%s] server died before %s; restarting\n" % (_ts(), label))
            _restart(server)
        # Fresh credentials per action so character blobs are isolated.
        login_a = ("a_%s" % label).encode()
        pass_a = b"pa_" + login_a
        pseudo_a = ("A_%s" % label)
        login_b = ("b_%s" % label).encode()
        pseudo_b = ("B_%s" % label)
        try:
            sess_a = H.Session(server, login_a, pass_a, pseudo_a)
            sess_b = H.Session(server, login_b, b"pb_" + login_b, pseudo_b) if needs_two else None
        except Exception as exc:
            sys.stderr.write("    [%s] handshake failed: %s\n" % (label, exc))
            continue
        # Snapshot BEFORE (disconnect both, reconnect A to flush, snapshot blob).
        try:
            sess_a.sock.close()
            if sess_b:
                sess_b.sock.close()
        except Exception:
            pass
        time.sleep(0.4)   # let the server flush character files on disconnect
        before_a, vol_a = _snapshot(server, login_a, pass_a, pseudo_a)
        before_b = _char_blob(server.run_dir, pseudo_b) if needs_two else None

        # Reconnect, run the action.
        try:
            sess_a = H.Session(server, login_a, pass_a, pseudo_a)
            sess_b = H.Session(server, login_b, b"pb_" + login_b, pseudo_b) if needs_two else None
            fn(sess_a, sess_b, server)
        except Exception as exc:
            sys.stderr.write("    [%s] action raised: %s\n" % (label, exc))
            continue
        if not _server_alive(server):
            crash = server.crash_report()
            findings.append("## STATEDIFF crash mid-action: %s\n%s"
                            % (label, crash or "(no report)"))
            sys.stderr.write("    [!! %s CRASH mid-action]\n" % label)
            _restart(server)
            continue
        # Snapshot AFTER.
        try:
            sess_a.sock.close()
            if sess_b:
                sess_b.sock.close()
        except Exception:
            pass
        time.sleep(0.4)
        after_a, _ = _snapshot(server, login_a, pass_a, pseudo_a)
        after_b = _char_blob(server.run_dir, pseudo_b) if needs_two else None

        leaks = []
        if before_a and after_a:
            d = _masked_diff(before_a, after_a, vol_a)
            if d:
                leaks.append("A pseudo=%s: %d byte(s) moved (offsets %s)"
                             % (pseudo_a, len(d), d[:8]))
        if needs_two and before_b and after_b:
            d = _masked_diff(before_b, after_b, vol_a)   # same vol set
            if d:
                leaks.append("B pseudo=%s: %d byte(s) moved (offsets %s)"
                             % (pseudo_b, len(d), d[:8]))
        if leaks:
            findings.append(
                "## STATEDIFF state leak on `%s`: character bytes changed where the "
                "rules require NO mutation (action was a no-op or rejected).\n"
                "%s\n\nThis is a candidate dupe / illicit-get / corruption: prove via "
                "GDB field inspection (case c) in the exploit phase.\n"
                % (label, "\n".join("  " + x for x in leaks)))
            sys.stderr.write("    [!! %s STATE LEAK] %s\n" % (label, "; ".join(leaks)))
        else:
            sys.stderr.write("    [%s] clean (no leak)\n" % label)

    try:
        server.proc.terminate()
    except Exception:
        pass
    by_file = {}
    for f in findings:
        # integrity findings point at the trade/object/crafting handlers
        rel = "server/base/ClientEvents/LocalClientHandlerTrade.cpp"
        if "drop" in f or "destroy" in f.lower():
            rel = "server/base/ClientEvents/LocalClientHandlerObject.cpp"
        by_file.setdefault(rel, []).append(f)
    _write_findings(by_file)
    return 0


# ---------------------------------------------------------------------------
# 4. crash2ia - re-seed the per-function IA audit from proven crashes
# ---------------------------------------------------------------------------
CRASH_FEEDBACK_SYSTEM = (
    "You are a security auditor of a C/C++ TCP server. You are handed a PROVEN "
    "CRASH: an input that demonstrably crashes the server (ASan / UBSan / gdb "
    "backtrace attached). Your job is to reason BACKWARD from the crash to the "
    "ROOT-CAUSE bug, then describe the minimal fix.\n"
    "The crash is REAL - do not dispute it. Trace the attacker bytes from the "
    "shown function's entry to the faulting instruction. Confirm reachability "
    "from a remote TCP socket (the parser/dispatcher path). Then output ONE "
    "line per root-cause finding:\n"
    "  SEVERITY(high|critical) | function:line | the bug + the fix + how remote "
    "input triggers it.\n"
    "If the crash is genuinely in a vendored lib (NOT our code), say so. "
    "Otherwise be specific: name the unchecked length/index/size/overflow and "
    "the guard that would prevent it.")


def _collect_crashes(srcs):
    """Gather (input_path, backtrace_text) pairs from fuzz output, exploit
    outdirs, or a glob of crash-* files."""
    out = []
    for src in srcs:
        if os.path.isdir(src):
            # autosec fuzz layout: numbered dirs with input + backtrace.gdb
            for d in sorted(glob.glob(os.path.join(src, "[0-9]*"))):
                inp = os.path.join(d, "input")
                bt = os.path.join(d, "backtrace.gdb")
                if os.path.isfile(inp):
                    bt_text = open(bt, "r", errors="replace").read() if os.path.isfile(bt) else ""
                    out.append((inp, bt_text))
            # flat crash-* / leak-* / oom-* (libFuzzer CWD, or a user glob dir)
            for f in sorted(glob.glob(os.path.join(src, "crash-*")) +
                            glob.glob(os.path.join(src, "leak-*")) +
                            glob.glob(os.path.join(src, "oom-*"))):
                out.append((f, ""))
        elif os.path.isfile(src):
            out.append((src, ""))
    return out


_CRASH_FRAME_RE = re.compile(
    r"#\d+\s+0x[0-9a-f]+\s+in\s+([^ ]+)\s+\(\)\s+at\s+"
    r"([^:]+):(\d+)")


def _crash_to_function(bt, idx):
    """Topmost repo frame in the backtrace -> (qual_name, file, line). Walks the
    frames until one resolves in the codetree index (skips libc / sanitiser
    frames). None if no repo frame resolves."""
    if not bt:
        return None
    for m in _CRASH_FRAME_RE.finditer(bt):
        fn, path, line = m.group(1), m.group(2), int(m.group(3))
        # only frames inside our repo
        if REPO_ROOT in os.path.abspath(path) or any(
                seg in path for seg in ("general/", "server/", "client/")):
            # try exact qual_name match, else match by file+line via the index
            for qn, fi in idx.by_name.items():
                if fi.file and os.path.realpath(fi.file) == os.path.realpath(path) \
                        and fi.line <= line <= (fi.line + 400):
                    return (qn, path, line)
    return None


def run_crash2ia(argv):
    """For each crash: map to its function, re-seed the per-function IA audit
    with the crash as proof, let the model reason to the root cause."""
    _ensure(CRASH2IA_OUT)
    srcs = []
    grab_src = False
    for a in argv:
        if a.startswith("--src="):
            srcs.append(a[len("--src="):])
        elif grab_src:
            srcs.append(a)
            grab_src = False
        elif a == "--src":
            grab_src = True
    if not srcs:
        srcs = [FUZZ_OUT]   # default: whatever `autosec.py fuzz` produced
    crashes = _collect_crashes(srcs)
    if not crashes:
        sys.stderr.write("[crash2ia] no crash inputs under %s\n"
                         % ", ".join(srcs))
        sys.stderr.write("hint: run `autosec.py fuzz` first, or pass "
                         "--src=PATH/TO/crash-*\n")
        return 0
    sys.stderr.write("[%s] crash2ia: %d crash(es) to triage\n" % (_ts(), len(crashes)))

    # Build the codetree index once (maps a backtrace frame -> function).
    sys.stderr.write("[%s] building call index...\n" % _ts())
    idx = codetree.Index()
    idx.build()

    specs = agentic.resolve_llms() or [None]   # mirror server.py default fallback
    sys.stderr.write("[%s] using IA: %s\n" % (_ts(), ", ".join(agentic._label(s) for s in specs)))

    by_file = {}
    for i, (inp, bt) in enumerate(crashes, 1):
        if not bt:
            sys.stderr.write("[%d] (%s) no backtrace; re-deriving under gdb\n" % (i, inp))
            if os.path.isfile(FUZZ_BIN):
                _ok, bt = _gdb_batch([FUZZ_BIN, inp],
                                     ["set pagination off", "run", "bt 40"],
                                     timeout=20)
            if not bt:
                bt = "(no backtrace available)"
        target = _crash_to_function(bt, idx)
        if not target:
            sys.stderr.write("[%d] (%s) no repo frame in backtrace - skipping\n"
                             % (i, os.path.basename(inp)))
            continue
        qn, path, line = target
        rel = os.path.relpath(path, REPO_ROOT) if os.path.isabs(path) else path
        fi = idx.by_name.get(qn)
        if not fi:
            sys.stderr.write("[%d] %s not in index - skipping\n" % (i, qn))
            continue
        sys.stderr.write("[%d] crash -> %s (%s:%d)\n" % (i, qn, rel, line))
        try:
            with open(inp, "rb") as fh:
                raw_hex = fh.read()[:96].hex()
        except OSError:
            raw_hex = "??"
        outdir = _ensure(os.path.join(CRASH2IA_OUT, "%02d_%s" % (i, re.sub(r"[^A-Za-z0-9_]", "_", qn)[-40:])))

        # Seed prompt: the crash is PROVEN. The IA reasons backward to the fix.
        seed = (
            "PROVEN CRASH (re-seed). The server crashed on the input below; the "
            "backtrace and the function under review are attached. The crash is "
            "real - find the ROOT-CAUSE bug in %s and the minimal fix.\n\n"
            "Crashing input (hex, first 96 bytes): %s\n\n"
            "Backtrace:\n```\n%s\n```\n" % (qn, raw_hex, bt.strip()[:3000]))
        with open(os.path.join(outdir, "seed.txt"), "w") as fh:
            fh.write(seed)

        # Drive the per-function agentic audit with the crash as seed. role=
        # "security" + an exploit_cb routes confirmed findings to run_exploit().
        blocks = agentic.audit_function(
            idx, fi, specs, role="security",
            sysprompt=CRASH_FEEDBACK_SYSTEM,
            exploit_cb=lambda f, finding, members: _exploit_cb(f, finding, outdir))
        if blocks:
            for b in blocks:
                sys.stderr.write("    [%d] IA: %s\n" % (i, b.replace("\n", " ")[:140]))
            by_file.setdefault(rel, []).append(
                "## crash2ia on %s (%s:%d)\nseed input: %s\n\n%s"
                % (qn, rel, line, os.path.basename(inp), "\n\n".join(blocks)))
    _write_findings(by_file)
    return 0


def _exploit_cb(fi, finding, outdir):
    """Confirm a crash2ia finding by re-running it through the exploit phase.
    Reuses server.py.run_exploit() against a freshly written one-shot FINDINGS."""
    sys.stderr.write("    [crash2ia] developing proof for %s...\n" % fi.qual_name)
    one = os.path.join(outdir, "findings.txt")
    rel = os.path.relpath(fi.file, REPO_ROOT)
    with open(one, "w") as fh:
        fh.write("=" * 70 + "\nFINDINGS: %s\n" % rel + "=" * 70 + "\n")
        fh.write(finding + "\n")
    # run_exploit reads the module-level FINDINGS constant; override it for this call.
    saved = S.FINDINGS
    try:
        S.FINDINGS = one
        S.run_exploit()
    except Exception as exc:
        sys.stderr.write("    [crash2ia] exploit phase error: %s\n" % exc)
        return ""
    finally:
        S.FINDINGS = saved
    return ("## crash2ia PROOF developed for %s\nexploit artifacts under %s\n"
            % (fi.qual_name, outdir))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
_HELP = """autosec.py - DETERMINISTIC security vectors feeding the IA audit + exploit.

Four subcommands. All reuse existing infra (test/_protoharness.py, test/fuzz/,
security/server.py + agentic.py) and need ZERO production C/C++ change.

Usage:
  python3 autosec.py <subcommand> [options]   2> progress.log

Subcommands:
  fuzz        Build & run test/fuzz/fuzz_protocol_parser.cpp under libFuzzer +
              AddressSanitizer + UBSan; collect crash/leak/oom inputs, re-run each
              under gdb for a backtrace, emit each as a FINDINGS candidate.
              Options:
                DIR                optional seed-corpus dir (its files seed the run)
                --budget=N         libFuzzer wall seconds (default CC_AUTOFUZZ_BUDGET=600)
  pexer       For every pre-auth Client state x every packet code (107, parsed
              from ProtocolParsingGeneral.cpp), send the packet in the WRONG
              state; flag a server crash (memory bug) or a substantive non-kick
              reply (auth-bypass candidate).
              Options:
                --timeout=S        per-code TCP wait seconds (default CC_PEXER_TIMEOUT=1.0)
  statediff   Snapshot the character HPS blob before vs after scripted drop_object
              and trade_cancel (2-client); mask moving stamps via
              reconnect_volatile_offsets; flag a crash mid-action or a non-empty
              masked diff after a no-op/rejected action (dupe / state leak).
              Rule-conformance (cash conservation, overflow caps) is proved by
              the exploit phase via GDB case (c).
  crash2ia    Collect crash artifacts, map each crash's top repo frame to its
              function via the codetree index, and re-seed the per-function IA
              audit so the model reasons backward from a PROVEN crash to the
              root-cause bug + minimal fix. Mirrors the adversarial exploit pass.
              Options:
                --src=PATH         crash dir / glob / file (repeatable; default the
                                   `fuzz` output dir); a numbered/ flat crash-* layout
                                   both work

Common options:
  --help, -h     Show this help (or a subcommand's: `autosec.py fuzz --help`)

Output:
  Findings -> OUTPUT_ROOT/autosec/findings.txt in server.py's FINDINGS block
  format (so `python3 server.py exploit` proves them). Per-subcommand artifacts
  (crashes, pexer logs, state dumps, IA transcripts) under
  OUTPUT_ROOT/autosec/<sub>/.

Environment:
  CC_AUTOFUZZ_BUDGET     libFuzzer wall budget seconds (default 600)
  CC_PEXER_TIMEOUT       pexer per-code TCP wait seconds (default 1.0)
  CC_IA_PANEL            IA model list for crash2ia (>= 2 => workgroup), else
                         the configured single model - same resolution as
                         server.py (--model / CC_IA_MODEL / claude[-cli])
  CC_CODECHECK_SCOPE     comma-separated repo subtrees for the crash2ia index
                         (default: the whole C/C++ tree excl. vendor)

Examples:
  # 10-minute libFuzzer pass on the wire parser, then triage crashes:
  python3 autosec.py fuzz 2> progress.log

  # Short fuzz run, seed from an existing corpus:
  python3 autosec.py fuzz /tmp/seeds --budget=300 2> progress.log

  # Wrong-state sweep (restarts the server between crashes):
  python3 autosec.py pexer 2> progress.log

  # Integrity byte-diff around drop + trade-cancel:
  python3 autosec.py statediff 2> progress.log

  # Re-seed the IA audit from the fuzzer output, then prove findings:
  python3 autosec.py crash2ia 2> progress.log
  python3 server.py exploit 2> progress.log      # proves autosec's findings.txt
"""


def _has_help(argv):
    return any(a in ("-h", "--help") for a in argv)


def main(argv):
    rest = argv[1:]
    # The first non-option token is the subcommand.
    sub = next((a for a in rest if not a.startswith("-")), None)
    # No subcommand (bare, or only -h/--help) -> top help.
    if sub is None:
        sys.stderr.write(_HELP)
        return 0
    # `autosec.py <sub> --help` -> top help (sub details are inline above).
    if _has_help(rest):
        sys.stderr.write(_HELP)
        return 0
    if sub not in ("fuzz", "pexer", "statediff", "crash2ia"):
        sys.stderr.write("error: unknown subcommand %r\n%s\n"
                         % (sub, _HELP))
        return 2
    args = [a for a in rest if a != sub]
    if sub == "fuzz":
        return run_fuzz(args)
    if sub == "pexer":
        return run_pexer(args)
    if sub == "statediff":
        return run_statediff(args)
    if sub == "crash2ia":
        return run_crash2ia(args)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
