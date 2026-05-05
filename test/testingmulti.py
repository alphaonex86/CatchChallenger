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

SERVER_FILEDB_PRO  = os.path.join(ROOT, "server/epoll/catchchallenger-server-filedb.pro")
SERVER_REF_BUILD   = build_paths.build_path("server/epoll/build/catchchallenger-server-filedb-llvm-Debug")
SERVER_BUILD       = build_paths.build_path("server/epoll/build/testing-multi" + _DIAG_SUFFIX)
SERVER_BIN_NAME    = "catchchallenger-server-cli-epoll"

CLIENT_CPU_PRO     = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_CPU_BUILD   = build_paths.build_path("client/qtcpu800x600/build/testing-multi-cpu" + _DIAG_SUFFIX)
CLIENT_CPU_BIN     = "catchchallenger"

CLIENT_GL_PRO      = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")
CLIENT_GL_BUILD    = build_paths.build_path("client/qtopengl/build/testing-multi-gl" + _DIAG_SUFFIX)
CLIENT_GL_BIN      = "catchchallenger"

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


def load_failed_cases():
    """Load failed cases for this script from failed.json.
    Returns None (run everything), [] (skip all), or [names] (resume only those)."""
    if not os.path.isfile(FAILED_JSON):
        return None
    try:
        with open(FAILED_JSON, "r") as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError):
        return None
    if SCRIPT_NAME not in data:
        return None
    return data[SCRIPT_NAME]


def should_run(test_name, failed_cases):
    """Check if test should run. None=run all, []=skip all, [..]=only listed."""
    if failed_cases is None:
        return True
    idx = 0
    while idx < len(failed_cases):
        if failed_cases[idx] == test_name:
            return True
        idx += 1
    return False


def save_failed_cases():
    """Update failed.json with current failures for this script."""
    data = {}
    if os.path.isfile(FAILED_JSON):
        try:
            with open(FAILED_JSON, "r") as f:
                data = json.load(f)
        except (json.JSONDecodeError, IOError):
            data = {}
    failed = []
    idx = 0
    while idx < len(results):
        if not results[idx][1]:
            failed.append(results[idx][0])
        idx += 1
    data[SCRIPT_NAME] = failed
    with open(FAILED_JSON, "w") as f:
        json.dump(data, f, indent=2)


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")
def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
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
        preexec_fn=os.setsid)
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
        env=env, preexec_fn=os.setsid)
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

    summary()


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
