#!/usr/bin/env python3
"""
testingbots.py — CatchChallenger bot-actions stability test.

Sections:
  1. Compilation: build bot-actions and server-filedb
  2. Bot run: start server, run bot for 10 minutes, check for crashes/errors/inactivity
"""

import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re, time
from remote_build import (start_remote_builds, collect_remote_results,
                          REMOTE_SERVERS, REMOTE_DIR, count_remote_tests)
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

BOT_PRO         = os.path.join(ROOT, "tools/bot-actions/bot-actions.pro")
BOT_BUILD       = os.path.join(ROOT, "tools/bot-actions/build/testing" + _DIAG_SUFFIX)
BOT_BIN         = "bot-actions"

SERVER_HOST  = _config["server_host"]
SERVER_PORT  = str(_config["server_port"])

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
BOT_RUN_DURATION     = 60    # 1 minute stability run
BOT_ACTION_TIMEOUT   = 120   # 2 minutes without non-move action -> fail (kept above duration so action-idle is not the failure mode in this short stability test)

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


# ── patterns that indicate a non-move action ───────────────────────────────
# These stdout patterns mean the bot is doing something beyond just walking.
ACTION_PATTERNS = [
    "Start this: Try capture",
    "Start this: Try skill",
    "is now in fight",
    "Seed correctly planted",
    "Seed cannot be planted",
    "Plant collected",
    "You have finish the quest",
    "tp after loose",
    "normal tp",
    "monsterCatch(",
    "haveBeatBot",
    "Start this: In fight",
]

# Patterns that indicate errors/bugs/problems in console output
ERROR_PATTERNS = [
    re.compile(r"abort", re.IGNORECASE),
    re.compile(r"\berror\b", re.IGNORECASE),
    re.compile(r"\bbug\b", re.IGNORECASE),
    re.compile(r"SIGABRT"),
    re.compile(r"SIGSEGV"),
    re.compile(r"Segmentation fault"),
    re.compile(r"out_of_range"),
    re.compile(r"std::bad_alloc"),
    re.compile(r"ASSERT"),
    re.compile(r"mHaveAnError"),
]

# Lines matching these are NOT real errors (false positives from the patterns above)
ERROR_WHITELIST = [
    "CATCHCHALLENGER_ABORTIFERROR",
    "set breakpoint pending",
    "handle SIGPIPE",
    "error at loading",            # map loading info, followed by abort only if fatal
    "Path not found",              # pathfinding miss, not a crash
    "Warning: Bug due to resolved path is empty",  # known benign warning
    "uniqueKey==0, suspect bug",   # benign warning when only one server is in the pool
]


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
                   extra_defines=None):
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
                                            None, extra_defines, ROOT)
    if _decision == "full":
        log_info(f"compiler/std changed → make distclean {label}")
        run_cmd(["make", "distclean"], build_dir, timeout=60)
        clean_build_artifacts(build_dir)
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", compiler_spec, "CONFIG+=debug", "CONFIG+=qml_debug",
                  "QMAKE_CXXFLAGS+=-g", "QMAKE_CFLAGS+=-g", "QMAKE_LFLAGS+=-g",
                  "QMAKE_LFLAGS+=-fuse-ld=mold", "LIBS+=-fuse-ld=mold"]
    qmake_args.extend(diagnostic.qmake_extra_args(DIAG))
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
        while True:
            raw = server_proc.stdout.readline()
            if not raw:
                break
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


def is_whitelisted(line):
    """Return True if the line matches a known false-positive error pattern."""
    idx = 0
    while idx < len(ERROR_WHITELIST):
        if ERROR_WHITELIST[idx] in line:
            return True
        idx += 1
    return False


def is_action_line(line):
    """Return True if the line indicates a non-move bot action."""
    idx = 0
    while idx < len(ACTION_PATTERNS):
        if ACTION_PATTERNS[idx] in line:
            return True
        idx += 1
    return False


def is_error_line(line):
    """Return True if the line indicates an error/bug/problem."""
    if is_whitelisted(line):
        return False
    idx = 0
    while idx < len(ERROR_PATTERNS):
        if ERROR_PATTERNS[idx].search(line):
            return True
        idx += 1
    return False


