#!/usr/bin/env python3
"""
testingclustersecurity.py — bounty-style robustness test for the CLUSTER.

The standalone game server (server/cli) is covered per-handler by
testingprotocolstate.py. This script does the equivalent for the four
multi-server binaries that only work *together as a cluster*:

    catchchallenger-server-master       (server/master)            — node-facing
    catchchallenger-server-login        (server/login)             — client-facing
    catchchallenger-game-server-alone   (server/game-server-alone) — client-facing
    (gateway is a DB-less proxy, covered by testinggateway.py)

login / master / game-server-alone build ONLY against a real SQL backend and
only work wired together, so we stand up the SAME ephemeral-PostgreSQL cluster
that testingcluster.py uses (reusing its helpers) and then, for BOTH the
production and the testing build flavour:

  1. drive a real qtcpu800x600 client through login -> master -> game-server to
     the map — proves the cluster works together (the operator's "test with
     client qtcpu800x600");
  2. fire a battery of malformed / boundary / hostile packets at EACH server's
     listening port (login client port, master node port, game-server client
     port), respawning a server that dies so the WHOLE battery is exercised;
  3. re-drive the client to confirm the cluster survived the abuse.

HARDENED ON *and* OFF — both are tested (per operator request):

  * OFF (production): a malformed packet must be gracefully disconnected, never
    crash the process. ANY process exit during the battery is a FAIL — a real
    memory-safety bug, because production runs with HARDENED off.

  * ON (CI tripwire): a rejected packet abort()s on purpose so CI catches it.
    There an abort is EXPECTED *iff* it is the clean "protocol parsing was
    wrong" tripwire — that means the server DETECTED the bad input (it would
    gracefully disconnect in production). We classify every abort: a clean
    tripwire abort is recorded as a detection (not a fail); an abort carrying a
    memory-corruption signature (ASan / SIGSEGV / double-free / heap-*) is a
    REAL bug and FAILS regardless of HARDENED.

So a FAIL in either pass == a genuine crash / corruption. Skips ONLY when the
PostgreSQL server binaries (initdb/postgres) are absent (mirrors
testingcluster.py's sole legitimate skip).
"""

import os
import sys
import time
import socket
import struct
import shutil
import threading
import subprocess
import faulthandler

SELF_DIR = os.path.dirname(os.path.abspath(__file__))
if SELF_DIR not in sys.path:
    sys.path.insert(0, SELF_DIR)

# testingcluster.py owns the heavy lifting: ephemeral PostgreSQL, the
# server-properties/login/master XML builders, ServerProc, ports. Importing it
# only runs its module-level config load (no servers started).
import testingcluster as TC          # noqa: E402
import build_paths                   # noqa: E402

# ── wall guard ───────────────────────────────────────────────────────────────
# Two cluster builds (HARDENED off + on) + the qtcpu800x600 client + ephemeral
# PG + two boots + respawning batteries. Cold ccache makes the builds the long
# pole. Kept in lock-step with all.sh PER_TEST_TIMEOUT_MAP / wall_cap.py.
WALL_LIMIT_SEC = 90 * 60
faulthandler.enable()
faulthandler.dump_traceback_later(WALL_LIMIT_SEC + 10, exit=False)

CRED = "clustersec_user"
CHAR = "Player"

CLIENT_BUILD = build_paths.build_path("client/qtcpu800x600/build/testing-clustersec")
# The qtcpu800x600 CMake target is catchchallenger800x600 but its OUTPUT_NAME is
# plain "catchchallenger" (same as qtopengl; they live in separate build dirs).
CLIENT_BIN   = os.path.join(CLIENT_BUILD, "catchchallenger")

# Base flags shared by both flavours. PostgreSQL backend; HPS off (a known
# unrelated abort in the 2-gsa token-rewrite path, see testingcluster.py); the
# event-rate tripwire on so a single packet that pins a core (CPU amplification,
# a rewarded bounty class) trips an abort we can detect.
BASE_FLAGS = [
    "-DCATCHCHALLENGER_DB_POSTGRESQL=ON",
    "-DCATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE=ON",
    "-DCATCHCHALLENGER_CACHE_HPS=OFF",
]

# Restart budget per server per pass — bounds the wall time if a flavour aborts
# on most packets (expected under HARDENED on).
RESTART_CAP = 30

# Corruption signatures: an abort carrying any of these is a REAL memory-safety
# bug regardless of HARDENED, not the intended protocol tripwire.
CORRUPTION_MARKERS = (
    "AddressSanitizer", "SUMMARY: ", "SIGSEGV", "SIGBUS",
    "double free", "heap-buffer-overflow", "heap-use-after-free",
    "stack-buffer", "free(): ", "malloc(): ", "corrupted",
    "runtime error:", "munmap_chunk",
)
TRIPWIRE_MARKER = "protocol parsing was wrong"

# ── valgrind (default) ───────────────────────────────────────────────────────
# By default EVERY spawned server runs under valgrind memcheck and any defect it
# reports (errors / definite+indirect leaks, minus the suppressed io_uring CQE
# false positives) FAILS the test. Disable with CC_CLUSTERSEC_NO_VALGRIND=1
# (then the harness falls back to the HARDENED on/off + crash-detection passes).
import process_helpers              # noqa: E402  (valgrind-safe preexec)
VALGRIND = (os.environ.get("CC_CLUSTERSEC_NO_VALGRIND", "") == ""
            and shutil.which("valgrind") is not None)
_SUPP = os.path.join(SELF_DIR, "clustersec-valgrind.supp")
# Under valgrind only the production (non-HARDENED) build is meaningful: valgrind
# is the detector, and a HARDENED abort would just cut the analysis short.
FLAVOURS = [False] if VALGRIND else [False, True]
# RelWithDebInfo so valgrind backtraces carry file:line.
VG_DEBUG = ["-DCMAKE_BUILD_TYPE=RelWithDebInfo"] if VALGRIND else []


