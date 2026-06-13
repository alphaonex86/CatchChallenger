#!/usr/bin/env python3
"""
testingmulti.py — CatchChallenger multiplayer interaction testing.

Steps:
  1. Build server-filedb, qtcpu800x600, qtopengl
  2. Setup server with CatchChallenger-datapack maincode=test
  3. Start server
  4. Start both clients in parallel with different character names
     and --closewhenonmapafter=10
  5. Check each client sees the other player (insert/move)
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
import process_helpers
import wall_cap
wall_cap.arm()
import cleanup_helpers
sys.dont_write_bytecode = True


import os, sys, signal, subprocess, threading, multiprocessing, json, shutil, re, time
import diagnostic
import build_paths
from cmd_helpers import (clamp_local, assert_port_or_fail,
                         assert_port_or_fail_with_remotes)
from remote_build import all_enabled_exec_nodes

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT       = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QMAKE      = _config["qmake"]
NPROC      = str(multiprocessing.cpu_count())

DATAPACK_SRC       = _config["paths"]["datapacks"][0]
MAINCODE           = "test"

SERVER_FILEDB_PRO  = os.path.join(ROOT, "server/cli/catchchallenger-server-filedb.pro")
SERVER_REF_BUILD   = build_paths.build_path("server/cli/build/catchchallenger-server-filedb-llvm-Debug")
SERVER_BUILD       = build_paths.build_path("server/cli/build/testing-multi" + _DIAG_SUFFIX)
SERVER_BIN_NAME    = "catchchallenger-server-cli"

CLIENT_CPU_PRO     = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_CPU_BUILD   = build_paths.build_path("client/qtcpu800x600/build/testing-multi-cpu" + _DIAG_SUFFIX)
CLIENT_CPU_BIN     = "catchchallenger"

CLIENT_GL_PRO      = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")
CLIENT_GL_BUILD    = build_paths.build_path("client/qtopengl/build/testing-multi-gl" + _DIAG_SUFFIX)
CLIENT_GL_BIN      = "catchchallenger"

# Tear down owned build dirs at script exit (see cleanup_helpers).
cleanup_helpers.register_build_dir(SERVER_BUILD)
cleanup_helpers.register_build_dir(CLIENT_CPU_BUILD)
cleanup_helpers.register_build_dir(CLIENT_GL_BUILD)

SERVER_HOST = _config["server_host"]
SERVER_PORT = str(_config["server_port"])

CLIENT_DATAPACK_CACHE = os.path.expanduser(_config["paths"]["client_datapack_cache"])
CLIENT_GL_DATAPACK_CACHE = os.path.expanduser("~/.local/share/CatchChallenger/client/datapack")
SAVEGAME_CPU = os.path.expanduser(_config["paths"]["savegame_cpu"])
SAVEGAME_GL  = os.path.expanduser(_config["paths"]["savegame_gl"])

CHAR_A = "PlayerA"
CHAR_B = "PlayerB"

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
CLIENT_TIMEOUT       = 60

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
_last_log_time = [time.monotonic()]
total_expected = [0]
server_proc = None

SCRIPT_NAME = os.path.basename(__file__)
from test_config import FAILED_JSON


import failed_cases as _fc
import phase_timer


def load_failed_cases():
    """Load failed cases for this script.  See failed_cases.load_names()."""
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    """Persist failures (with detail strings) under SCRIPT_NAME."""
    failed = []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            failed.append((name, d))
    _fc.save(SCRIPT_NAME, failed)


def log_info(msg):
    print(f"{phase_timer.t()} {C_CYAN}[INFO]{C_RESET} {msg}")
def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)
def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)

def clean_build_artifacts(build_dir):
    if not os.path.isdir(build_dir):
        return
    import glob as _glob
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in _glob.glob(os.path.join(build_dir, pattern)):
            os.remove(f)

def set_map_visibility_minimize(build_dir, value):
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'<minimize\s+value="[^"]*"',
                     f'<minimize value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)
    log_info(f'server-properties.xml minimize="{value}"')

def set_http_datapack_mirror(build_dir, value):
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'httpDatapackMirror\s+value="[^"]*"',
                     f'httpDatapackMirror value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)
    log_info(f'server-properties.xml httpDatapackMirror="{value}"')

def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"

def build_project(pro_file, build_dir, label):
    """Phase 5 (qmake -> CMake) port via cmake_helpers."""
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )

def setup_server_datapack(build_dir, datapack_src, maincode):
    import datapack_stage as _ds
    staged = _ds.staged_local(datapack_src)
    dst = os.path.join(build_dir, "datapack")
    if os.path.islink(dst) or os.path.isfile(dst):
        os.remove(dst)
    elif os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(build_dir, exist_ok=True)
    os.symlink(staged, dst)
    log_info(f"symlinked datapack {dst} -> {staged}")
    # set mainDatapackCode
    src_xml = os.path.join(SERVER_REF_BUILD, "server-properties.xml")
    dst_xml = os.path.join(build_dir, "server-properties.xml")
    if os.path.islink(dst_xml):
        os.remove(dst_xml)
    if os.path.isfile(src_xml):
        with open(src_xml, "r") as f:
            content = f.read()
        content = re.sub(r'mainDatapackCode\s+value="[^"]*"',
                         f'mainDatapackCode value="{maincode}"', content)
        with open(dst_xml, "w") as f:
            f.write(content)
        log_info(f"server-properties.xml mainDatapackCode={maincode}")

_TEST_SERVER_PROPERTIES_SEED = """<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <automatic_account_creation value="true"/>
    <master>
        <external-server-port value="{port}"/>
    </master>
