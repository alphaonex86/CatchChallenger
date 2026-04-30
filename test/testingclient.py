#!/usr/bin/env python3
"""
testingclient.py — CatchChallenger client compilation, solo, and multiplayer tests.

Sections:
  1. Datapack: copy CatchChallenger-datapack to <binary>/datapack/internal/
  2. Compilation: build qtcpu800x600 and qtopengl
  3. Solo: remove savegames, play, close, come back
  4. Multiplayer: start server-filedb, test both clients connect
  5. Remote: build server-filedb on remote nodes (mips-lxc, x86-lxc, ...)
     and run local clients against them.

Windows (MXE+wine64), macOS (qemu VM via ssh) and Android (Qt-for-Android +
local emulator) cross-compile + run phases moved out into standalone scripts:
testingcompilationwindows.py, testingcompilationmac.py,
testingcompilationandroid.py — invoked separately by all.sh.
"""

import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re, time
from remote_build import (start_remote_builds, collect_remote_results,
                          REMOTE_SERVERS, REMOTE_NODES, node_runs, get_node,
                          setup_remote_server_runtime, start_remote_server,
                          stop_remote_server, get_remote_server_address,
                          cleanup_remote_node, run_remote_autosolo_phase,
                          count_remote_tests)
import diagnostic

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

DATAPACKS       = _config["paths"]["datapacks"]

SERVER_PRO      = os.path.join(ROOT, "server/epoll/catchchallenger-server-filedb.pro")
SERVER_BUILD    = os.path.join(ROOT, "server/epoll/build/testing-filedb" + _DIAG_SUFFIX)
SERVER_REF_BUILD= os.path.join(ROOT, "server/epoll/build/catchchallenger-server-filedb-llvm-Debug")
SERVER_BIN_NAME = "catchchallenger-server-cli-epoll"

CLIENT_CPU_PRO   = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_CPU_BUILD = os.path.join(ROOT, "client/qtcpu800x600/build/testing-cpu" + _DIAG_SUFFIX)
CLIENT_CPU_BIN   = "catchchallenger"

CLIENT_GL_PRO    = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")
CLIENT_GL_BUILD  = os.path.join(ROOT, "client/qtopengl/build/testing-gl" + _DIAG_SUFFIX)
CLIENT_GL_BIN    = "catchchallenger"

SAVEGAME_CPU = os.path.expanduser(_config["paths"]["savegame_cpu"])
SAVEGAME_GL  = os.path.expanduser(_config["paths"]["savegame_gl"])

SERVER_HOST  = _config["server_host"]
SERVER_PORT  = str(_config["server_port"])

# Host-visible IP this machine has on the LAN that the qemu mac VM and any
# other remote/VM client live on.  Used as --host by clients that run off-
# host and therefore can't reach SERVER_HOST=localhost.  Wine runs in-process
# and keeps using SERVER_HOST; mac/android multi runs use this.
SERVER_HOST_REMOTE = "192.168.158.10"

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
CLIENT_TIMEOUT       = 5
CLIENT_SOLO_TIMEOUT  = 30
BENCHMARK_MAX_MS     = 1000

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

# ── compilers and optional flag combinations ───────────────────────────────
COMPILERS = [
    ("gcc",   "linux-g++"),
    ("clang", "linux-clang"),
]

CXX_VERSIONS = ["c++11", "c++23"]

# CATCHCHALLENGER_NOAUDIO is commented out in qtcpu800x600.pro but the build
# requires it (Qt multimedia audio path has issues), so always add it as a
# base define.  The remaining flags are independently toggleable.
CPU_BASE_DEFINES = ["CATCHCHALLENGER_NOAUDIO"]
CPU_OPTIONAL_FLAGS = ["NOSINGLEPLAYER", "CATCHCHALLENGER_EXTRA_CHECK", "NOWEBSOCKET"]

# qtopengl default build has audio enabled (Qt multimedia works).
# Only CATCHCHALLENGER_EXTRA_CHECK and NOWEBSOCKET are safe independent
# toggles; NOSINGLEPLAYER/CATCHCHALLENGER_NOAUDIO/NOTCPSOCKET/NOTHREADS
# are WASM-only flags with interdependencies that break native builds.
GL_BASE_DEFINES = []
GL_OPTIONAL_FLAGS = ["NOSINGLEPLAYER", "CATCHCHALLENGER_NOAUDIO",
                     "CATCHCHALLENGER_EXTRA_CHECK", "NOWEBSOCKET"]