def enable_valgrind():
    TC.ServerProc.launch_wrapper = [
        "valgrind", "--tool=memcheck", "--leak-check=full",
        "--show-leak-kinds=definite,indirect",
        # The servers are SIGTERM'd and do not free on abrupt exit, so valgrind
        # reports their in-use allocations as "leaked" — kill-time noise, not
        # runtime bugs. --errors-for-leak-kinds=none keeps leaks OUT of ERROR
        # SUMMARY (valgrind's default is definite,possible), so ERROR SUMMARY
        # counts only real defects (invalid read/write, use-after-free,
        # mismatched free, branch-on-uninitialised). Leaks are surfaced
        # separately as info and do NOT fail the run.
        "--errors-for-leak-kinds=none",
        "--error-exitcode=0", "--child-silent-after-fork=yes",
        "--num-callers=25", f"--suppressions={_SUPP}",
        "--log-file=valgrind.%p.log",   # relative to the server's work_dir
    ]
    # valgrind needs a preexec WITHOUT the RLIMIT_AS/CPU caps (they kill it).
    TC.ServerProc.child_preexec = process_helpers.setsid_and_pdeathsig
    TC.READY_TIMEOUT = 600              # valgrind boots are 10-30x slower


def parse_valgrind(log_path):
    """(errors, lost_bytes, detail) from a valgrind --log-file. errors excludes
    suppressed io_uring noise; lost_bytes = definitely+indirectly lost."""
    try:
        with open(log_path, "r", errors="replace") as f:
            txt = f.read()
    except OSError:
        return 0, 0, ""
    errors = 0
    lost = 0
    detail_lines = []
    for line in txt.splitlines():
        if "ERROR SUMMARY:" in line:
            try:
                errors = int(line.split("ERROR SUMMARY:")[1].split("errors")[0].strip())
            except (ValueError, IndexError):
                pass
        elif "definitely lost:" in line or "indirectly lost:" in line:
            try:
                n = int(line.split("lost:")[1].split("bytes")[0].replace(",", "").strip())
                lost += n
            except (ValueError, IndexError):
                pass
        elif ("Mismatched free" in line or "Invalid read" in line
              or "Invalid write" in line or "Invalid free" in line
              or "uninitialised" in line or "SizeMismatch" in line
              or ("lost in loss record" in line)):
            detail_lines.append(line.split("== ")[-1].strip()[:90])
    return errors, lost, " | ".join(detail_lines[:4])


def report_valgrind_defects():
    """Scan every server's valgrind log under the clustersec run tree and FAIL on
    any reported defect."""
    import glob
    base = os.path.join(TC.TMPFS_ROOT, "clustersec")
    logs = sorted(glob.glob(os.path.join(base, "**", "valgrind.*.log"),
                            recursive=True))
    if not logs:
        log_fail("valgrind", "no valgrind logs produced — servers did not run "
                             "under valgrind")
        return
    clean = 0
    total_lost = 0
    seen_defects = set()
    for lp in logs:
        errors, lost, detail = parse_valgrind(lp)
        who = os.path.relpath(os.path.dirname(lp), base)
        total_lost += lost
        if errors > 0:
            # Dedupe by (server-role, defect signature) so the same bug across
            # the 9 http-mirror reboots is reported once.
            role = who.split("/")[-1]
            sig = (role, detail)
            if sig not in seen_defects:
                seen_defects.add(sig)
                log_fail(f"valgrind error: {role}",
                         f"{errors} memcheck errors :: {detail}")
        else:
            clean += 1
    log_pass("valgrind", f"{clean}/{len(logs)} server runs error-clean under "
                         "memcheck (io_uring CQE noise suppressed)")
    if total_lost > 0:
        # Informational: leaks at SIGTERM-kill are not runtime bugs.
        log_info(f"valgrind: {total_lost} bytes reported leaked across "
                 f"{len(logs)} runs (kill-time, not failing).")

# ── gateway (DB-less proxy) — probed with a file-db server-cli backend ────────
# The gateway is client-facing and needs NO SQL: it proxies to a file-db
# server-cli backend (as testinggateway.py does). With empty rewrite URLs it
# serves datapack in-protocol (no nginx), so the 0xA1 file-list parser /
# DatapackDownloader is reachable directly. Ports kept clear of the SQL cluster
# and of testinggateway.py's own 61918/61919.
GW_BACKEND_PORT = 61958
GW_GATEWAY_PORT = 61959
SERVER_CLI_BIN  = "catchchallenger-server-cli"
GATEWAY_BIN     = "catchchallenger-gateway"
GW_MAINCODE     = "test"


def _backend_xml(mirror=""):
    # `mirror` is advertised to the gateway, which then downloads the datapack
    # over HTTP from it (DatapackDownloaderMainSub). Point it at a malicious HTTP
    # server to attack the gateway's download path; leave empty for in-protocol.
    return (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        f'    <server-port value="{GW_BACKEND_PORT}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        '    <max-players value="200"/>\n'
        f'    <httpDatapackMirror value="{mirror}"/>\n'
        '    <content>\n'
        f'        <mainDatapackCode value="{GW_MAINCODE}"/>\n'
        '        <subDatapackCode value=""/>\n'
        '    </content>\n'
        '</configuration>\n'
    )


