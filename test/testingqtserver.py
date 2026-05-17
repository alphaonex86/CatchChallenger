#!/usr/bin/env python3
"""
testingqtserver.py — build & smoke-test the Qt-based admin server.

Two phases:
  1. Compile catchchallenger-server-gui via the
     -DCATCHCHALLENGER_BUILD_QT_SERVER_GUI=ON cmake toggle. The target
     lives in server/qt/CMakeLists.txt (an opt-in option that's OFF by
     default so the rest of the test matrix doesn't pick it up).
  2. Stage the test datapack (CatchChallenger-datapack, maincode=test)
     next to the binary, then run with --autostart. PASS when stdout
     prints the "Listen " marker that NormalServer::start_server()
     emits once the SSL/QSslServer socket is bound, within 10s.

The server's SQLite backend auto-creates the on-disk database on first
launch (mirrors qtcpu800x600's SoloDatabaseInit pattern): on its first
QSqlDatabase::open() it sees an empty file, reads
:/catchchallenger-sqlite.sql from the embedded resource, and replays
every statement. So the harness only has to make the build dir
writable; it does NOT pre-seed the schema.
"""

import sys
sys.dont_write_bytecode=True

import os, json, subprocess, shutil, time, multiprocessing
import diagnostic
import process_helpers
import wall_cap
wall_cap.arm()
import build_paths
import cleanup_helpers
from cmd_helpers import clamp_local

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()

# ── paths ───────────────────────────────────────────────────────────────────
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

BUILD_DIR = build_paths.build_path("server", "qt", "build",
                                   "testing-qtserver" + diagnostic.build_dir_suffix(DIAG))
SERVER_BIN = os.path.join(BUILD_DIR, "catchchallenger-server-gui")
cleanup_helpers.register_build_dir(BUILD_DIR)

DATAPACK_SRC = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
MAINCODE = "test"
# The server reads <exe_dir>/datapack/ and <exe_dir>/server-properties.xml,
# so stage everything next to the produced binary.
DATAPACK_DST = os.path.join(os.path.dirname(SERVER_BIN), "datapack")

COMPILE_TIMEOUT = 1200
RUN_TIMEOUT = 10
LISTEN_MARKER = "Listen "

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

SCRIPT_COMPILE_NAME = "compile catchchallenger-server-gui"
SCRIPT_RUN_NAME = "run server-gui --autostart"
SCRIPT_NOPLUGIN_NAME = "run server-gui no Qt platform plugin (graceful exit, no SIGABRT)"
from test_config import FAILED_JSON

results = []
_last_log = [time.monotonic()]


def log_info(msg):
    print(f"{phase_timer.t()} {C_CYAN}[INFO]{C_RESET} {msg}", flush=True)

def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log[0]
    _last_log[0] = now
    results.append((name, True, detail, elapsed))
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)", flush=True)
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)

def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log[0]
    _last_log[0] = now
    results.append((name, False, detail, elapsed))
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)", flush=True)
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


# ── failed.json resume plumbing ─────────────────────────────────────────────
import failed_cases as _fc
import phase_timer


def load_failed_cases():
    a = _fc.load_names(SCRIPT_COMPILE_NAME)
    b = _fc.load_names(SCRIPT_RUN_NAME)
    if a is None and b is None:
        return None
    return (a or []) + (b or [])


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    fc, fr = [], []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            entry = (name, d)
            if "compile" in name:
                fc.append(entry)
            else:
                fr.append(entry)
    _fc.save(SCRIPT_COMPILE_NAME, fc)
    _fc.save(SCRIPT_RUN_NAME, fr)