# Flags that are tested as standalone single-flag tests instead of being
# part of the powerset.  In multi-flag combinations:
#   ALWAYS_ON_IN_COMBOS  — always added
#   ALWAYS_OFF_IN_COMBOS — never added
ALWAYS_ON_IN_COMBOS  = ["CATCHCHALLENGER_EXTRA_CHECK"]
ALWAYS_OFF_IN_COMBOS = ["CATCHCHALLENGER_NOAUDIO", "NOWEBSOCKET"]

# Windows (MXE+wine64), macOS (qemu VM via ssh) and Android (Qt-for-Android +
# emulator) constants used to live here. They moved to the new standalone
# scripts: testingcompilationwindows.py, testingcompilationmac.py,
# testingcompilationandroid.py. all.sh runs the three after testingclient.py.

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
FAILED_JSON = "/mnt/data/perso/tmpfs/failed.json"


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

#log_pass / log_fail are called from worker threads (mac and android phases
#run in parallel), so the elapsed/counter/print sequence has to be serialised.
_log_lock = threading.Lock()

def log_pass(name, detail=""):
    with _log_lock:
        now = time.monotonic()
        elapsed = now - _last_log_time[0]
        _last_log_time[0] = now
        results.append((name, True, detail, elapsed))
        if len(results) > total_expected[0]:
            total_expected[0] = len(results)
        print(f"{C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")

def log_fail(name, detail=""):
    with _log_lock:
        now = time.monotonic()
        elapsed = now - _last_log_time[0]
        _last_log_time[0] = now
        results.append((name, False, detail, elapsed))
        if len(results) > total_expected[0]:
            total_expected[0] = len(results)
        print(f"{C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")


def flag_combinations(flags):
    """Generate test flag combos.

    Strategy:
      * Empty combo () — baseline.
      * Each flag in ALWAYS_ON_IN_COMBOS or ALWAYS_OFF_IN_COMBOS that is
        present in `flags` is tested as a standalone single-flag combo.
      * The remaining ("base") flags are powerset-combined; for each
        base subset we always add the ALWAYS_ON_IN_COMBOS flags that
        appear in `flags` and never add the ALWAYS_OFF_IN_COMBOS flags.
    """
    separate = []
    idx = 0
    while idx < len(ALWAYS_ON_IN_COMBOS):
        separate.append(ALWAYS_ON_IN_COMBOS[idx])
        idx += 1
    idx = 0
    while idx < len(ALWAYS_OFF_IN_COMBOS):
        separate.append(ALWAYS_OFF_IN_COMBOS[idx])
        idx += 1

    base_flags = []
    on_in_combos = []
    idx = 0
    while idx < len(flags):
        f = flags[idx]
        if f in ALWAYS_ON_IN_COMBOS:
            on_in_combos.append(f)
        elif f in ALWAYS_OFF_IN_COMBOS:
            pass
        else:
            base_flags.append(f)
        idx += 1

    seen = set()
    result = []

    seen.add(tuple())
    result.append(tuple())

    idx = 0
    while idx < len(flags):
        f = flags[idx]
        if f in separate:
            combo = (f,)
            if combo not in seen:
                seen.add(combo)
                result.append(combo)
        idx += 1

    n = len(base_flags)
    i = 0
    while i < (1 << n):
        combo = []
        j = 0
        while j < n:
            if i & (1 << j):
                combo.append(base_flags[j])
            j += 1
        k = 0
        while k < len(on_in_combos):
            combo.append(on_in_combos[k])
            k += 1
        combo_tuple = tuple(combo)
        if combo_tuple not in seen:
            seen.add(combo_tuple)
            result.append(combo_tuple)
        i += 1

    return result