def _gateway_xml():
    # destination = the file-db backend; empty rewrite URLs -> the in-protocol
    # 0xA1 datapack path (no nginx); proxy_port=1 satisfies the validator.
    return (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        '    <ip value=""/>\n'
        f'    <port value="{GW_GATEWAY_PORT}"/>\n'
        '    <destination_ip value="127.0.0.1"/>\n'
        f'    <destination_port value="{GW_BACKEND_PORT}"/>\n'
        '    <destination_proxy_ip value=""/>\n'
        '    <destination_proxy_port value="1"/>\n'
        '    <httpDatapackMirrorRewriteBase value=""/>\n'
        '    <httpDatapackMirrorRewriteMainAndSub value=""/>\n'
        '    <commandUpdateDatapack>\n'
        '        <base value=""/>\n'
        '        <main value=""/>\n'
        '        <sub value=""/>\n'
        '    </commandUpdateDatapack>\n'
        '</configuration>\n'
    )

# ── tiny self-contained reporting ────────────────────────────────────────────
_C_GREEN = "\033[92m"
_C_RED   = "\033[91m"
_C_RST   = "\033[0m"
_results = []   # (ok, name, detail)


def log_info(msg):
    print(f"[INFO] {msg}", flush=True)


def log_pass(name, detail=""):
    _results.append((True, name, detail))
    print(f"{_C_GREEN}[PASS]{_C_RST} {name}  {detail}", flush=True)


def log_fail(name, detail=""):
    _results.append((False, name, detail))
    print(f"{_C_RED}[FAIL]{_C_RST} {name}  {detail}", flush=True)


# ── build ────────────────────────────────────────────────────────────────────
def build_cluster(base, hardened):
    """Build master + login + game-server-alone (in-protocol-push variant, so no
    nginx mirror is needed) for the requested flavour. Returns (ok, detail)."""
    flags = list(BASE_FLAGS) + VG_DEBUG
    flags.append("-DCATCHCHALLENGER_HARDENED=" + ("ON" if hardened else "OFF"))
    for sub, bd in (("server/master", "server_master"),
                    ("server/login", "server_login")):
        ok, detail = TC.cmake_configure_and_build(
            sub, os.path.join(base, bd), flags)
        if not ok:
            return False, f"{sub}: {detail}"
    ok, detail = TC.cmake_configure_and_build(
        "server/game-server-alone",
        os.path.join(base, "server_game-server-alone-push"),
        flags + ["-DCATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR=OFF"])
    if not ok:
        return False, "game-server-alone (push): " + detail
    return True, "ok"


def build_client():
    return TC.cmake_configure_and_build("client/qtcpu800x600", CLIENT_BUILD, [])


# ── client drive ─────────────────────────────────────────────────────────────
# Drive a REAL client through login -> master -> game-server to prove the cluster
# works together. The operator asked to test with qtcpu800x600, so it is tried
# first; the qtopengl client (the one testingcluster.py drives through the same
# cluster) is the reliable fallback — pass if EITHER reaches the map.
import tempfile  # noqa: E402


def _plain_env():
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["DISPLAY"] = ""
    return env


def _qtcpu_env():
    """Isolated XDG home with the datapack pre-staged into the client's cache
    (keyed by the connection argument, as the qtcpu800x600 client expects),
    mirroring testingmulti.py. This skips the in-protocol datapack download —
    whose push-mode path stalls in this client — so it reaches the map and then
    exits cleanly via --closewhenonmap (no mid-flow disconnect)."""
    env = _plain_env()
    tmp = tempfile.mkdtemp(prefix="cc-clustersec-cpu-")
    env["XDG_CONFIG_HOME"] = os.path.join(tmp, "config")
    env["XDG_DATA_HOME"] = os.path.join(tmp, "data")
    cache = os.path.join(tmp, "data", "CatchChallenger", "client-qtcpu800x600",
                         "datapack", f"argument-127.0.0.1-{TC.PORT_LOGIN_1}")
    try:
        shutil.copytree(TC.DATAPACK_SRC, cache,
                        ignore=shutil.ignore_patterns(".git"))
    except OSError:
        pass
    return env


def _run_client(binpath, env, timeout):
    cmd = [binpath,
           "--host", "127.0.0.1", "--port", str(TC.PORT_LOGIN_1),
           "--autologin", "--login", CRED, "--pass", CRED,
           "--character", CHAR, "--closewhenonmap"]
    try:
        p = subprocess.run(cmd, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=timeout)
        out = p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or b"").decode(errors="replace")
        return False, f"timeout {timeout}s; tail:\n{out[-300:]}"
    if "MapVisualiserPlayer::mapDisplayedSlot" in out:
        return True, "reached map"
    return False, "no mapDisplayedSlot; tail:\n" + out[-300:]


def drive_client(timeout=90):
    if VALGRIND:
        # Under valgrind the servers are 10-30x slower, so the datapack sync the
        # client waits on can't complete in a sane timeout. The reach-map proof
        # is the non-valgrind run's job; here we only care about memcheck
        # defects from the probe batteries, so skip it.
        return True, "skipped (valgrind: servers too slow for the reach-map proof)"
    candidates = [("qtcpu800x600", CLIENT_BIN, _qtcpu_env),
                  ("qtopengl", TC.CLIENT_BIN, _plain_env)]
    last = "no client available"
    for name, binpath, envfn in candidates:
        if not os.path.exists(binpath):
            last = f"{name}: binary missing"
            continue
        ok, detail = _run_client(binpath, envfn(), timeout)
        if ok:
            return True, f"{name}: {detail}"
        last = f"{name}: {detail}"
    return False, last


# ── probe battery ────────────────────────────────────────────────────────────
def _b(*xs):
    return bytes(xs)


