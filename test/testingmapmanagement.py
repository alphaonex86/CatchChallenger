#!/usr/bin/env python3
"""
testingmapmanagement.py — CatchChallenger map-visibility unit tests.

Builds and runs test/testingmapmanagement/testmapvisibility, a
self-contained binary that exercises every branch in:
  - server/base/MapManagement/MapVisibilityAlgorithm.cpp
      min_CPU()
      min_network()
      send_reinsertAll()
      send_reinsertAllWithFilter()
  - server/base/MapServer.cpp
      playerToFullInsert()

The binary stubs out the entire server stack (Client / ClientList /
ClientWithMap / GlobalServerData / ProtocolParsingBase /
CommonSettingsServer) via test/testingmapmanagement/Stubs.hpp, so it
makes ZERO syscalls (modulo libc init), allocates nothing on the
network, and never touches an event loop.

The C++ side also feeds every Client::sendRawBlock() byte stream
through a tiny mirror of client/libcatchchallenger/Api_protocol_message.cpp
that recognises packet codes 0x6C/0x65/0x6B/0x66/0x69/0xE3 and
dispatches to a MapControllerMP-shaped OtherPlayer dict. After every
tick the dict is diff'd against the server-side authoritative
map_clients_id (skipping clients whose pingCountInProgress()>0 — the
intentional stale-data window for buffer-saturation simulation).

Resource budget per the user's brief: 60s wall, 16 MiB RSS.
"""
import sys
sys.dont_write_bytecode = True

import os
import resource
import signal
import subprocess
import time

import diagnostic
import wall_cap
wall_cap.arm()
import build_paths
import cleanup_helpers
from cmd_helpers import clamp_local

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(os.cpu_count() or 1)

SUBDIR     = os.path.join(ROOT, "test/testingmapmanagement")
PRO        = os.path.join(SUBDIR, "testmapvisibility.pro")  # virtual; CMake source dir is SUBDIR
BUILD_DIR  = build_paths.build_path("test/testingmapmanagement/build/testing-mapmanagement" + _DIAG_SUFFIX)
cleanup_helpers.register_build_dir(BUILD_DIR)
BIN_NAME   = "testmapvisibility"

COMPILE_TIMEOUT = 300
# 60s budget per user brief. diagnostic.scale_timeout() multiplies under
# valgrind so the cap still leaves room.
RUN_TIMEOUT     = 60

# 16 MiB peak RSS per user brief. RLIMIT_AS would also include the
# loader / libc / libstdc++ shared mappings (~30 MiB on x86_64 alone)
# and the exec'd binary couldn't even start. Instead we measure peak
# RSS via getrusage(RUSAGE_CHILDREN) after the child exits and fail
# the run if it exceeded the cap. (Skipped under valgrind, which
# inflates RSS 5-10×.)
RUN_PEAK_RSS_CAP_KB = 16 * 1024

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]
NICE_PREFIX_RUNTIME = []

C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer

# Internal wall-time cap matched 1:1 with test/all.sh's
# PER_TEST_TIMEOUT_MAP; bumping either side must touch both, per the
# test/CLAUDE.md contract. RUN_TIMEOUT (60s) bounds the test binary
# itself; this wall covers build + run + slow-host margin.
_WALL_LIMIT_SEC = 600
import faulthandler
faulthandler.enable()
faulthandler.dump_traceback_later(_WALL_LIMIT_SEC + 10, exit=False)
try:
    faulthandler.register(signal.SIGUSR1)
except (AttributeError, RuntimeError):
    pass

def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
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
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


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
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


def _apply_child_rlimits():
    """preexec_fn for the test child: 60s CPU. Memory cap is enforced
    post-exit via /usr/bin/time -v parse (RLIMIT_AS would block exec
    of even a minimal C++ binary because shared lib mappings alone
    exceed 16 MiB on x86_64). Gated on --valgrind absence."""
    try:
        if not diagnostic.is_valgrind(DIAG):
            resource.setrlimit(resource.RLIMIT_CPU,
                               (RUN_TIMEOUT, RUN_TIMEOUT + 30))
    except Exception as exc:
        try:
            os.write(2, f"[testingmapmanagement] preexec rlimit failed: {exc!r}\n".encode())
        except Exception:
            pass