def run_bot(build_dir, bin_name, args, label, duration=BOT_RUN_DURATION,
            action_timeout=BOT_ACTION_TIMEOUT):
    """Run bot for the given duration. Track crashes, errors, and action inactivity.
    Returns (crashed, errors_found, action_timed_out, last_action_age_s)."""
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail(label, f"binary not found: {binary}")
        return True, [], False, 0

    duration = diagnostic.scale_timeout(DIAG, duration)
    action_timeout = diagnostic.scale_timeout(DIAG, action_timeout)
    log_info(f"running bot: {bin_name} {' '.join(args)} for {duration}s")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    wrapper = diagnostic.runtime_wrapper(DIAG)

    proc = subprocess.Popen(
        NICE_PREFIX + wrapper + [binary] + list(args), cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)

    lock = threading.Lock()
    output_lines = []
    error_lines = []
    on_map = threading.Event()
    last_action_time = [time.monotonic()]
    action_timeout_detected = [False]
    crashed = [False]
    stop_event = threading.Event()

    def reader():
        while True:
            raw = proc.stdout.readline()
            if not raw:
                break
            line = raw.decode(errors="replace").rstrip("\n")
            with lock:
                output_lines.append(line)

            # detect on-map
            if "all_player_on_map()" in line:
                on_map.set()
                with lock:
                    last_action_time[0] = time.monotonic()

            # detect non-move action
            if is_action_line(line):
                with lock:
                    last_action_time[0] = time.monotonic()

            # detect errors
            if is_error_line(line):
                with lock:
                    error_lines.append(line)

    reader_thread = threading.Thread(target=reader, daemon=True)
    reader_thread.start()

    # wait for bot to reach map (use 60s timeout like the bot itself)
    if not on_map.wait(timeout=diagnostic.scale_timeout(DIAG, 60)):
        log_fail(label, "timeout waiting for bot to reach map (60s)")
        with lock:
            tail = output_lines[-50:]
        for l in tail:
            print(f"  | {l}")
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            try: os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError: pass
            proc.wait(timeout=5)
        return True, [], False, 0

    log_info("bot is on map, starting stability timer")

    # monitor for the full duration
    start_time = time.monotonic()
    while time.monotonic() - start_time < duration:
        # check if process died
        ret = proc.poll()
        if ret is not None:
            crashed[0] = True
            break

        # check action inactivity
        with lock:
            age = time.monotonic() - last_action_time[0]
        if age >= action_timeout:
            action_timeout_detected[0] = True
            age_s = int(age)
            print(f"{C_RED}[BOT]{C_RESET} no non-move action for {age_s}s (limit {action_timeout}s)")
            break

        elapsed = int(time.monotonic() - start_time)
        remaining = duration - elapsed
        # print progress every 60s
        if elapsed > 0 and elapsed % 60 == 0:
            with lock:
                a = int(time.monotonic() - last_action_time[0])
            log_info(f"bot running: {elapsed}s/{duration}s, last action {a}s ago")

        time.sleep(1)

    # collect final action age
    with lock:
        final_action_age = time.monotonic() - last_action_time[0]

    # kill the bot
    if proc.poll() is None:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            try: os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError: pass
            proc.wait(timeout=5)
    elif not crashed[0]:
        # process exited on its own — that's a crash
        crashed[0] = True

    reader_thread.join(timeout=5)

    with lock:
        errs = list(error_lines)

    return crashed[0], errs, action_timeout_detected[0], final_action_age


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_server()
    sys.exit(1)

signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Bot Actions Testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete test/failed.json for full re-run)")
        return

    # total: compile bot + compile server + server start + bot stability
    total_expected[0] = 4

    # ═══════════════════════════════════════════════════════════════
    # 1. COMPILATION
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Compilation ---{C_RESET}\n")

    # start remote builds in background
    remote_pro_rels = [
        os.path.relpath(BOT_PRO, ROOT),
    ]
    if diagnostic.is_active(DIAG):
        log_info(f"diagnostic mode: {diagnostic.describe(DIAG)}")
    if failed_cases is None:
        remote_threads, remote_results, remote_lock = start_remote_builds(
            remote_pro_rels, diag=DIAG)
        log_info("remote builds started in background")
    else:
        remote_threads, remote_results, remote_lock = [], [], None
        log_info("resume mode: skipping remote builds")

    # count remote expected results — single source of truth: remote_build
    # replays the same per-node iteration that start_remote_builds() runs,
    # so adding/disabling nodes (or changing cxx_versions) updates the count
    # automatically.
    total_expected[0] += count_remote_tests(remote_pro_rels, diag=DIAG)

    log_info(f"total expected results: {total_expected[0]}")

    if should_run("compile bot-actions", failed_cases):
        bot_ok = build_project(BOT_PRO, BOT_BUILD, "bot-actions")
    else:
        bot_ok = os.path.isfile(os.path.join(BOT_BUILD, BOT_BIN))
    if should_run("compile server-filedb", failed_cases):
        server_ok = build_project(SERVER_PRO, SERVER_BUILD, "server-filedb")
    else:
        server_ok = os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN_NAME))

    # collect remote results
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

    if not bot_ok or not server_ok:
        summary()
        return

    # ═══════════════════════════════════════════════════════════════
    # 2. BOT STABILITY RUN
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Bot stability run ({BOT_RUN_DURATION}s) ---{C_RESET}\n")

    setup_server_runtime(SERVER_BUILD)
    # replace server datapack with clean copy (previous tests may have modified it)
    dp_src = DATAPACKS[0] if DATAPACKS else None
    server_dp = os.path.join(SERVER_BUILD, "datapack")
    if dp_src and os.path.isdir(dp_src):
        if os.path.islink(server_dp):
            os.remove(server_dp)
        elif os.path.isdir(server_dp):
            shutil.rmtree(server_dp)
        shutil.copytree(dp_src, server_dp, ignore=shutil.ignore_patterns(".git"))
        log_info(f"copied server datapack from {dp_src}")
    # clear stale bot datapack and copy from actual datapack source
    bot_dp = os.path.join(BOT_BUILD, "datapack")
    if os.path.exists(bot_dp) or os.path.islink(bot_dp):
        if os.path.islink(bot_dp):
            os.remove(bot_dp)
        else:
            shutil.rmtree(bot_dp)
        log_info("removed stale bot datapack")
    if dp_src and os.path.isdir(dp_src):
        shutil.copytree(dp_src, bot_dp, ignore=shutil.ignore_patterns(".git"))
        log_info(f"copied bot datapack from {dp_src}")
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
        log_info("cleared database/ for clean bot test")
    set_http_datapack_mirror(SERVER_BUILD, "")
    srv = start_server(SERVER_BUILD)
    if srv is None:
        summary()
        return

    crashed, error_lines, action_timed_out, last_action_age = run_bot(
        BOT_BUILD, BOT_BIN,
        ["--host", SERVER_HOST, "--port", SERVER_PORT,
         "--login", "testbot1", "--pass", "testbot1"],
        "bot stability run",
        duration=BOT_RUN_DURATION,
        action_timeout=BOT_ACTION_TIMEOUT)

    stop_server()

    # evaluate results
    label = "bot stability"
    failed = False

    if crashed:
        log_fail(label, "bot crashed or exited unexpectedly")
        failed = True

    if error_lines:
        unique_errors = []
        seen = set()
        ei = 0
        while ei < len(error_lines):
            stripped = error_lines[ei].strip()
            if stripped not in seen:
                seen.add(stripped)
                unique_errors.append(stripped)
            ei += 1
        log_fail(label, f"{len(error_lines)} error(s) in console output ({len(unique_errors)} unique)")
        ui = 0
        while ui < len(unique_errors) and ui < 20:
            print(f"  | {unique_errors[ui]}")
            ui += 1
        failed = True

    if action_timed_out:
        log_fail(label,
                 f"no non-move action for {int(last_action_age)}s "
                 f"(limit {BOT_ACTION_TIMEOUT}s), bot is idle")
        failed = True

    if not failed:
        log_pass(label,
                 f"ran {BOT_RUN_DURATION}s without crash, errors, or inactivity "
                 f"(last action {int(last_action_age)}s ago)")

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