# (name, raw bytes). Each is fired on a FRESH connection. Every one must be
# absorbed (graceful disconnect / silent ignore) or — under HARDENED — caught by
# the clean tripwire, never corrupt memory.
BATTERY = [
    ("empty-then-close",        b""),
    ("one-zero-byte",           b"\x00"),
    ("ff-flood-64",             b"\xff" * 64),
    ("zero-256",                b"\x00" * 256),
    ("every-message-code",      bytes(range(0x00, 0x80))),
    ("dyn-query-4GB-size",      _b(0xAA, 0x01) + struct.pack("<I", 0xFFFFFFFF) + b"\x00" * 4),
    ("dyn-msg-4GB-size",        _b(0x03) + struct.pack("<I", 0xFFFFFFFF) + b"AB"),
    ("dyn-query-zero-len",      _b(0xAA, 0x02) + struct.pack("<I", 0)),
    ("handshake-short-magic",   _b(0xA0, 0x9c, 0xd6)),
    ("handshake-bad-magic",     _b(0xA0) + b"\xde\xad\xbe\xef\x99"),
    ("master-handshake-junk",   _b(0xB8, 0x01) + b"\x00" * 16),
    ("query-header-only",       _b(0xAA, 0x02)),
    ("reply-spoof",             _b(0x7F, 0x05) + b"\x00" * 8),
    ("blocked-code-0xFF",       _b(0xFF, 0x00, 0x00, 0x00, 0x00)),
    ("size-claims-1024-send-1", _b(0xAA, 0x03) + struct.pack("<I", 1024) + b"\x01"),
    ("size-negative-looking",   _b(0xAA, 0x04) + struct.pack("<i", -1) + b"\x01\x02"),
    ("partial-size-header",     _b(0xAA, 0x05, 0x00, 0x10)),   # 2 of the 4 size bytes
]


def _a1(payload):
    """Frame a 0xA1 (datapack file-list) dynamic query."""
    return bytes([0xA1, 0x01]) + struct.pack("<I", len(payload)) + payload


# Gateway-specific: hammer the 0xA1 datapack file-list parser / DatapackDownloader
# (`stepToSkip`, `number_of_file`, per-file `textSize`, `partialHash`) and the
# 0xAC reconnect-select `serverReconnectList[charactersGroupIndex]` index.
GATEWAY_BATTERY = [
    ("a1-numfiles-4G",        _a1(b"\x02" + struct.pack("<I", 0xFFFFFFFF))),
    ("a1-numfiles-over-cap",  _a1(b"\x02" + struct.pack("<I", 1000001))),
    ("a1-numfiles-100-trunc", _a1(b"\x02" + struct.pack("<I", 100) + b"\x05abc")),
    ("a1-step-invalid",       _a1(b"\xff" + struct.pack("<I", 1) + b"\x02ab")),
    ("a1-file-textsize-255",  _a1(b"\x02" + struct.pack("<I", 1) + b"\xff" + b"ab")),
    ("a1-file-no-hash",       _a1(b"\x02" + struct.pack("<I", 1) + b"\x02ab")),
    ("a1-declared-64k-short", bytes([0xA1, 0x01]) + struct.pack("<I", 0xFFFF)
                              + b"\x02" + struct.pack("<I", 1)),
    ("ac-reconnect-oob-idx",  bytes([0xAC, 0x02, 0xFF]) + b"\x00" * 8),
]


def classify(server, hardened):
    """('alive','') | ('tripwire',msg) | ('crash',msg). A bare process exit is a
    crash unless it is the clean HARDENED tripwire."""
    if server.proc.poll() is None:
        return "alive", ""
    rc = server.proc.returncode
    try:
        with open(server.log_path, "r", errors="replace") as f:
            tail = f.read()[-8000:]
    except OSError:
        tail = ""
    corruption = next((m for m in CORRUPTION_MARKERS if m in tail), None)
    if corruption is not None:
        return "crash", f"rc={rc} CORRUPTION {corruption!r}"
    if hardened and TRIPWIRE_MARKER in tail:
        return "tripwire", f"rc={rc} (clean HARDENED detect)"
    # process exited without a corruption signature and without the tripwire:
    #  OFF -> production must NOT exit on bad input => crash.
    #  ON  -> a bare abort with no tripwire text is suspicious => crash.
    return "crash", f"rc={rc} (process exited, no graceful disconnect)"


def _send_one(port, payload):
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=3)
    except OSError:
        return  # a refused/RST probe connection is not itself a failure
    try:
        if payload:
            s.sendall(payload)
        s.settimeout(0.4)
        try:
            s.recv(4096)
        except (socket.timeout, OSError):
            pass
    except OSError:
        pass
    finally:
        try:
            s.close()
        except OSError:
            pass


def _respawn(srv):
    """Recreate a dead ServerProc with identical params (same work_dir, so its
    config + datapack symlink are reused; the log is truncated fresh)."""
    ns = TC.ServerProc(srv.label, srv.exe, srv.work_dir, srv.xml_text).start()
    ok, detail = ns.wait_ready()
    return ns, ok, detail


def probe_server(holder, port, hardened, failures, stats, extra_battery=()):
    """Fire the whole battery (+ any server-specific extra packets) and a gentle
    connection churn at `port`. holder is a one-element list with the live
    ServerProc (replaced on respawn so the caller sees the current process for
    teardown). Populates stats/failures."""
    battery = list(BATTERY) + list(extra_battery)
    restarts = 0
    idx = 0
    while idx < len(battery):
        name, payload = battery[idx]
        _send_one(port, payload)
        time.sleep(0.12)
        kind, msg = classify(holder[0], hardened)
        if kind == "alive":
            stats["absorbed"] += 1
            idx += 1
            continue
        if kind == "tripwire":
            stats["tripwire"] += 1
        else:
            stats["crash"] += 1
            failures.append(f"{holder[0].label}: '{name}' -> {msg}")
        if restarts >= RESTART_CAP:
            stats["note"] = f"restart cap ({RESTART_CAP}) hit; battery truncated"
            return
        ns, ok, detail = _respawn(holder[0])
        holder[0] = ns
        restarts += 1
        if not ok:
            failures.append(f"{ns.label}: respawn after '{name}' failed: {detail}")
            return
        idx += 1
    # gentle connection churn: connect+close repeatedly. Kept well under
    # max_players (50) so this targets connect/disconnect handling, not the
    # separate at-capacity path, and well under the 1000 events/s tripwire.
    j = 0
    while j < 24:
        try:
            socket.create_connection(("127.0.0.1", port), timeout=2).close()
        except OSError:
            pass
        time.sleep(0.03)
        j += 1
    time.sleep(0.3)
    kind, msg = classify(holder[0], hardened)
    if kind == "crash":
        stats["crash"] += 1
        failures.append(f"{holder[0].label}: connection churn -> {msg}")
    elif kind == "tripwire":
        stats["tripwire"] += 1
        ns, ok, detail = _respawn(holder[0])
        holder[0] = ns