</configuration>
"""


def _force_test_server_settings(build_dir):
    """Flip server-properties.xml so PlayerA / PlayerB --autologin works:
      * automatic_account_creation=true (default false rejects the
        implicit account-create from autologin).
      * server-port pinned to SERVER_PORT (server picks a RANDOM port
        on first init via NormalServerGlobal::checkSettingsFile, which
        defeats both the test's --port arg and the TCP probe).

    When server-properties.xml doesn't exist yet (first run on a fresh
    build dir), seed it with these two settings — checkSettingsFile()
    fills in everything else and respects what we pre-set."""
    xml = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml):
        os.makedirs(build_dir, exist_ok=True)
        with open(xml, "w") as f:
            f.write(_TEST_SERVER_PROPERTIES_SEED.format(port=SERVER_PORT))
        return
    if os.path.islink(xml):
        target = os.path.realpath(xml)
        os.remove(xml)
        shutil.copy2(target, xml)
    with open(xml, "r") as f:
        content = f.read()
    content = re.sub(r'automatic_account_creation\s+value="[^"]*"',
                     'automatic_account_creation value="true"', content)
    content = re.sub(r'(?<!external-)server-port\s+value="[^"]*"',
                     f'server-port value="{SERVER_PORT}"', content)
    content = re.sub(r'external-server-port\s+value="[^"]*"',
                     f'external-server-port value="{SERVER_PORT}"', content)
    with open(xml, "w") as f:
        f.write(content)


def start_server(build_dir, bin_name=SERVER_BIN_NAME):
    global server_proc
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("start server", f"binary not found: {binary}")
        return None
    _force_test_server_settings(build_dir)
    log_info(f"starting server: {binary}")
    env = os.environ.copy()
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    wrapper = diagnostic.runtime_wrapper(DIAG)
    srv_args = NICE_PREFIX + wrapper + [binary]
    diagnostic.record_cmd(srv_args, build_dir)
    server_proc = subprocess.Popen(
        srv_args, cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    ready = threading.Event()
    output_lines = []
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if "correctly bind:" in line:
                ready.set()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if ready.wait(timeout=diagnostic.scale_timeout(DIAG, SERVER_READY_TIMEOUT)):
        log_pass("server start", "correctly bind: detected")
        return server_proc
    else:
        log_fail("server start", "timeout waiting for 'correctly bind:'")
        for l in output_lines[-30:]:
            print(f"  | {l}")
        stop_server()
        return None

def stop_server():
    global server_proc
    if server_proc is None:
        return
    log_info("stopping server")
    try: os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError: pass
    try: server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        server_proc.wait(timeout=5)
    server_proc = None

def run_client_async(build_dir, bin_name, args, env):
    """Run client in background under gdb, return (proc, threading.Event for done, output list)."""
    binary = os.path.join(build_dir, bin_name)
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    wrapper = diagnostic.runtime_wrapper(DIAG)
    if wrapper:
        # gdb + valgrind don't compose well — run under valgrind directly.
        gdb_args = wrapper + [binary] + args
    elif shutil.which("gdb") is not None:
        gdb_args = ["gdb", "-batch",
                    "-ex", "set breakpoint pending on",
                    "-ex", "handle SIGPIPE nostop noprint pass",
                    "-ex", "break __throw_out_of_range",
                    "-ex", "run", "-ex", "bt", "-ex", "quit",
                    "--args", binary] + args
    else:
        gdb_args = [binary] + args
    cli_args = NICE_PREFIX + gdb_args
    diagnostic.record_cmd(cli_args, build_dir)
    proc = subprocess.Popen(
        cli_args, cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        env=env, preexec_fn=process_helpers.setsid_and_pdeathsig)
    output_lines = []
    done = threading.Event()
    # `early_fail` is set by the reader when the client either explicitly
    # refuses the connection or drops after a stateChanged(1) — both
    # always-FAIL conditions that let the harness skip the rest of the
    # timeout window. Returned alongside (proc, done, lines) so the
    # caller can read it after wait().
    early_fail = [None]
    seen_state1 = [False]

    def reader():
        for raw in iter(proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if early_fail[0] is None:
                if "Connection refused by the server" in line:
                    early_fail[0] = "Connection refused by the server"
                    done.set()
                elif "stateChanged(1)" in line:
                    seen_state1[0] = True
                elif seen_state1[0] and "stateChanged(0)" in line:
                    early_fail[0] = ("stateChanged(0) after stateChanged(1) — "
                                     "client connected then dropped before map")
                    done.set()
        done.set()

    t = threading.Thread(target=reader, daemon=True)
    t.start()
    return proc, done, output_lines, early_fail

def kill_client(proc):
    if proc is None:
        return
    try: os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except ProcessLookupError: pass
    try: proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        proc.wait(timeout=5)


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_server()
    sys.exit(1)

signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Multiplayer Interaction Testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete test/failed.json for full re-run)")
        return

    dp_name = os.path.basename(DATAPACK_SRC)
    log_info(f"datapack: {dp_name}, maincode: {MAINCODE}")

    MINIMIZE_MODES = ("network", "cpu")
    CLIENTS = (CLIENT_CPU_BUILD, CLIENT_GL_BUILD)
    CHECK_TYPES = ("map_displayed", "insert_player", "direction", "chat")
    # per mode: 1 server_start + len(CLIENTS) * len(CHECK_TYPES)
    total_expected[0] = 3 + len(MINIMIZE_MODES) * (1 + len(CLIENTS) * len(CHECK_TYPES))

    # ── 1. build ────────────────────────────────────────────────────
    print(f"\n{C_CYAN}--- Compilation ---{C_RESET}\n")

    if should_run("compile server-filedb", failed_cases):
        if not build_project(SERVER_FILEDB_PRO, SERVER_BUILD, "server-filedb"):
            summary()
            return
    elif not os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN_NAME)):
        log_fail("compile server-filedb", "binary not found from previous build")
        summary()
        return

    if should_run("compile qtcpu800x600", failed_cases):
        cpu_ok = build_project(CLIENT_CPU_PRO, CLIENT_CPU_BUILD, "qtcpu800x600")
    else:
        cpu_ok = os.path.isfile(os.path.join(CLIENT_CPU_BUILD, CLIENT_CPU_BIN))
    if should_run("compile qtopengl", failed_cases):
        gl_ok = build_project(CLIENT_GL_PRO, CLIENT_GL_BUILD, "qtopengl")
    else:
        gl_ok = os.path.isfile(os.path.join(CLIENT_GL_BUILD, CLIENT_GL_BIN))
    if not cpu_ok or not gl_ok:
        summary()
        return

    # ── 2. run multiplayer test with both minimize modes ──────────────
    for minimize_mode in MINIMIZE_MODES:
        run_multiplayer_test(dp_name, minimize_mode)

    # ── 3. single-client interaction run against the LOCAL server ─────
    # Reproduces the "kicked from server" report locally: one client connects
    # over a real TCP socket and runs the keyboard sign+door round-trip, which
    # probes the whole reachable map on every click. If that probe stalls the
    # Qt event loop (the old per-click canGoTo cerr flood), the server drops the
    # client mid-run. The embedded-solo --test-keyboard can't catch this — it
    # has no real keepalive — so this needs a real server.
    run_server_interaction_test(dp_name, failed_cases)

    # Drive a live client over its QLocalServer automation socket and replay the
    # inputs that used to crash the server (/trade <unknown>) and the client
    # (out-of-fight fight spam -> re-entrant use-after-free). Both are fixed.
    run_channel_crash_regression_test(dp_name, failed_cases)

    # Drive a live client over the same socket to leave a map by clicking a
    # "teleport on push" exit (regression for the can't-leave-the-gym bug).
    run_channel_teleport_push_test(dp_name, failed_cases)

    summary()


def run_server_interaction_test(dp_name, failed_cases):
    """One client vs the local server-filedb over real TCP, running the keyboard
    sign+door round-trip self-test. PASS only if it walks to a sign and opens it
    (Enter), closes (Escape), goes through a door and comes back — WITHOUT being
    dropped by the server. The heavy per-click map probe must stay event-loop
    friendly; the old canGoTo cerr flood stalled it and got the client kicked."""
    import tempfile
    case = "keyboard sign+door round-trip vs local server"
    if not should_run(case, failed_cases):
        return
    print(f"\n{C_CYAN}--- Server interaction (keyboard, vs local server-filedb): {dp_name} {MAINCODE} ---{C_RESET}\n")
    setup_server_datapack(SERVER_BUILD, DATAPACK_SRC, MAINCODE)
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
    cache_file = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache_file):
        os.remove(cache_file)
    set_http_datapack_mirror(SERVER_BUILD, "")
    set_map_visibility_minimize(SERVER_BUILD, "cpu")
    if start_server(SERVER_BUILD) is None:
        log_fail(case, "server did not start")
        return
    if not assert_port_or_fail(SERVER_HOST, int(SERVER_PORT), log_fail,
                               "interaction tcp probe"):
        stop_server()
        log_fail(case, "tcp probe failed")
        return
    tmpdir = tempfile.mkdtemp(prefix="cc-test-kb-")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["XDG_CONFIG_HOME"] = os.path.join(tmpdir, "config")
    env["XDG_DATA_HOME"] = os.path.join(tmpdir, "data")
    app_name = "CatchChallenger/client-qtcpu800x600"
    cache_name = f"argument-{SERVER_HOST}-{_config['server_port']}"
    cache_dir = os.path.join(tmpdir, "data", app_name, "datapack", cache_name)
    shutil.copytree(os.path.join(SERVER_BUILD, "datapack"), cache_dir,
                    ignore=shutil.ignore_patterns(".git"))
    conf_dir = os.path.join(tmpdir, "config", "CatchChallenger")
    os.makedirs(conf_dir, exist_ok=True)
    with open(os.path.join(conf_dir, "client-qtcpu800x600.conf"), "w") as f:
        f.write("[General]\nkey=testKBKey\n")
    args = ["--host", SERVER_HOST, "--port", SERVER_PORT,
            "--autologin", "--character", CHAR_A, "--test-keyboard"]
    log_info("connecting client (qtcpu800x600) for the keyboard interaction run")
    proc, done, out, fail = run_client_async(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, args, env)
    done.wait(timeout=diagnostic.scale_timeout(DIAG, 150))
    if proc.poll() is None:
        kill_client(proc)
    text = "\n".join(out)
    if "[KEYBOARDTEST] PASS came back" in text:
        log_pass(case, "arrow-walk sign+Enter+Escape, door round-trip, no kick")
    elif fail[0] is not None:
        log_fail(case, f"client dropped by server mid-interaction: {fail[0]}")
        for line in out[-25:]:
            print(f"  | {line}")
    elif "Connection closed by the server" in text:
        log_fail(case, "kicked: 'Connection closed by the server' during interaction")
        for line in out[-25:]:
            print(f"  | {line}")
    else:
        log_fail(case, "no '[KEYBOARDTEST] PASS came back' marker")
        for line in out[-25:]:
            print(f"  | {line}")
    stop_server()


def _channel_connect(candidate_paths, settle=4.0):
    """Try each AF_UNIX candidate path; return the first socket that connects and
    answers GETSTATE with 'STATE'. None if none work within `settle` seconds. The
    client's QLocalServer socket lives at <TMPDIR>/CatchChallenger-Client-<n>-<uid>;
    we isolate TMPDIR per-test so slot 0 is ours, but probe a few slots + /tmp as a
    fallback in case TMPDIR is not honoured by QDir::tempPath()."""
    import socket as _s, time as _t
    deadline = _t.time() + settle
    while _t.time() < deadline:
        for p in candidate_paths:
            if not os.path.exists(p):
                continue
            sk = None
            try:
                sk = _s.socket(_s.AF_UNIX, _s.SOCK_STREAM)
                sk.settimeout(1.5)
                sk.connect(p)
                sk.sendall(b"GETSTATE\n")
                _t.sleep(0.3)
                if "STATE" in sk.recv(4096).decode(errors="replace"):
                    return sk
                sk.close()
            except Exception:
                if sk is not None:
                    try: sk.close()
                    except Exception: pass
        _t.sleep(0.3)
    return None


def _channel_cmd(sk, line, wait=0.5):
    """Send one newline-terminated command, return the (stripped) reply text."""
    import time as _t
    sk.sendall((line + "\n").encode())
    _t.sleep(wait)
    buf = b""
    try:
        while True:
            d = sk.recv(4096)
            if not d:
                break
            buf += d
            if b"\n" in buf:
                break
    except Exception:
        pass
    return buf.decode(errors="replace").strip()


def _channel_state(sk):
    """GETSTATE -> (map_id, x, y) as ints, or None if the reply isn't a STATE line."""
    import re
    m = re.search(r"map=(\d+)\s+x=(\d+)\s+y=(\d+)", _channel_cmd(sk, "GETSTATE"))
    return (int(m.group(1)), int(m.group(2)), int(m.group(3))) if m else None