# ── build helpers ──────────────────────────────────────────────────────────
def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    full_args = ["nice", "-n", "19", "ionice", "-c", "3"] + list(args)
    diagnostic.record_cmd(full_args, cwd)
    try:
        p = subprocess.run(
            full_args, cwd=cwd, timeout=timeout, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if e.stdout else ""
        return -1, f"TIMEOUT after {timeout}s\n{out}"


def test_compile():
    name = SCRIPT_COMPILE_NAME
    cc, cxx, _ = ("clang", "clang++", "clang") if (
        diagnostic.compiler_name(DIAG) == "clang") else ("gcc", "g++", "gcc")

    os.makedirs(BUILD_DIR, exist_ok=True)
    # Per "one binary per CMakeLists.txt" refactor, server/CMakeLists.txt
    # is a self-contained project that produces only catchchallenger-server-gui.
    # No -DCATCHCHALLENGER_BUILD_X gating flags needed.
    cmake_args = [
        "cmake", "-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=/usr/bin/ninja",
        "-S", os.path.join(ROOT, "server"), "-B", BUILD_DIR,
        f"-DCMAKE_C_COMPILER={cc}",
        f"-DCMAKE_CXX_COMPILER={cxx}",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold -fno-lto",
        "-DCMAKE_C_FLAGS_INIT=-fno-lto",
        "-DCMAKE_CXX_FLAGS_INIT=-fno-lto",
    ]
    log_info(f"cmake configure (build dir: {BUILD_DIR})")
    rc, out = run_cmd(cmake_args, BUILD_DIR, COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return False
    log_info(f"cmake --build -j{NPROC}")
    rc, out = run_cmd(["cmake", "--build", BUILD_DIR,
                       "--target", "catchchallenger-server-gui",
                       "-j", NPROC],
                      BUILD_DIR, COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"cmake build failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return False
    if not os.path.isfile(SERVER_BIN):
        log_fail(name, f"binary missing: {SERVER_BIN}")
        return False
    log_pass(name, f"-> {os.path.relpath(SERVER_BIN, ROOT)}")
    return True


def setup_runtime():
    """Stage datapack maincode=test next to the server binary, and wipe
    any leftover SQLite file so the auto-init path runs from scratch."""
    if not os.path.isdir(DATAPACK_SRC):
        log_fail(SCRIPT_RUN_NAME, f"datapack source not found: {DATAPACK_SRC}")
        return False
    main_main = os.path.join(DATAPACK_SRC, "map", "main", MAINCODE)
    if not os.path.isdir(main_main):
        log_fail(SCRIPT_RUN_NAME, f"maincode '{MAINCODE}' missing in {DATAPACK_SRC}")
        return False
    # Use a real copy (NOT a symlink): the server may write back into
    # datapack/ for cache files, and we don't want to mutate the
    # user-owned source tree. shutil.copytree handles the recursive copy.
    if os.path.islink(DATAPACK_DST) or os.path.isfile(DATAPACK_DST):
        os.remove(DATAPACK_DST)
    elif os.path.isdir(DATAPACK_DST):
        shutil.rmtree(DATAPACK_DST)
    log_info(f"copying datapack {DATAPACK_SRC} -> {DATAPACK_DST}")
    shutil.copytree(DATAPACK_SRC, DATAPACK_DST,
                    ignore=shutil.ignore_patterns(".git"))
    # Wipe any pre-existing SQLite files in the exe dir so the auto-init
    # in QtDatabaseSQLite::syncConnect always exercises the schema-load
    # path. SQLite default location is alongside the binary.
    exe_dir = os.path.dirname(SERVER_BIN)
    si = 0
    while si < 64:
        si += 1
        # The default DB filenames the Qt server uses in SQLite mode are
        # "catchchallenger.sqlite" / "catchchallenger-login.sqlite" /
        # ".db" — wipe anything matching either suffix once.
        break
    fi = 0
    listed = os.listdir(exe_dir)
    while fi < len(listed):
        fname = listed[fi]
        fi += 1
        if fname.endswith(".sqlite") or fname.endswith(".db"):
            try:
                os.remove(os.path.join(exe_dir, fname))
            except OSError:
                pass
    return True


def test_run():
    name = SCRIPT_RUN_NAME
    if not os.path.isfile(SERVER_BIN):
        # all.sh wipes the build dir between runs, so a "compile
        # passed previously" cache entry doesn't imply the binary
        # still exists. Rebuild on demand instead of failing the
        # whole script — ccache makes the compile cheap.
        log_info(f"binary missing: {SERVER_BIN} — rebuilding")
        if not test_compile():
            log_fail(name, f"binary missing and rebuild failed: {SERVER_BIN}")
            return False
    if not setup_runtime():
        return False
    # Headless run: catchchallenger-server-gui now uses QApplication (Qt
    # Widgets MainWindow), so we need the offscreen Qt platform — xcb
    # would refuse to start without DISPLAY. Strip the GUI envvars too
    # so a stale DISPLAY can't talk us into the xcb path.
    env = os.environ.copy()
    for var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(var, None)
    env["QT_QPA_PLATFORM"] = "offscreen"
    # --auto-quit-after lets the binary self-terminate cleanly after
    # printing Listen and finishing preload; --autostart triggers the
    # programmatic Start.  /usr/bin/timeout is the belt-and-braces
    # outer cap so a hang in Qt's offscreen platform / TLS / DB init
    # can never wedge the test session indefinitely (subprocess.Popen
    # + select.select on stdout was blocking past `deadline` because
    # readline() on a dead-but-not-EOF pipe waits forever).
    inner_quit_secs = max(RUN_TIMEOUT - 2, 3)
    outer_kill_secs = RUN_TIMEOUT + 5
    args = ["timeout", "--signal=KILL", str(outer_kill_secs),
            SERVER_BIN, "--autostart",
            f"--auto-quit-after={inner_quit_secs}"]
    log_info(f"running: {os.path.basename(SERVER_BIN)} "
             f"--autostart --auto-quit-after={inner_quit_secs} "
             f"(outer timeout {outer_kill_secs}s)")
    diagnostic.record_cmd(args, os.path.dirname(SERVER_BIN))
    proc = subprocess.Popen(
        args, cwd=os.path.dirname(SERVER_BIN), env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        bufsize=1, universal_newlines=True,
        start_new_session=True)
    deadline = time.monotonic() + RUN_TIMEOUT
    captured = []
    found = False
    try:
        while time.monotonic() < deadline:
            # readline blocks until a line lands OR the pipe closes.
            # With --auto-quit-after the child exits on its own; with
            # the outer `timeout` wrapper a stuck child still gets
            # killed before this loop's deadline fires, so readline
            # always returns within the test budget.
            line = proc.stdout.readline()
            if not line:
                if proc.poll() is not None:
                    break
                continue
            captured.append(line.rstrip("\n"))
            if LISTEN_MARKER in line:
                found = True
                # Don't break: drain remaining stdout so the child
                # (which is still running until --auto-quit-after
                # fires) doesn't get blocked on a full pipe buffer
                # while we wait for its clean exit below.
                break
    finally:
        # Once we've seen the marker (or the deadline elapsed) we don't
        # need to wait for --auto-quit-after to fire — SIGTERM the
        # child group right away so the next test can start. The outer
        # `timeout` wrapper and --auto-quit-after are still there as
        # last-resort guardrails if SIGTERM somehow doesn't take.
        if proc.poll() is None:
            try:
                os.killpg(proc.pid, 15)  # SIGTERM
            except (ProcessLookupError, OSError):
                pass
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(proc.pid, 9)
                except (ProcessLookupError, OSError):
                    pass
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    pass
    if found:
        log_pass(name, f"saw '{LISTEN_MARKER}' marker")
        return True
    log_fail(name, f"no '{LISTEN_MARKER}' marker within {RUN_TIMEOUT}s")
    li = max(0, len(captured) - 30)
    while li < len(captured):
        print(f"  | {captured[li]}")
        li += 1
    return False


def test_run_no_platform_plugin():
    # Headless box, QT_QPA_PLATFORM points at a plugin that does not
    # exist: Qt used to qFatal → abort() → SIGABRT + core dump inside
    # the QApplication ctor. main.cpp now installs a message handler
    # that turns that one fatal into a plain stderr line + clean
    # EXIT_FAILURE. Assert: process exits non-zero, is NOT a crash
    # (no SIGABRT/SIGSEGV), and prints the simple error.
    name = SCRIPT_NOPLUGIN_NAME
    if not os.path.isfile(SERVER_BIN):
        log_info(f"binary missing: {SERVER_BIN} — rebuilding")
        if not test_compile():
            log_fail(name, f"binary missing and rebuild failed: {SERVER_BIN}")
            return False
    env = os.environ.copy()
    for var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(var, None)
    # Deliberately bogus: no such platform plugin exists.
    env["QT_QPA_PLATFORM"] = "definitely-not-a-real-qpa-plugin"
    args = ["timeout", "--signal=KILL", str(RUN_TIMEOUT + 5),
            SERVER_BIN, "--autostart"]
    log_info(f"running: {os.path.basename(SERVER_BIN)} --autostart "
             f"with QT_QPA_PLATFORM={env['QT_QPA_PLATFORM']}")
    diagnostic.record_cmd(args, os.path.dirname(SERVER_BIN))
    try:
        p = subprocess.run(
            args, cwd=os.path.dirname(SERVER_BIN), env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=RUN_TIMEOUT + 8, start_new_session=True)
        rc = p.returncode
        out = p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if e.stdout else ""
        log_fail(name, f"binary hung instead of exiting on missing QPA plugin\n{out[-2000:]}")
        return False
    crashed = process_helpers.is_crash(rc)
    graceful_msg = "no usable Qt platform plugin" in out
    if crashed:
        log_fail(name,
                 f"crashed (rc={rc}, SIGABRT/SIGSEGV) instead of clean error exit\n{out[-2000:]}")
        return False
    if rc == 0:
        log_fail(name,
                 f"exited 0 with no usable platform plugin — should fail non-zero\n{out[-2000:]}")
        return False
    if not graceful_msg:
        log_fail(name,
                 f"no-crash but missing the simple error line (rc={rc})\n{out[-2000:]}")
        return False
    log_pass(name, f"clean non-zero exit (rc={rc}) with simple error, no SIGABRT")
    return True


# ── main ───────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Qt server (server-gui) testing")
    print(f"{'='*60}{C_RESET}\n")

    failed = load_failed_cases()
    if failed is not None and len(failed) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    compile_ok = True
    if should_run(SCRIPT_COMPILE_NAME, failed):
        compile_ok = test_compile()
    if compile_ok and should_run(SCRIPT_RUN_NAME, failed):
        test_run()
    if compile_ok and should_run(SCRIPT_NOPLUGIN_NAME, failed):
        test_run_no_platform_plugin()

    # summary
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed_count = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for nm, ok, dt, el in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {nm}  {dt}  ({el:.1f}s)")
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed_count} failed{C_RESET}")
    save_failed_cases()
    if failed_count:
        sys.exit(1)


if __name__ == "__main__":
    main()