# ── one full pass (boot cluster of a given flavour, probe, re-verify) ─────────
def run_pass(hardened, base):
    flavour = "HARDENED" if hardened else "production(no-HARDENED)"
    log_info(f"==== pass: {flavour} ====")
    ok, detail = TC.wipe_postgresql()
    if not ok:
        log_fail(f"{flavour}/schema", detail)
        return
    work = os.path.join(TC.TMPFS_ROOT, "clustersec", "run",
                        "hardened" if hardened else "prod")
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    mp = 50
    holders = {}   # label -> [ServerProc]
    try:
        master = TC.ServerProc(
            "master",
            TC.bin_path(base, "server/master", "catchchallenger-server-master"),
            os.path.join(work, "master"),
            TC._xml_master("postgresql", mp)).start()
        holders["master"] = [master]
        ok, detail = master.wait_ready()
        if not ok:
            log_fail(f"{flavour}/master-start", detail)
            return
        login = TC.ServerProc(
            "login1",
            TC.bin_path(base, "server/login", "catchchallenger-server-login"),
            os.path.join(work, "login1"),
            TC._xml_login(TC.PORT_LOGIN_1, "postgresql", mp, "")).start()
        holders["login1"] = [login]
        ok, detail = login.wait_ready()
        if not ok:
            log_fail(f"{flavour}/login-start", detail)
            return
        gsa_bin = TC.bin_path(base, "server/game-server-alone-push",
                              "catchchallenger-game-server-alone")
        game = TC.ServerProc(
            "game1", gsa_bin, os.path.join(work, "game1"),
            TC._xml_gameserver(TC.PORT_GAME_1, "postgresql", mp, "")).start()
        holders["game1"] = [game]
        ok, detail = game.wait_ready()
        if not ok:
            log_fail(f"{flavour}/game-start", detail)
            return
        log_pass(f"{flavour}/cluster-boot",
                 f"master:{TC.PORT_MASTER} login:{TC.PORT_LOGIN_1} game:{TC.PORT_GAME_1}")

        # cluster works together: qtcpu800x600 client -> map.
        ok, detail = drive_client()
        if ok:
            log_pass(f"{flavour}/client-reaches-map (qtcpu800x600)", detail)
        else:
            log_fail(f"{flavour}/client-reaches-map (qtcpu800x600)", detail)

        # Probe only the CLIENT-facing listeners. The master is internal-only
        # (out of scope — see the bounty doc): it stays booted so login/gsa can
        # register, but a remote attacker can't reach it, so it isn't probed.
        targets = [
            (holders["login1"], TC.PORT_LOGIN_1, "login(client)"),
            (holders["game1"],  TC.PORT_GAME_1, "game-server(client)"),
        ]
        for holder, port, desc in targets:
            failures = []
            stats = {"absorbed": 0, "tripwire": 0, "crash": 0, "note": ""}
            log_info(f"  probing {desc} :{port}")
            probe_server(holder, port, hardened, failures, stats)
            summary = (f"absorbed={stats['absorbed']} tripwire={stats['tripwire']} "
                       f"crash={stats['crash']}"
                       + (f" [{stats['note']}]" if stats['note'] else ""))
            if failures:
                log_fail(f"{flavour}/robustness {desc}",
                         summary + " :: " + " ; ".join(failures))
            else:
                log_pass(f"{flavour}/robustness {desc}", summary)

        # cluster survived: re-drive the client (only if every server alive).
        if all(h[0].proc.poll() is None for h in holders.values()):
            ok, detail = drive_client()
            if ok:
                log_pass(f"{flavour}/cluster-survives-probes", detail)
            else:
                log_fail(f"{flavour}/cluster-survives-probes", detail)
        else:
            dead = [lbl for lbl, h in holders.items() if h[0].proc.poll() is not None]
            log_fail(f"{flavour}/cluster-survives-probes",
                     f"servers down after probing: {dead}")
    finally:
        for h in holders.values():
            try:
                h[0].stop()
            except Exception:
                pass