def run_binary():
    binary = os.path.join(BUILD_DIR, BIN_NAME)
    if not os.path.isfile(binary):
        log_fail("binary", f"missing: {binary}")
        return False
    timeout = diagnostic.scale_timeout(DIAG, RUN_TIMEOUT)
    wrapper = diagnostic.runtime_wrapper(DIAG)
    # /usr/bin/time -v prints "Maximum resident set size (kbytes): N"
    # on stderr after the binary exits — most accurate way to measure
    # this single child's peak RSS without conflating with other
    # children of this Python process (RUSAGE_CHILDREN.ru_maxrss is a
    # global high-water across every reaped child, so cmake/cc1plus
    # peaks contaminate it).
    time_bin = "/usr/bin/time"
    use_time = os.path.isfile(time_bin) and not diagnostic.is_valgrind(DIAG)
    if use_time:
        cmd = NICE_PREFIX_RUNTIME + [time_bin, "-v"] + wrapper + [binary]
    else:
        cmd = NICE_PREFIX_RUNTIME + wrapper + [binary]
    diagnostic.record_cmd(cmd, BUILD_DIR)
    log_info(f"running: {BIN_NAME} (timeout={timeout}s, peak_rss_cap={RUN_PEAK_RSS_CAP_KB} KiB)")
    started = time.monotonic()
    try:
        p = subprocess.run(cmd, cwd=BUILD_DIR,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           timeout=clamp_local(timeout),
                           preexec_fn=_apply_child_rlimits)
    except subprocess.TimeoutExpired:
        log_fail("run", f"timeout after {timeout}s")
        return False
    elapsed = time.monotonic() - started
    # /usr/bin/time -v writes its report to stderr — parse the peak
    # RSS from there. stdout still holds the test binary's own output.
    err_text = p.stderr.decode(errors="replace") if p.stderr else ""
    peak_rss_kb = -1
    if use_time:
        for line in err_text.splitlines():
            line = line.strip()
            if line.startswith("Maximum resident set size"):
                try:
                    peak_rss_kb = int(line.split(":")[-1].strip())
                except ValueError:
                    pass
                break
    out = p.stdout.decode(errors="replace")
    for line in out.splitlines():
        print(f"  | {line}")

    any_fail = False
    for raw in out.splitlines():
        line = raw.strip()
        if line.startswith("PASS "):
            rest = line[5:].split(" ", 1)
            name = rest[0]
            detail = rest[1] if len(rest) > 1 else ""
            log_pass(name, detail)
        elif line.startswith("FAIL "):
            rest = line[5:].split(" ", 1)
            name = rest[0]
            detail = rest[1] if len(rest) > 1 else ""
            log_fail(name, detail)
            any_fail = True

    if p.returncode != 0:
        log_fail("exit", f"binary exit code {p.returncode} (elapsed {elapsed:.2f}s)")
        any_fail = True
    else:
        log_info(f"binary exited 0 in {elapsed:.2f}s, peak_rss={peak_rss_kb} KiB")
    if not diagnostic.is_valgrind(DIAG) and peak_rss_kb > RUN_PEAK_RSS_CAP_KB:
        log_fail("memory_budget",
                 f"peak_rss={peak_rss_kb} KiB > cap={RUN_PEAK_RSS_CAP_KB} KiB")
        any_fail = True
    return not any_fail


def cleanup(*_args):
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


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


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — MapManagement Unit Tests")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    if not build_project(PRO, BUILD_DIR, BIN_NAME):
        summary()
        return

    run_binary()
    summary()


if __name__ == "__main__":
    main()