def _channel_wait_map(sk, want_equal=None, change_from=None, timeout=20.0):
    """Poll GETSTATE until the player's map id equals `want_equal` (or differs from
    `change_from`). Return the final (map,x,y) on success, None on timeout."""
    import time as _t
    deadline = _t.time() + timeout
    while _t.time() < deadline:
        st = _channel_state(sk)
        if st is not None:
            if want_equal is not None and st[0] == want_equal:
                return st
            if change_from is not None and st[0] != change_from:
                return st
        _t.sleep(0.4)
    return None


def run_channel_teleport_push_test(dp_name, failed_cases):
    """Regression guard, driven over the QLocalServer channel (no in-client test
    scaffolding): leaving a map by clicking a "teleport on push" tile must work.
    The gym "to city" exit sits on a COLLISION tile (you LEAVE by pushing into it);
    canGoTo() used to reject the step because the tile is blocked, so neither
    keyboard nor mouse could leave. Enter the gym via the city door, then click the
    exit at (9,20) and assert the player returns to the city."""
    import tempfile, time, re
    case = "channel: leave a map by clicking a teleport-on-push exit (gym)"
    if not should_run(case, failed_cases):
        return
    print(f"\n{C_CYAN}--- Channel teleport-on-push leave (gym, vs local server): {dp_name} {MAINCODE} ---{C_RESET}\n")
    setup_server_datapack(SERVER_BUILD, DATAPACK_SRC, MAINCODE)
    # The SERVER_REF build has no server-properties.xml so setup_server_datapack
    # can't stamp the maincode; the gym only exists under "test", so force it (the
    # fresh character then spawns on the test city, next to the gym door at 21,25).
    sp_xml = os.path.join(SERVER_BUILD, "server-properties.xml")
    if os.path.isfile(sp_xml):
        with open(sp_xml) as f:
            _sp = f.read()
        _sp = re.sub(r'mainDatapackCode\s+value="[^"]*"', 'mainDatapackCode value="test"', _sp)
        with open(sp_xml, "w") as f:
            f.write(_sp)
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
    cache_file = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache_file):
        os.remove(cache_file)
    set_http_datapack_mirror(SERVER_BUILD, "")
    set_map_visibility_minimize(SERVER_BUILD, "cpu")
    srv = start_server(SERVER_BUILD)
    if srv is None:
        log_fail(case, "server did not start")
        return
    if not assert_port_or_fail(SERVER_HOST, int(SERVER_PORT), log_fail, "tp-push tcp probe"):
        stop_server()
        log_fail(case, "tcp probe failed")
        return
    tmpdir = tempfile.mkdtemp(prefix="cc-test-tppush-")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["XDG_CONFIG_HOME"] = os.path.join(tmpdir, "config")
    env["XDG_DATA_HOME"] = os.path.join(tmpdir, "data")
    env["TMPDIR"] = tmpdir
    app_name = "CatchChallenger/client-qtcpu800x600"
    cache_name = f"argument-{SERVER_HOST}-{_config['server_port']}"
    cache_dir = os.path.join(tmpdir, "data", app_name, "datapack", cache_name)
    shutil.copytree(os.path.join(SERVER_BUILD, "datapack"), cache_dir,
                    ignore=shutil.ignore_patterns(".git"))
    conf_dir = os.path.join(tmpdir, "config", "CatchChallenger")
    os.makedirs(conf_dir, exist_ok=True)
    with open(os.path.join(conf_dir, "client-qtcpu800x600.conf"), "w") as f:
        f.write("[General]\nkey=testTpPushKey\n")
    args = ["--host", SERVER_HOST, "--port", SERVER_PORT,
            "--autologin", "--character", CHAR_A]
    log_info("connecting client (qtcpu800x600) for the teleport-on-push channel run")
    proc, done, out, fail = run_client_async(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, args, env)
    on_map = False
    deadline = time.time() + diagnostic.scale_timeout(DIAG, 150)
    while time.time() < deadline:
        if any("MapVisualiserPlayer::mapDisplayedSlot()" in l for l in out):
            on_map = True
            break
        if done.is_set():
            break
        time.sleep(0.5)
    if not on_map:
        kill_client(proc)
        stop_server()
        log_fail(case, f"client never reached the map; early_fail={fail[0]}")
        return
    uid = os.getuid()
    candidates = ([os.path.join(tmpdir, f"CatchChallenger-Client-{n}-{uid}") for n in range(4)] +
                  [os.path.join("/tmp", f"CatchChallenger-Client-{n}-{uid}") for n in range(4)])
    sk = _channel_connect(candidates)
    if sk is None:
        kill_client(proc)
        stop_server()
        log_fail(case, "could not connect to the client's QLocalServer automation socket")
        return
    result = None
    try:
        spawn = _channel_state(sk)
        if spawn is None:
            result = ("fail", "GETSTATE returned no STATE on the spawn map")
        else:
            city_map = spawn[0]
            log_info(f"spawn (city) map id={city_map} at ({spawn[1]},{spawn[2]})")
            _channel_cmd(sk, "CLICKTILE 21 25")           # the gym door on the city
            gym = _channel_wait_map(sk, change_from=city_map, timeout=25)
            if gym is None:
                result = ("fail", "could not enter the gym (clicking the city gym door did not change map)")
            else:
                log_info(f"entered the gym: map id={gym[0]} at ({gym[1]},{gym[2]})")
                # The gym interior is gated, so we sit at the entrance (9,19), next to
                # the "to city" teleport-on-push exit (9,20). Click it to leave.
                log_info("clicking the gym 'to city' teleport-on-push exit (9,20)")
                _channel_cmd(sk, "CLICKTILE 9 20")
                back = _channel_wait_map(sk, want_equal=city_map, timeout=25)
                if back is not None:
                    result = ("pass", f"clicking the push-exit returned to the city (map {city_map}) at ({back[1]},{back[2]})")
                else:
                    result = ("fail", "clicking the gym 'teleport on push' exit did NOT leave the gym")
    finally:
        try: sk.close()
        except Exception: pass
        if proc.poll() is None:
            kill_client(proc)
        stop_server()
    if result[0] == "pass":
        log_pass(case, result[1])
    else:
        log_fail(case, result[1])
        for line in out[-25:]:
            print(f"  | {line}")