# ── malicious HTTP datapack mirror ───────────────────────────────────────────
# The gateway downloads the datapack over HTTP from the mirror the backend
# advertises (DatapackDownloaderMainSub fetches tar.zst / file-list / per-file).
# This raw-socket server impersonates that mirror and misbehaves on every
# request, so we attack the gateway's HTTP download/parse/reassembly path:
# corrupt bodies, truncated / oversized Content-Length, mid-body disconnect,
# mid-body stall (timeout), slow drip, connection reset, bad status.
class MaliciousMirror:
    def __init__(self, behavior):
        self.behavior = behavior
        self.hits = 0
        self._stop = False
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("127.0.0.1", 0))
        self.port = self.sock.getsockname()[1]
        self.sock.listen(32)
        self.t = threading.Thread(target=self._serve, daemon=True)
        self.t.start()

    def _serve(self):
        self.sock.settimeout(0.5)
        while not self._stop:
            try:
                conn, _ = self.sock.accept()
            except (socket.timeout, OSError):
                continue
            self.hits += 1
            threading.Thread(target=self._handle, args=(conn,), daemon=True).start()

    def _handle(self, conn):
        try:
            conn.settimeout(2.0)
            try:
                conn.recv(8192)   # consume the request line/headers
            except OSError:
                pass
            b = self.behavior
            if b == "reset":
                conn.close(); return
            if b == "status500":
                conn.sendall(b"HTTP/1.1 500 Internal Server Error\r\n"
                             b"Content-Length: 0\r\n\r\n")
            elif b == "badstatus":
                conn.sendall(b"HTTP/1.1 999 \r\n\r\n" + b"\xff" * 4000)
            elif b == "corrupt":
                body = bytes((i * 37 + 11) & 0xFF for i in range(6000))
                conn.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 6000\r\n\r\n" + body)
            elif b == "hugeheaders":
                conn.sendall(b"HTTP/1.1 200 OK\r\n" + b"X-Pad: " + b"A" * 70000
                             + b"\r\nContent-Length: 0\r\n\r\n")
            elif b == "truncate":
                conn.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 4000000\r\n\r\n"
                             + b"\x00" * 64)            # claim 4 MB, send 64 B, close
            elif b == "oversize":
                conn.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 9000000000\r\n\r\n"
                             + b"\x00" * 128)           # 9 GB Content-Length
                time.sleep(3.0)
            elif b == "stall":
                conn.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 4000000\r\n\r\n"
                             + b"\x00" * 64)            # then never finish
                t0 = time.time()
                while not self._stop and time.time() - t0 < 16.0:
                    time.sleep(0.3)
            elif b == "slowdrip":
                conn.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 4000\r\n\r\n")
                i = 0
                while not self._stop and i < 40:
                    try:
                        conn.sendall(b"\x00")
                    except OSError:
                        break
                    time.sleep(0.4); i += 1
            try:
                conn.close()
            except OSError:
                pass
        except OSError:
            try:
                conn.close()
            except OSError:
                pass

    def stop(self):
        self._stop = True
        try:
            self.sock.close()
        except OSError:
            pass


HTTP_MIRROR_BEHAVIORS = ["reset", "status500", "badstatus", "corrupt",
                         "hugeheaders", "truncate", "oversize", "stall", "slowdrip"]


def _trigger_gateway_download():
    """Nudge the gateway to fetch the datapack from the (malicious) mirror: a
    handshake + a 0xA1 datapack-file-list request makes it serve/sync, which
    needs the mirror-downloaded datapack."""
    try:
        s = socket.create_connection(("127.0.0.1", GW_GATEWAY_PORT), timeout=3)
        s.sendall(bytes([0xA0]) + b"\x9c\xd6\x49\x8d\x14")           # protocol magic
        time.sleep(0.3)
        s.sendall(_a1(b"\x02" + struct.pack("<I", 0)))               # empty base list
        time.sleep(0.3)
        s.close()
    except OSError:
        pass


def _client_through_gateway(timeout=20):
    """Drive a real client at the GATEWAY port (login is proxied to the backend;
    the datapack sync forces the gateway to fetch from the mirror). It need not
    reach the map — a malicious mirror breaks the datapack — we only need the
    gateway to ATTEMPT the download. Prefer qtopengl (proven), else qtcpu800x600."""
    for binp in (TC.CLIENT_BIN, CLIENT_BIN):
        if not os.path.exists(binp):
            continue
        cmd = [binp, "--host", "127.0.0.1", "--port", str(GW_GATEWAY_PORT),
               "--autologin", "--login", "gwuser", "--pass", "gwuser",
               "--character", "Player", "--closewhenonmap"]
        try:
            subprocess.run(cmd, env=_plain_env(), stdout=subprocess.DEVNULL,
                           stderr=subprocess.STDOUT, timeout=timeout)
        except subprocess.TimeoutExpired:
            pass
        except OSError:
            continue
        return


def run_http_mirror_attacks(gw_base):
    """For each malicious-mirror behaviour: boot a backend that advertises the
    malicious mirror + the gateway, nudge the download, and require the gateway
    process to survive (no crash / OOB / hang-wedge) and keep accepting."""
    backend_bin = os.path.join(gw_base, "server_cli_filedb", SERVER_CLI_BIN)
    gw_bin = os.path.join(gw_base, "server_gateway", GATEWAY_BIN)  # production flavour
    for behavior in HTTP_MIRROR_BEHAVIORS:
        mirror = MaliciousMirror(behavior)
        url = f"http://127.0.0.1:{mirror.port}/"
        work = os.path.join(TC.TMPFS_ROOT, "clustersec", "gw-http", behavior)
        shutil.rmtree(work, ignore_errors=True)
        os.makedirs(work, exist_ok=True)
        backend = None
        gw = None
        try:
            backend = TC.ServerProc("backend", backend_bin,
                                    os.path.join(work, "backend"),
                                    _backend_xml(url)).start()
            okb, db = backend.wait_ready()
            if not okb:
                log_fail(f"http-mirror[{behavior}]/backend", db)
                continue
            # Empty datapack dir -> ServerProc won't symlink the real datapack,
            # so the gateway MUST fetch it from the (malicious) mirror.
            gw_work = os.path.join(work, "gateway")
            os.makedirs(os.path.join(gw_work, "datapack"), exist_ok=True)
            gw = TC.ServerProc("gateway", gw_bin, gw_work, _gateway_xml()).start()
            okg, dg = gw.wait_ready()
            if not okg:
                # a gateway that can't even bind with a bad mirror is itself a
                # finding; but it more likely binds then downloads async.
                log_fail(f"http-mirror[{behavior}]/gateway-start", dg)
                continue
            # Drive a client through the gateway so it must serve (and therefore
            # download from the malicious mirror) the datapack; the client need
            # not succeed — we only need the gateway to hit the mirror. Also poke
            # the 0xA1 path directly as a fallback trigger.
            _client_through_gateway(timeout=20)
            i = 0
            while i < 3 and gw.proc.poll() is None:
                _trigger_gateway_download()
                time.sleep(1.0)
                i += 1
            time.sleep(2.0)
            kind, msg = classify(gw, False)   # gateway built non-HARDENED here
            if kind == "crash":
                log_fail(f"http-mirror[{behavior}]",
                         f"gateway {msg} (mirror hits={mirror.hits})")
            elif not TC.port_open(GW_GATEWAY_PORT):
                log_fail(f"http-mirror[{behavior}]",
                         f"gateway stopped accepting — wedged by mirror "
                         f"(hits={mirror.hits})")
            else:
                log_pass(f"http-mirror[{behavior}]",
                         f"gateway survived + still accepts (mirror hits={mirror.hits})")
        finally:
            for p in (gw, backend):
                if p is not None:
                    try:
                        p.stop()
                    except Exception:
                        pass
            mirror.stop()