def combo_suffix(compiler_name, flags):
    """Build a unique directory/label suffix for a compiler+flags combination."""
    parts = [compiler_name]
    if flags:
        idx = 0
        while idx < len(flags):
            parts.append(flags[idx].lower())
            idx += 1
    else:
        parts.append("default")
    return "-".join(parts)


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def clean_build_artifacts(build_dir):
    """Remove *.o, moc_*.cpp, qrc_*.cpp, ui_*.h and Makefile from a build directory."""
    if not os.path.isdir(build_dir):
        return
    import glob as _glob
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in _glob.glob(os.path.join(build_dir, pattern)):
            os.remove(f)


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
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def build_project(pro_file, build_dir, label, compiler_spec="linux-g++",
                   extra_defines=None, cxx_version=None):
    ensure_dir(build_dir)
    name = f"compile {label}"
    diag_spec = diagnostic.compiler_spec(DIAG)
    if diag_spec:
        compiler_spec = diag_spec
    # State-aware cleanup: full distclean only when compiler or C++
    # standard changed since the previous run in this dir; otherwise
    # let make handle incremental rebuild via .o-mtime tracking +
    # qmake_helpers macro invalidation for EXTRA_DEFINES changes.
    import qmake_helpers as _qh
    _decision = _qh.prepare_qmake_build_dir(build_dir, compiler_spec,
                                            cxx_version, extra_defines, ROOT)
    if _decision == "full":
        log_info(f"compiler/std changed → make distclean {label}")
        run_cmd(["make", "distclean"], build_dir, timeout=60)
        clean_build_artifacts(build_dir)
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", compiler_spec, "CONFIG+=debug", "CONFIG+=qml_debug",
                  "QMAKE_CXXFLAGS+=-g", "QMAKE_CFLAGS+=-g", "QMAKE_LFLAGS+=-g",
                  "QMAKE_LFLAGS+=-fuse-ld=mold", "LIBS+=-fuse-ld=mold"]
    qmake_args.extend(diagnostic.qmake_extra_args(DIAG))
    if cxx_version:
        qmake_args.append(f"QMAKE_CXXFLAGS+=-std={cxx_version}")
    if extra_defines:
        qmake_args.append("DEFINES+=" + " ".join(extra_defines))
    log_info(f"qmake {label}")
    rc, out = run_cmd(qmake_args, build_dir)
    if rc != 0:
        log_fail(name, f"qmake failed (rc={rc})")
        if out.strip(): print(out[-2000:])
        return False
    log_info(f"make -j{NPROC} {label}")
    rc, out = run_cmd(["make", f"-j{NPROC}"], build_dir)
    if rc != 0:
        log_fail(name, f"make failed (rc={rc})")
        if out.strip(): print(out[-3000:])
        return False
    log_pass(name)
    return True


def detect_maincodes(datapack_src):
    """Return sorted list of maincode subdirectories in map/main/."""
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return []
    return sorted([d for d in os.listdir(map_main)
                   if os.path.isdir(os.path.join(map_main, d))])

def setup_datapack_client(build_dir, datapack_src, maincode, label):
    """Copy datapack to <build_dir>/datapack/internal/, keep only maincode in map/main/."""
    dst = os.path.join(build_dir, "datapack", "internal")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    if not os.path.isdir(datapack_src):
        log_fail(f"datapack {label}", f"source not found: {datapack_src}")
        return False
    log_info(f"copying datapack to {dst}")
    shutil.copytree(datapack_src, dst, ignore=shutil.ignore_patterns(".git"))
    map_main = os.path.join(dst, "map", "main")
    if os.path.isdir(map_main):
        for entry in os.listdir(map_main):
            if entry == maincode:
                continue
            full = os.path.join(map_main, entry)
            if os.path.isdir(full):
                shutil.rmtree(full)
                log_info(f"removed map/main/{entry}")
    log_info(f"maincode: {maincode}")
    log_pass(f"datapack {label}")
    return True


def setup_server_runtime(build_dir, ref_dir=None):
    if ref_dir is None:
        ref_dir = SERVER_REF_BUILD
    for name in ("datapack", "server-properties.xml", "database"):
        dst_path = os.path.join(build_dir, name)
        src_path = os.path.join(ref_dir, name)
        if os.path.exists(dst_path) or os.path.islink(dst_path):
            continue
        if os.path.exists(src_path):
            os.symlink(src_path, dst_path)
            log_info(f"symlinked {name} -> {src_path}")