def run_channel_crash_regression_test(dp_name, failed_cases):
    """Drive a live client over its QLocalServer automation socket against the
    local server-filedb, replaying the exact inputs that used to crash:
      - "/trade <unknown>"      -> server ClientList::isNull() out-of-range abort
      - out-of-fight fight spam -> client re-entrant use-after-free (freed client)
    Both are fixed; this is the regression guard. PASS iff the server stays up
    (process alive) after the bad /trade AND the client never SIGSEGVs / throws
    std::out_of_range under gdb while being driven."""
    import tempfile, time
    case = "QLocalServer channel crash-regression vs local server"
    if not should_run(case, failed_cases):
        return
    print(f"\n{C_CYAN}--- Channel crash-regression (vs local server-filedb): {dp_name} {MAINCODE} ---{C_RESET}\n")
    setup_server_datapack(SERVER_BUILD, DATAPACK_SRC, MAINCODE)
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
    cache_file = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache_file):
        os.remove(cache_file)
    set_http_datapack_mirror(SERVER_BUILD, "")
    set_map_visibility_minimize(SERVER_BUILD, "cpu")
    srv = start_server(SERVER_BUILD)
    if srv is None:
        log_fail(case, "server did not start")
        return
    if not assert_port_or_fail(SERVER_HOST, int(SERVER_PORT), log_fail,
                               "channel-crash tcp probe"):
        stop_server()
        log_fail(case, "tcp probe failed")
        return
    tmpdir = tempfile.mkdtemp(prefix="cc-test-chan-")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["XDG_CONFIG_HOME"] = os.path.join(tmpdir, "config")
    env["XDG_DATA_HOME"] = os.path.join(tmpdir, "data")
    # isolate the QLocalServer socket so a parallel client can't steal slot 0
    env["TMPDIR"] = tmpdir
    app_name = "CatchChallenger/client-qtcpu800x600"
    cache_name = f"argument-{SERVER_HOST}-{_config['server_port']}"
    cache_dir = os.path.join(tmpdir, "data", app_name, "datapack", cache_name)
    shutil.copytree(os.path.join(SERVER_BUILD, "datapack"), cache_dir,
                    ignore=shutil.ignore_patterns(".git"))
    conf_dir = os.path.join(tmpdir, "config", "CatchChallenger")
    os.makedirs(conf_dir, exist_ok=True)
    with open(os.path.join(conf_dir, "client-qtcpu800x600.conf"), "w") as f:
        f.write("[General]\nkey=testChanKey\n")
    # NO --test flag: the client stays on the map so we can drive it via the socket
    args = ["--host", SERVER_HOST, "--port", SERVER_PORT,
            "--autologin", "--character", CHAR_A]
    log_info("connecting client (qtcpu800x600) for the channel crash-regression run")
    proc, done, out, fail = run_client_async(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, args, env)
    # wait for the client to reach the map
    on_map = False
    deadline = time.time() + diagnostic.scale_timeout(DIAG, 150)
    while time.time() < deadline:
        if any("MapVisualiserPlayer::mapDisplayedSlot()" in l for l in out):
            on_map = True
            break
        if done.is_set():
            break
        time.sleep(0.5)
    if not on_map:
        kill_client(proc)
        stop_server()
        log_fail(case, f"client never reached the map; early_fail={fail[0]}")
        for line in out[-25:]:
            print(f"  | {line}")
        return
    uid = os.getuid()
    candidates = ([os.path.join(tmpdir, f"CatchChallenger-Client-{n}-{uid}") for n in range(4)] +
                  [os.path.join("/tmp", f"CatchChallenger-Client-{n}-{uid}") for n in range(4)])
    sk = _channel_connect(candidates)
    if sk is None:
        kill_client(proc)
        stop_server()
        log_fail(case, "could not connect to the client's QLocalServer automation socket")
        return
    # --- sequence 1: the server /trade crash (must NOT abort the server) ---
    _channel_cmd(sk, "GETSTATE")
    log_info('sending "/trade <unknown>" (used to abort the server)')
    _channel_cmd(sk, "TRADEREQUEST ZzNoSuchPlayer123")
    time.sleep(1.5)
    server_alive = (srv.poll() is None)
    # --- sequence 2: out-of-fight fight spam (must NOT crash the client) ---
    log_info("spamming fight actions out of fight (used to use-after-free the client)")
    for c in ["FIGHTSKILL 5", "FIGHTCHANGEMONSTER 0", "FIGHTITEM 1 0",
              "FIGHTITEM 7", "FIGHTESCAPE", "FIGHTACCEPT"]:
        _channel_cmd(sk, c, wait=0.3)
    time.sleep(1.5)
    try: sk.close()
    except Exception: pass
    # give gdb a moment to print any crash backtrace, then reap the client
    time.sleep(1.0)
    if proc.poll() is None:
        kill_client(proc)
    text = "\n".join(out)
    # Match only ACTUAL gdb crash events. run_client_async runs the client under
    # `gdb ... -ex 'break __throw_out_of_range'`, so gdb echoes a breakpoint-setup
    # line containing "__throw_out_of_range" into the captured output even on a
    # clean run — a naive substring match on that string false-positives. A real
    # crash shows "received signal SIG..." (SIGSEGV/SIGABRT) or "hit Breakpoint"
    # (the std::out_of_range breakpoint actually fired), neither of which appears
    # in gdb's setup output.
    crashed = any(m in text for m in (
        "received signal SIGSEGV", "received signal SIGABRT", "hit Breakpoint"))
    stop_server()
    if crashed:
        log_fail(case, "client crashed under gdb during the abusive channel sequence")
        for line in out[-30:]:
            print(f"  | {line}")
    elif not server_alive:
        log_fail(case, "server died on '/trade <unknown>' (isNull out-of-range abort)")
    else:
        log_pass(case, "server survived /trade<unknown>; client survived out-of-fight fight spam")