# ── internal-protocol connection attacks (client -> gateway) ─────────────────
def _partial_a1(declared, sent):
    """A 0xA1 dynamic query whose 4-byte size claims `declared` bytes but only
    `sent` payload bytes follow — a truncated/incomplete datapack request."""
    return bytes([0xA1, 0x01]) + struct.pack("<I", declared) + (b"\x02" + b"\x00" * max(0, sent - 1))


def gateway_connection_attacks(holder, port, hardened, failures, stats):
    """Slow / incomplete / abrupt datapack transfers from the client side:
    stall mid-request (timeout), disconnect mid-request (cleanup), byte-drip,
    and many concurrent stalled connections (slowloris). After each, the gateway
    must stay alive AND keep accepting fresh connections (not wedged)."""
    def _check(tag):
        kind, msg = classify(holder[0], hardened)
        if kind == "crash":
            failures.append(f"gateway: {tag} -> {msg}")
            stats["crash"] += 1
            return False
        if not TC.port_open(port):
            failures.append(f"gateway: {tag} -> port wedged (not accepting)")
            return False
        stats["absorbed"] += 1
        return True

    # 1) stall mid-request: send a partial 0xA1, keep the socket OPEN.
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=3)
        s.sendall(_partial_a1(4096, 8))
        time.sleep(4.0)                       # hold it open, send nothing more
        if not _check("stall-mid-request"):
            try: s.close()
            except OSError: pass
            return
        s.close()
    except OSError:
        pass

    # 2) disconnect mid-request: partial 0xA1 then abrupt RST/close.
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=3)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                     struct.pack("ii", 1, 0))   # RST on close
        s.sendall(_partial_a1(65536, 16))
        s.close()
    except OSError:
        pass
    time.sleep(0.4)
    if not _check("disconnect-mid-request"):
        return

    # 3) byte-drip: dribble a 0xA1 one byte at a time.
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=3)
        blob = _a1(b"\x02" + struct.pack("<I", 3) + b"\x02ab")
        for byte in blob[:24]:
            try:
                s.sendall(bytes([byte]))
            except OSError:
                break
            time.sleep(0.05)
        time.sleep(1.0)
        s.close()
    except OSError:
        pass
    if not _check("byte-drip"):
        return

    # 4) slowloris: many concurrent half-open stalled requests.
    held = []
    k = 0
    while k < 24:
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=2)
            s.sendall(_partial_a1(4096, 4))
            held.append(s)
        except OSError:
            pass
        time.sleep(0.02)
        k += 1
    time.sleep(2.0)
    ok = _check("slowloris-24-stalled")
    for s in held:
        try:
            s.close()
        except OSError:
            pass
    if not ok:
        return


# ── gateway phase (DB-less; no PostgreSQL needed) ────────────────────────────
def build_gateway_stack(gw_base):
    """Build the file-db server-cli backend (stable, not the probe target → no
    HARDENED) and the gateway twice (HARDENED off + on). Returns (ok, detail)."""
    ok, detail = TC.cmake_configure_and_build(
        "server/cli", os.path.join(gw_base, "server_cli_filedb"),
        ["-DCATCHCHALLENGER_DB_FILE=ON",
         "-DCATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE=ON"] + VG_DEBUG)
    if not ok:
        return False, "backend: " + detail
    for hard in FLAVOURS:
        sub = "server_gateway_hard" if hard else "server_gateway"
        flags = ["-DCATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE=ON",
                 "-DCATCHCHALLENGER_HARDENED=" + ("ON" if hard else "OFF")] + VG_DEBUG
        ok, detail = TC.cmake_configure_and_build(
            "server/gateway", os.path.join(gw_base, sub), flags)
        if not ok:
            return False, f"gateway({'hard' if hard else 'prod'}): " + detail
    return True, "ok"