def start_server(build_dir, bin_name=SERVER_BIN_NAME):
    global server_proc
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("start server", f"binary not found: {binary}")
        return None
    log_info(f"starting server: {binary}")
    env = os.environ.copy()
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    wrapper = diagnostic.runtime_wrapper(DIAG)
    server_proc = subprocess.Popen(
        NICE_PREFIX + wrapper + [binary], cwd=build_dir,
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
    try:
        os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        server_proc.wait(timeout=5)
    server_proc = None


def run_client(build_dir, bin_name, args, label, timeout=CLIENT_TIMEOUT,
               success_marker=None, use_offscreen=True):
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail(label, f"binary not found: {binary}")
        return False
    log_info(f"running: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    if use_offscreen:
        env["QT_QPA_PLATFORM"] = "offscreen"
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    timeout = diagnostic.scale_timeout(DIAG, timeout)
    wrapper = diagnostic.runtime_wrapper(DIAG)
    if wrapper:
        gdb_args = wrapper + [binary] + args
    else:
        gdb_args = ["gdb", "-batch",
                    "-ex", "set breakpoint pending on",
                    "-ex", "handle SIGPIPE nostop noprint pass",
                    "-ex", "break __throw_out_of_range",
                    "-ex", "run", "-ex", "bt", "-ex", "quit",
                    "--args", binary] + args
    try:
        p = subprocess.run(NICE_PREFIX + gdb_args, cwd=build_dir, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        out = p.stdout.decode(errors="replace")
        if p.returncode == 0:
            log_pass(label, "exit code 0")
            return True
        else:
            log_fail(label, f"exit code {p.returncode}")
            # print backtrace from gdb output
            for line in out.splitlines()[-40:]:
                print(f"  | {line}")
            return False
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if e.stdout else ""
        if success_marker and success_marker in out:
            log_pass(label, f"timeout but connected (found '{success_marker}')")
            return True
        log_fail(label, f"timeout after {timeout}s")
        for line in out.splitlines()[-20:]:
            print(f"  | {line}")
        return False


def run_client_benchmark(build_dir, bin_name, args, label, timeout=30,
                         success_marker="MapVisualiserPlayer::mapDisplayedSlot()"):
    """Run client and measure time in ms until success_marker appears in output.
    Returns elapsed_ms (float) or None on failure/timeout.
    Runs without nice/gdb for accurate timing."""
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_info(f"benchmark skip: binary not found: {binary}")
        return None
    log_info(f"benchmark: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    timeout = diagnostic.scale_timeout(DIAG, timeout)
    wrapper = diagnostic.runtime_wrapper(DIAG)

    start_time = time.monotonic()
    proc = subprocess.Popen(
        wrapper + [binary] + list(args), cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)

    found_event = threading.Event()
    elapsed_ms = [None]
    output_lines = []

    def reader():
        while True:
            raw = proc.stdout.readline()
            if not raw:
                break
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if success_marker in line:
                elapsed_ms[0] = (time.monotonic() - start_time) * 1000.0
                found_event.set()
                return

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    if found_event.wait(timeout=timeout):
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            proc.wait(timeout=5)
        log_info(f"{label}: {elapsed_ms[0]:.0f}ms")
        return elapsed_ms[0]
    else:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            proc.wait(timeout=5)
        log_info(f"{label}: timeout after {timeout}s")
        idx = len(output_lines) - 20
        if idx < 0:
            idx = 0
        while idx < len(output_lines):
            print(f"  | {output_lines[idx]}")
            idx += 1
        return None


def remove_savegames(path, label):
    if os.path.isdir(path):
        shutil.rmtree(path)
        log_info(f"removed savegames: {path}")
    else:
        log_info(f"no savegames to remove: {path}")


# Windows / macOS / Android cross-build helpers used to live here. Moved to
# the standalone scripts (testingcompilationwindows.py, ...mac.py, ...android.py).


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_server()
    sys.exit(1)

signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Client Testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete test/failed.json for full re-run)")
        return

    # ═══════════════════════════════════════════════════════════════
    # 1. COMPILATION — all flag + compiler combinations
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Compilation (all combinations) ---{C_RESET}\n")

    # start remote builds in parallel with local builds.
    # server-filedb is built on remote so that section 5 (REMOTE SERVER +
    # LOCAL CLIENT) has a binary to launch — testingserver.py also builds
    # it but cleans up the remote work dir afterwards, so testingclient.py
    # cannot rely on it being present.
    remote_pro_rels = [
        "server/epoll/catchchallenger-server-filedb.pro",
        os.path.relpath(CLIENT_CPU_PRO, ROOT),
        os.path.relpath(CLIENT_GL_PRO, ROOT),
    ]
    if diagnostic.is_active(DIAG):
        log_info(f"diagnostic mode: {diagnostic.describe(DIAG)}")
    if failed_cases is None:
        remote_threads, remote_results, remote_lock = start_remote_builds(
            remote_pro_rels, diag=DIAG)
        log_info("remote builds started in background")
    else:
        # In resume mode, still build server-filedb on remote when a
        # failing case requires the remote binary (it gets wiped between
        # full runs by testingserver.py's cleanup_remote_node).
        needs_remote_server = any(("remote " in c and "server start" in c)
                                  or "--autosolo" in c
                                  for c in failed_cases)
        if needs_remote_server:
            remote_threads, remote_results, remote_lock = start_remote_builds(
                ["server/epoll/catchchallenger-server-filedb.pro"], diag=DIAG)
            log_info("resume mode: rebuilding server-filedb on remote (needed by failing cases)")
        else:
            remote_threads, remote_results, remote_lock = [], [], None
            log_info("resume mode: skipping remote builds")

    cpu_combos = flag_combinations(CPU_OPTIONAL_FLAGS)
    gl_combos = flag_combinations(GL_OPTIONAL_FLAGS)

    total_cpu = len(CXX_VERSIONS) * len(COMPILERS) * len(cpu_combos)
    total_gl = len(CXX_VERSIONS) * len(COMPILERS) * len(gl_combos)

    # compute total expected results
    n_maincodes = 0
    idx = 0
    while idx < len(DATAPACKS):
        mc = detect_maincodes(DATAPACKS[idx])
        n_maincodes += len(mc) if mc else 0
        idx += 1
    # Compile-phase remote count: derived from start_remote_builds' own
    # iteration, so node opt-in / cxx_versions / make.py vs qmake all count
    # automatically.
    remote_count = count_remote_tests(remote_pro_rels, diag=DIAG)
    # server-client phase: per opted-in node, 1 server start + 1 client run
    # per client target (CPU + GL).
    client_builds_list = [CLIENT_CPU_PRO, CLIENT_GL_PRO]
    ri = 0
    while ri < len(REMOTE_SERVERS):
        _label = REMOTE_SERVERS[ri][0]
        if node_runs(_label, "server-client"):
            remote_count += 1 + len(client_builds_list)
        ri += 1
    client_types_list = [CLIENT_CPU_BUILD, CLIENT_GL_BUILD]
    solo_runs_list = ["first run", "come back"]
    # per maincode: datapack setups + solo runs per client
    per_maincode = len(client_types_list) + len(client_types_list) * len(solo_runs_list)
    # multiplayer: server build + server start + client connects
    multiplayer_parts = ["server_build", "server_start"] + client_types_list
    multiplayer = len(multiplayer_parts)
    # remote: already counted in remote_count (rsync + builds + server_start + client_connects)
    benchmark_count = 1  # benchmark player on map (1 pass/fail result)
    #Windows (MXE+wine64), macOS (qemu VM via ssh) and Android (Qt-for-Android +
    #emulator) phases moved out into their own scripts (see all.sh); they no
    #longer contribute to this script's expected-results count.
    total_expected[0] = (total_cpu + total_gl + remote_count
                         + n_maincodes * per_maincode + multiplayer
                         + benchmark_count)
    log_info(f"qtcpu800x600: {total_cpu} combinations "
             f"({len(CXX_VERSIONS)} c++ versions x {len(COMPILERS)} compilers x {len(cpu_combos)} flag sets)")
    log_info(f"qtopengl:     {total_gl} combinations "
             f"({len(CXX_VERSIONS)} c++ versions x {len(COMPILERS)} compilers x {len(gl_combos)} flag sets)")
    log_info(f"total expected results: {total_expected[0]}")

    cpu_ok = False
    gl_ok = False

    # in resume mode, check if default binaries exist from previous builds
    if failed_cases is not None:
        if os.path.isfile(os.path.join(CLIENT_CPU_BUILD, CLIENT_CPU_BIN)):
            cpu_ok = True
        if os.path.isfile(os.path.join(CLIENT_GL_BUILD, CLIENT_GL_BIN)):
            gl_ok = True

    # qtcpu800x600 — all combinations
    vi = 0
    while vi < len(CXX_VERSIONS):
        cxx_ver = CXX_VERSIONS[vi]
        ci = 0
        while ci < len(COMPILERS):
            compiler_name, compiler_spec = COMPILERS[ci]
            fi = 0
            while fi < len(cpu_combos):
                flags = cpu_combos[fi]
                all_defines = list(CPU_BASE_DEFINES) + list(flags)
                suffix = combo_suffix(compiler_name, flags)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    build_dir = CLIENT_CPU_BUILD
                else:
                    build_dir = os.path.join(ROOT, "client", "qtcpu800x600", "build",
                                             f"testing-cpu-{cxx_ver}-{suffix}")
                label = f"qtcpu800x600 {cxx_ver} {compiler_name}"
                if flags:
                    label += " " + "+".join(flags)
                if not should_run(f"compile {label}", failed_cases):
                    fi += 1
                    continue
                ok = build_project(CLIENT_CPU_PRO, build_dir, label, compiler_spec,
                                   all_defines if all_defines else None,
                                   cxx_version=cxx_ver)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    cpu_ok = ok
                fi += 1
            ci += 1
        vi += 1

    # qtopengl — all combinations
    vi = 0
    while vi < len(CXX_VERSIONS):
        cxx_ver = CXX_VERSIONS[vi]
        ci = 0
        while ci < len(COMPILERS):
            compiler_name, compiler_spec = COMPILERS[ci]
            fi = 0
            while fi < len(gl_combos):
                flags = gl_combos[fi]
                all_defines = list(GL_BASE_DEFINES) + list(flags)
                suffix = combo_suffix(compiler_name, flags)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    build_dir = CLIENT_GL_BUILD
                else:
                    build_dir = os.path.join(ROOT, "client", "qtopengl", "build",
                                             f"testing-gl-{cxx_ver}-{suffix}")
                label = f"qtopengl {cxx_ver} {compiler_name}"
                if flags:
                    label += " " + "+".join(flags)
                if not should_run(f"compile {label}", failed_cases):
                    fi += 1
                    continue
                ok = build_project(CLIENT_GL_PRO, build_dir, label, compiler_spec,
                                   all_defines if all_defines else None,
                                   cxx_version=cxx_ver)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    gl_ok = ok
                fi += 1
            ci += 1
        vi += 1

    # collect remote build results
    if remote_threads:
        log_info("waiting for remote builds to finish...")
    remote = collect_remote_results(remote_threads, remote_results, remote_lock) if remote_threads else []
    idx = 0
    while idx < len(remote):
        name, ok, detail = remote[idx]
        if ok:
            log_pass(name, detail)
        else:
            log_fail(name, detail)
        idx += 1

    # ═══════════════════════════════════════════════════════════════
    # 2 + 3. DATAPACK SETUP + SOLO (iterate each datapack + maincode)
    # ═══════════════════════════════════════════════════════════════
    for dp_path in DATAPACKS:
        dp_name = os.path.basename(dp_path)
        maincodes = detect_maincodes(dp_path)
        if not maincodes:
            log_fail(f"datapack {dp_name}", "no maincodes in map/main/")
            continue
        log_info(f"{dp_name}: maincodes found: {', '.join(maincodes)}")

        for mc in maincodes:
            print(f"\n{C_CYAN}--- Datapack + Solo: {dp_name} maincode={mc} ---{C_RESET}\n")

            dp_cpu = False
            dp_gl  = False
            if cpu_ok:
                dp_cpu = setup_datapack_client(CLIENT_CPU_BUILD, dp_path, mc,
                                               f"qtcpu800x600 ({dp_name} {mc})")
            if gl_ok:
                dp_gl = setup_datapack_client(CLIENT_GL_BUILD, dp_path, mc,
                                              f"qtopengl ({dp_name} {mc})")

            if cpu_ok and dp_cpu:
                remove_savegames(SAVEGAME_CPU, "qtcpu800x600")
                if should_run(f"qtcpu800x600 solo first run ({dp_name} {mc})", failed_cases):
                    run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                               ["--autosolo", "--closewhenonmap"],
                               f"qtcpu800x600 solo first run ({dp_name} {mc})",
                               timeout=CLIENT_SOLO_TIMEOUT,
                               success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
                if should_run(f"qtcpu800x600 solo come back ({dp_name} {mc})", failed_cases):
                    run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                               ["--autosolo", "--closewhenonmap"],
                               f"qtcpu800x600 solo come back ({dp_name} {mc})",
                               timeout=CLIENT_SOLO_TIMEOUT,
                               success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

            if gl_ok and dp_gl:
                remove_savegames(SAVEGAME_GL, "qtopengl")
                if should_run(f"qtopengl solo first run ({dp_name} {mc})", failed_cases):
                    run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                               ["--autosolo", "--closewhenonmap"],
                               f"qtopengl solo first run ({dp_name} {mc})",
                               timeout=CLIENT_SOLO_TIMEOUT,
                               success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
                if should_run(f"qtopengl solo come back ({dp_name} {mc})", failed_cases):
                    run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                               ["--autosolo", "--closewhenonmap"],
                               f"qtopengl solo come back ({dp_name} {mc})",
                               timeout=CLIENT_SOLO_TIMEOUT,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    # ═══════════════════════════════════════════════════════════════
    # 4. MULTIPLAYER ON SERVER
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Multiplayer on server ---{C_RESET}\n")

    # check which multiplayer tests need to run
    need_mp = (should_run("compile server-filedb", failed_cases) or
               should_run("server start", failed_cases) or
               should_run("qtcpu800x600 connect to server", failed_cases) or
               should_run("qtopengl connect to server", failed_cases) or
               should_run("benchmark player on map", failed_cases))
    if need_mp:
        if should_run("compile server-filedb", failed_cases):
            if not build_project(SERVER_PRO, SERVER_BUILD, "server-filedb"):
                stop_server()
                summary()
                return
        elif not os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN_NAME)):
            log_fail("compile server-filedb", "binary not found from previous build")
            stop_server()
            summary()
            return

        setup_server_runtime(SERVER_BUILD)
        db_dir = os.path.join(SERVER_BUILD, "database")
        if os.path.isdir(db_dir):
            shutil.rmtree(db_dir)
            log_info("cleared database/ for clean multiplayer test")
        set_http_datapack_mirror(SERVER_BUILD, "")
        srv = start_server(SERVER_BUILD)
        if srv is None:
            summary()
            return

        if cpu_ok and should_run("qtcpu800x600 connect to server", failed_cases):
            run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                       ["--host", SERVER_HOST, "--port", SERVER_PORT,
                        "--autologin", "--character", "PlayerCPU",
                        "--closewhenonmap"],
                       "qtcpu800x600 connect to server",
                       timeout=30,
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

        if gl_ok and should_run("qtopengl connect to server", failed_cases):
            run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                       ["--host", SERVER_HOST, "--port", SERVER_PORT,
                        "--autologin", "--character", "PlayerGL",
                        "--closewhenonmap"],
                       "qtopengl connect to server",
                       timeout=30,
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    # ═══════════════════════════════════════════════════════════════
    # 4b. BENCHMARK — time to player on map
    # ═══════════════════════════════════════════════════════════════
    if should_run("benchmark player on map", failed_cases):
        print(f"\n{C_CYAN}--- Benchmark: client player on map ---{C_RESET}\n")

    cpu_time_ms = None
    gl_time_ms = None

    if cpu_ok and should_run("benchmark player on map", failed_cases):
        cpu_time_ms = run_client_benchmark(
            CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
            ["--host", SERVER_HOST, "--port", SERVER_PORT,
             "--autologin", "--character", "BenchCPU",
             "--closewhenonmap"],
            "benchmark qtcpu800x600",
            timeout=30)

    if gl_ok and should_run("benchmark player on map", failed_cases):
        gl_time_ms = run_client_benchmark(
            CLIENT_GL_BUILD, CLIENT_GL_BIN,
            ["--host", SERVER_HOST, "--port", SERVER_PORT,
             "--autologin", "--character", "BenchGL",
             "--closewhenonmap"],
            "benchmark qtopengl",
            timeout=30)

    benchmark_label = "benchmark player on map"
    if not should_run(benchmark_label, failed_cases):
        pass  # benchmark intentionally skipped in resume mode
    elif cpu_time_ms is not None and gl_time_ms is not None:
        failed = False
        if cpu_time_ms > BENCHMARK_MAX_MS:
            log_fail(benchmark_label,
                     f"qtcpu800x600 took {cpu_time_ms:.0f}ms (>{BENCHMARK_MAX_MS}ms), has to be optimized")
            failed = True
        if gl_time_ms > BENCHMARK_MAX_MS:
            log_fail(benchmark_label,
                     f"qtopengl took {gl_time_ms:.0f}ms (>{BENCHMARK_MAX_MS}ms), has to be optimized")
            failed = True
        if cpu_time_ms > 0 and gl_time_ms > 0:
            if cpu_time_ms >= gl_time_ms * 2:
                log_fail(benchmark_label,
                         f"[FAILED] have to be optimized, clientcpu800x600 is slower than other "
                         f"({cpu_time_ms:.0f}ms vs {gl_time_ms:.0f}ms, {cpu_time_ms/gl_time_ms:.1f}x)")
                failed = True
            elif gl_time_ms >= cpu_time_ms * 2:
                log_fail(benchmark_label,
                         f"[FAILED] have to be optimized, clientopengl is slower than other "
                         f"({gl_time_ms:.0f}ms vs {cpu_time_ms:.0f}ms, {gl_time_ms/cpu_time_ms:.1f}x)")
                failed = True
        if not failed:
            log_pass(benchmark_label,
                     f"qtcpu800x600={cpu_time_ms:.0f}ms, qtopengl={gl_time_ms:.0f}ms")
    elif cpu_time_ms is not None:
        if cpu_time_ms > BENCHMARK_MAX_MS:
            log_fail(benchmark_label,
                     f"qtcpu800x600 took {cpu_time_ms:.0f}ms (>{BENCHMARK_MAX_MS}ms), has to be optimized")
        else:
            log_pass(benchmark_label,
                     f"qtcpu800x600={cpu_time_ms:.0f}ms (qtopengl not available)")
    elif gl_time_ms is not None:
        if gl_time_ms > BENCHMARK_MAX_MS:
            log_fail(benchmark_label,
                     f"qtopengl took {gl_time_ms:.0f}ms (>{BENCHMARK_MAX_MS}ms), has to be optimized")
        else:
            log_pass(benchmark_label,
                     f"qtopengl={gl_time_ms:.0f}ms (qtcpu800x600 not available)")
    else:
        log_fail(benchmark_label, "no client available for benchmark")

    stop_server()

    # ═══════════════════════════════════════════════════════════════
    # 5. REMOTE SERVER + LOCAL CLIENT
    # ═══════════════════════════════════════════════════════════════
    if failed_cases is not None:
        log_info("resume mode: skipping remote server tests")
        summary()
        return

    print(f"\n{C_CYAN}--- Remote server + local client ---{C_RESET}\n")

    dp_src = DATAPACKS[0] if DATAPACKS else None

    ri = 0
    while ri < len(REMOTE_SERVERS):
        (label, host, ssh_port, _use_mold, _extra_defines, _has_gui, _cxx_ver,
         remote_dir) = REMOTE_SERVERS[ri]
        if not node_runs(label, "server-client"):
            log_info(f"skipping remote {label}: not opted into 'server-client'")
            ri += 1
            continue
        node = get_node(label)
        if node is not None and not diagnostic.node_supports(node, DIAG):
            log_info(f"skipping remote {label}: no "
                     f"{diagnostic.compiler_name(DIAG)} for diagnostic mode")
            ri += 1
            continue
        server_addr = get_remote_server_address(host)
        print(f"\n{C_CYAN}  -- Remote: {label} ({server_addr}) --{C_RESET}\n")

        remote_build_root = f"{remote_dir}/build-remote{_DIAG_SUFFIX}"
        remote_filedb_bin = f"{remote_build_root}/catchchallenger-server-filedb/catchchallenger-server-cli-epoll"
        remote_filedb_build = f"{remote_build_root}/catchchallenger-server-filedb"

        try:
            if dp_src is None:
                log_fail(f"remote {label}", "no datapack source")
                ri += 1
                continue
            if not setup_remote_server_runtime(host, ssh_port, remote_filedb_build, dp_src):
                log_fail(f"remote {label} setup", "failed to rsync datapack")
                ri += 1
                continue

            node_ready_timeout = node.get("server_ready_timeout") if node else None
            ssh_proc, actual_port = start_remote_server(host, ssh_port,
                                                        remote_filedb_bin,
                                                        remote_filedb_build,
                                                        diag=DIAG,
                                                        ready_timeout=node_ready_timeout)
            if actual_port == 0:
                if ssh_proc == "BINARY_MISSING":
                    log_fail(f"remote {label} server start",
                             f"binary missing at {remote_filedb_bin}")
                else:
                    log_fail(f"remote {label} server start",
                             "timeout waiting for 'correctly bind:'")
                ri += 1
                continue
            log_pass(f"remote {label} server start", f"port {actual_port}")

            if cpu_ok:
                run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                           ["--host", server_addr, "--port", str(actual_port),
                            "--autologin", "--character", "PlayerCPU",
                            "--closewhenonmap"],
                           f"qtcpu800x600 -> remote {label}",
                           timeout=30,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

            if gl_ok:
                run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                           ["--host", server_addr, "--port", str(actual_port),
                            "--autologin", "--character", "PlayerGL",
                            "--closewhenonmap"],
                           f"qtopengl -> remote {label}",
                           timeout=30,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

            stop_remote_server(ssh_proc, host, ssh_port)
        finally:
            cleanup_remote_node(host, ssh_port, remote_dir)
        ri += 1

    # ═══════════════════════════════════════════════════════════════
    # 6. REMOTE --autosolo (build + run both clients on the node)
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Remote --autosolo ---{C_RESET}\n")

    if dp_src is None:
        log_info("skipping remote --autosolo: no datapack source")
    else:
        ni = 0
        while ni < len(REMOTE_NODES):
            node = REMOTE_NODES[ni]
            label = node["label"]
            if not node_runs(label, "solo"):
                ni += 1
                continue
            # resume mode: skip nodes whose tests all already passed
            need_run = True
            if failed_cases is not None:
                need_run = False
                for entry in failed_cases:
                    if entry.startswith(f"solo {label}") or \
                       entry.startswith(f"compile ") and f"({label}" in entry:
                        need_run = True
                        break
                if not need_run:
                    log_info(f"skipping solo {label}: no failed tests to resume")
                    ni += 1
                    continue
            print(f"\n{C_CYAN}  -- Solo: {label} --{C_RESET}\n")
            phase_results = run_remote_autosolo_phase(node, dp_src, diag=DIAG)
            ri = 0
            while ri < len(phase_results):
                name, ok, detail = phase_results[ri]
                if not should_run(name, failed_cases):
                    ri += 1
                    continue
                if ok:
                    log_pass(name, detail)
                else:
                    log_fail(name, detail)
                ri += 1
            ni += 1

    # ═══════════════════════════════════════════════════════════════
    # WINDOWS / macOS / ANDROID cross-compile + run phases used to live
    # here. They are now standalone scripts (testingcompilationwindows.py,
    # testingcompilationmac.py, testingcompilationandroid.py) so they can
    # be invoked / scheduled independently from this script. all.sh runs
    # the three after testingclient.py finishes.
    # ═══════════════════════════════════════════════════════════════


    summary()


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