def run_multiplayer_test(dp_name, minimize_mode):
    import tempfile, time

    print(f"\n{C_CYAN}--- Multiplayer test: minimize={minimize_mode} ({dp_name} {MAINCODE}) ---{C_RESET}\n")

    setup_server_datapack(SERVER_BUILD, DATAPACK_SRC, MAINCODE)
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
        log_info("cleared database/")
    cache_file = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache_file):
        os.remove(cache_file)
        log_info("removed datapack-cache.bin")
    set_http_datapack_mirror(SERVER_BUILD, "")
    set_map_visibility_minimize(SERVER_BUILD, minimize_mode)

    srv = start_server(SERVER_BUILD)
    if srv is None:
        return

    # Local-only TCP probe. The "correctly bind: detected" line only
    # proves listen() ran; this catches the rare case where bind
    # silently fell through. Remote probes would always FAIL by design:
    # the firewall blocks NEW connections from exec_nodes back to the
    # test box (see CLAUDE.md "Network …"), and both clients here run
    # on the test box anyway.
    if not assert_port_or_fail(SERVER_HOST, int(SERVER_PORT),
                               log_fail, f"[{minimize_mode}] tcp probe"):
        stop_server()
        return

    tag = f"[{minimize_mode}]"

    tmpdir_a = tempfile.mkdtemp(prefix="cc-test-a-")
    tmpdir_b = tempfile.mkdtemp(prefix="cc-test-b-")

    env_a = os.environ.copy()
    env_a["QT_QPA_PLATFORM"] = "offscreen"
    env_a["XDG_CONFIG_HOME"] = os.path.join(tmpdir_a, "config")
    env_a["XDG_DATA_HOME"] = os.path.join(tmpdir_a, "data")

    env_b = os.environ.copy()
    env_b["QT_QPA_PLATFORM"] = "offscreen"
    env_b["XDG_CONFIG_HOME"] = os.path.join(tmpdir_b, "config")
    env_b["XDG_DATA_HOME"] = os.path.join(tmpdir_b, "data")

    server_datapack = os.path.join(SERVER_BUILD, "datapack")
    cache_name = f"argument-{SERVER_HOST}-{_config['server_port']}"
    for tmpdir, app_name, login in ((tmpdir_a, "CatchChallenger/client-qtcpu800x600", "testA"),
                                     (tmpdir_b, "CatchChallenger/client", "testB")):
        cache_dir = os.path.join(tmpdir, "data", app_name, "datapack", cache_name)
        shutil.copytree(server_datapack, cache_dir,
                        ignore=shutil.ignore_patterns(".git"))
        conf_dir = os.path.join(tmpdir, "config", "CatchChallenger")
        os.makedirs(conf_dir, exist_ok=True)
        conf_name = app_name.split("/")[-1] + ".conf"
        conf_path = os.path.join(conf_dir, conf_name)
        with open(conf_path, "w") as f:
            f.write(f"[General]\nkey={login}Key\n")
        log_info(f"pre-populated {cache_dir} login={login}")

    client_args_base = ["--host", SERVER_HOST, "--port", SERVER_PORT,
                        "--autologin", "--closewhenonmapafter=30"]
    args_a = client_args_base + ["--character", CHAR_A]
    args_b = client_args_base + ["--character", CHAR_B]

    log_info(f"starting {CHAR_A} (qtcpu800x600)")
    proc_a, done_a, out_a, fail_a = run_client_async(
        CLIENT_CPU_BUILD, CLIENT_CPU_BIN, args_a, env_a)
    time.sleep(5)
    log_info(f"starting {CHAR_B} (qtopengl)")
    proc_b, done_b, out_b, fail_b = run_client_async(
        CLIENT_GL_BUILD, CLIENT_GL_BIN, args_b, env_b)

    done_a.wait(timeout=diagnostic.scale_timeout(DIAG, CLIENT_TIMEOUT))
    done_b.wait(timeout=diagnostic.scale_timeout(DIAG, CLIENT_TIMEOUT))
    if proc_a.poll() is None:
        log_info(f"{CHAR_A} still running, killing")
        kill_client(proc_a)
    if proc_b.poll() is None:
        log_info(f"{CHAR_B} still running, killing")
        kill_client(proc_b)
    # Surface early-fail signals (Connection refused / disconnect-after-
    # connect) BEFORE the marker checks. The reader thread already killed
    # the proc on detection — log here so the operator sees why.
    if fail_a[0] is not None:
        log_info(f"{CHAR_A} early fail: {fail_a[0]}")
    if fail_b[0] is not None:
        log_info(f"{CHAR_B} early fail: {fail_b[0]}")

    text_a = "\n".join(out_a)
    text_b = "\n".join(out_b)

    # ── check results ────────────────────────────────────────────
    print(f"\n{C_CYAN}--- Results: minimize={minimize_mode} ---{C_RESET}\n")

    map_marker = "MapVisualiserPlayer::mapDisplayedSlot()"
    insert_marker = "MapControllerMP::insert_player() id="
    direction_marker = "closewhenonmapafter direction"
    chat_marker = "Chat::new_chat_text() pseudo="

    if map_marker in text_a:
        log_pass(f"{tag} {CHAR_A}: map displayed")
    else:
        log_fail(f"{tag} {CHAR_A}: map displayed", "mapDisplayedSlot not found")
        for line in out_a[-20:]:
            print(f"  | {line}")

    if map_marker in text_b:
        log_pass(f"{tag} {CHAR_B}: map displayed")
    else:
        log_fail(f"{tag} {CHAR_B}: map displayed", "mapDisplayedSlot not found")
        for line in out_b[-20:]:
            print(f"  | {line}")

    if insert_marker in text_a and f"pseudo={CHAR_B}" in text_a:
        log_pass(f"{tag} {CHAR_A}: sees {CHAR_B} inserted")
    else:
        log_fail(f"{tag} {CHAR_A}: sees {CHAR_B} inserted",
                 f"insert_player with pseudo={CHAR_B} not found")
        for line in out_a:
            if "insert_player" in line:
                print(f"  | {line}")

    if insert_marker in text_b and f"pseudo={CHAR_A}" in text_b:
        log_pass(f"{tag} {CHAR_B}: sees {CHAR_A} inserted")
    else:
        log_fail(f"{tag} {CHAR_B}: sees {CHAR_A} inserted",
                 f"insert_player with pseudo={CHAR_A} not found")
        for line in out_b:
            if "insert_player" in line:
                print(f"  | {line}")

    if direction_marker in text_a:
        log_pass(f"{tag} {CHAR_A}: sends direction changes")
    else:
        log_fail(f"{tag} {CHAR_A}: sends direction changes",
                 "direction toggle not found")

    if direction_marker in text_b:
        log_pass(f"{tag} {CHAR_B}: sends direction changes")
    else:
        log_fail(f"{tag} {CHAR_B}: sends direction changes",
                 "direction toggle not found")

    chat_from_b = (f"{chat_marker}{CHAR_B}" in text_a or
                   f"{CHAR_B}: hello" in text_a or
                   f"] {CHAR_B}: hello" in text_a)
    if chat_from_b:
        log_pass(f"{tag} {CHAR_A}: receives chat from {CHAR_B}")
    else:
        log_fail(f"{tag} {CHAR_A}: receives chat from {CHAR_B}",
                 f"chat from {CHAR_B} not found")

    chat_from_a = (f"{chat_marker}{CHAR_A}" in text_b or
                   f"{CHAR_A}: hello" in text_b or
                   f"] {CHAR_A}: hello" in text_b)
    if chat_from_a:
        log_pass(f"{tag} {CHAR_B}: receives chat from {CHAR_A}")
    else:
        log_fail(f"{tag} {CHAR_B}: receives chat from {CHAR_A}",
                 f"chat from {CHAR_A} not found")

    stop_server()
    for d in (tmpdir_a, tmpdir_b):
        if os.path.isdir(d):
            shutil.rmtree(d, ignore_errors=True)


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for name, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    save_failed_cases()
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