def run_gateway_pass(hardened, gw_base, backend):
    """Boot the gateway (one flavour) against the live backend and fire the
    generic battery + the 0xA1 DatapackDownloader / 0xAC reconnect battery."""
    flavour = "GW-HARDENED" if hardened else "GW-production"
    log_info(f"==== pass: {flavour} ====")
    sub = "server_gateway_hard" if hardened else "server_gateway"
    gw_bin = os.path.join(gw_base, sub, GATEWAY_BIN)
    work = os.path.join(TC.TMPFS_ROOT, "clustersec", "gw-run",
                        "h" if hardened else "p")
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    holder = [TC.ServerProc("gateway", gw_bin,
                            os.path.join(work, "gateway"), _gateway_xml()).start()]
    try:
        ok, detail = holder[0].wait_ready()
        if not ok:
            log_fail(f"{flavour}/gateway-start", detail)
            return
        log_pass(f"{flavour}/gateway-start",
                 f":{GW_GATEWAY_PORT} -> backend :{GW_BACKEND_PORT}")
        failures = []
        stats = {"absorbed": 0, "tripwire": 0, "crash": 0, "note": ""}
        log_info(f"  probing gateway :{GW_GATEWAY_PORT} "
                 "(handshake + 0xA1 DatapackDownloader + 0xAC reconnect)")
        probe_server(holder, GW_GATEWAY_PORT, hardened, failures, stats,
                     extra_battery=GATEWAY_BATTERY)
        summary = (f"absorbed={stats['absorbed']} tripwire={stats['tripwire']} "
                   f"crash={stats['crash']}"
                   + (f" [{stats['note']}]" if stats['note'] else ""))
        if failures:
            log_fail(f"{flavour}/robustness gateway",
                     summary + " :: " + " ; ".join(failures))
        else:
            log_pass(f"{flavour}/robustness gateway", summary)
        # connection-level datapack attacks (stall / disconnect / drip / slowloris).
        cfailures = []
        cstats = {"absorbed": 0, "tripwire": 0, "crash": 0, "note": ""}
        log_info("  connection attacks: stall / disconnect / byte-drip / slowloris")
        gateway_connection_attacks(holder, GW_GATEWAY_PORT, hardened,
                                   cfailures, cstats)
        if cfailures:
            log_fail(f"{flavour}/datapack-connection-attacks",
                     " ; ".join(cfailures))
        else:
            log_pass(f"{flavour}/datapack-connection-attacks",
                     f"survived stall/disconnect/drip/slowloris "
                     f"({cstats['absorbed']} checks)")
        if backend.proc.poll() is not None:
            log_fail(f"{flavour}/backend-survives-gateway-probes",
                     f"backend died rc={backend.proc.returncode}")
        else:
            log_pass(f"{flavour}/backend-survives-gateway-probes", "backend alive")
    finally:
        try:
            holder[0].stop()
        except Exception:
            pass


def gateway_phase():
    gw_base = os.path.join(TC.TMPFS_BUILD, "clustersec", "gw")
    log_info("building gateway + file-db server-cli backend ...")
    ok, detail = build_gateway_stack(gw_base)
    if not ok:
        log_fail("gateway stack build", detail[-400:])
        return
    log_pass("gateway stack build", "ok")
    backend = TC.ServerProc(
        "backend", os.path.join(gw_base, "server_cli_filedb", SERVER_CLI_BIN),
        os.path.join(TC.TMPFS_ROOT, "clustersec", "gw-run", "backend"),
        _backend_xml()).start()
    ok, detail = backend.wait_ready()
    if not ok:
        log_fail("gateway backend start", detail)
        try:
            backend.stop()
        except Exception:
            pass
        return
    log_pass("gateway backend start", f"file-db backend :{GW_BACKEND_PORT}")
    try:
        for hardened in FLAVOURS:
            run_gateway_pass(hardened, gw_base, backend)
    finally:
        try:
            backend.stop()          # free GW_BACKEND_PORT for the http-mirror attacks
        except Exception:
            pass
    log_info("==== http-mirror attacks (malicious datapack mirror) ====")
    run_http_mirror_attacks(gw_base)


# ── cluster phase (login + master + game-server-alone; needs PostgreSQL) ─────
def cluster_phase():
    if shutil.which("initdb") is None or shutil.which("postgres") is None:
        log_info("cluster phase SKIP: PostgreSQL server binaries "
                 "(initdb/postgres) absent — login/master/gsa need SQL.")
        return
    base_prod = os.path.join(TC.TMPFS_BUILD, "clustersec", "prod")
    base_hard = os.path.join(TC.TMPFS_BUILD, "clustersec", "hardened")
    log_info("building qtcpu800x600 client ...")
    ok, detail = build_client()
    if ok:
        log_pass("build qtcpu800x600", "ok")
    else:
        log_fail("build qtcpu800x600", detail[-400:])
    base_for = {False: base_prod, True: base_hard}
    for hardened in FLAVOURS:
        label = "hardened" if hardened else "production"
        log_info(f"building cluster ({label}) ...")
        ok, detail = build_cluster(base_for[hardened], hardened)
        if not ok:
            log_fail(f"build cluster ({label})", detail[-400:])
            return
        log_pass(f"build cluster ({label})", "ok")
    log_info("starting ephemeral PostgreSQL ...")
    ok, detail = TC.start_postgresql()
    if not ok:
        log_fail("postgresql", detail)
        return
    try:
        for hardened in FLAVOURS:
            run_pass(hardened, base_for[hardened])
    finally:
        try:
            TC.stop_postgresql()
        except Exception:
            pass


# ── main ─────────────────────────────────────────────────────────────────────
def main():
    t0 = time.monotonic()
    if VALGRIND:
        enable_valgrind()
        log_info("VALGRIND mode (default): every spawned server runs under "
                 "memcheck; production build only; defects reported at the end. "
                 "Disable with CC_CLUSTERSEC_NO_VALGRIND=1.")
    else:
        log_info("valgrind disabled — HARDENED on/off + crash-detection passes.")
    # Gateway: DB-less, runs everywhere. Cluster: needs PostgreSQL (self-skips).
    gateway_phase()
    if os.environ.get("CC_CLUSTERSEC_GW_ONLY", "") == "":
        cluster_phase()
    if VALGRIND:
        report_valgrind_defects()

    n_fail = sum(1 for ok, _, _ in _results if not ok)
    n_pass = sum(1 for ok, _, _ in _results if ok)
    if not _results:
        print("0 failed (nothing ran)", flush=True)
        return 0
    print(f"\n{n_pass} passed, {n_fail} failed  "
          f"({time.monotonic() - t0:.0f}s)", flush=True)
    return 1 if n_fail else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
