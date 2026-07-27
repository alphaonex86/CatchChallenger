#!/usr/bin/env python3
"""
testingoverworld.py — monster overworld sheet unit + image test.

Builds and runs test/testingoverworld/testoverworld, a self-contained binary
that exercises the REAL client/libqtcatchchallenger/maprender/MonsterSheet.cpp,
which owns the two accepted layouts of monsters/<id>/overworld.png:

  skin layout    48x96, 3 columns x 4 rows of 16x24 — the SAME sheet as
                 skin/<x>/trainer.png, so a trainer.png can be copied over as
                 a monster sprite unchanged; rows top/right/bottom/left,
                 columns walk/idle/walk, so 3 frames per direction.
  monster layout 64x128, 2 columns x 4 rows of 32x32 — top-walk/left-walk,
                 top-idle/left-idle, bottom-walk/right-walk, bottom-idle/
                 right-idle, so 1 walk frame per direction.

The binary asserts the geometry of both, then walks every direction and every
animation step the way the renderer does and checks that EVERY frame of the
sheet is reached — a dropped frame is the regression this test exists for. The
tiles it picks are blitted exactly like MapRenderer::drawTile() into one
contact sheet, compared here pixel by pixel (image_compare, +/-10% per channel)
against test/overworld-test.png. A wrong tile index AND a wrong sprite on
screen both fail.

Inputs are two long-standing datapack files, so the test does not depend on any
particular monster having an overworld sprite.

No network, no event-loop exec, no DB — reads two PNGs and exits well under a
second.
"""
import sys
sys.dont_write_bytecode = True

import os
import resource
import signal
import shutil
import subprocess
import time

import diagnostic
import wall_cap
wall_cap.arm()
import build_paths
import cleanup_helpers
from cmd_helpers import clamp_local
from image_compare import compare_with_reference

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
# Same rationale as testingpathfinding.py: this is a tiny binary around real
# client code that exits in well under a second, so run it under valgrind
# memcheck BY DEFAULT. An explicit --sanitize / --valgrind / --profile still
# wins; CC_NO_VALGRIND=1 opts out (and it self-skips if valgrind is absent).
if not DIAG and os.environ.get("CC_NO_VALGRIND", "") == "" \
        and shutil.which("valgrind") is not None:
    DIAG = {"mode": "valgrind", "tool": "memcheck"}
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(os.cpu_count() or 1)

SUBDIR    = os.path.join(ROOT, "test/testingoverworld")
PRO       = os.path.join(SUBDIR, "testoverworld.pro")  # virtual; CMake source dir is SUBDIR
BUILD_DIR = build_paths.build_path("test/testingoverworld/build/testing-overworld" + _DIAG_SUFFIX)
cleanup_helpers.register_build_dir(BUILD_DIR)
BIN_NAME  = "testoverworld"

DATAPACK        = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
REFERENCE_IMAGE = os.path.join(ROOT, "test", "overworld-test.png")
OUTPUT_IMAGE    = os.path.join(BUILD_DIR, "overworld-out.png")
DIFF_IMAGE      = os.path.join(BUILD_DIR, "overworld-diff.png")

COMPILE_TIMEOUT = 600
RUN_TIMEOUT     = 60

# Qt Core/Gui + libtiled are linked in, so the resident set is far larger than a
# libc-only binary; generous ceiling that still catches a runaway allocation.
# Skipped under valgrind (inflates RSS 5-10x).
RUN_PEAK_RSS_CAP_KB = 384 * 1024

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

# Internal wall-time cap matched 1:1 with test/all.sh's PER_TEST_TIMEOUT_MAP;
# bumping either side must touch both, per the test/CLAUDE.md contract.
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
    """preexec_fn for the test child: CPU cap only. Memory is checked post-exit
    via /usr/bin/time -v (RLIMIT_AS would block exec of even a minimal Qt
    binary). Gated on --valgrind absence."""
    try:
        if not diagnostic.is_valgrind(DIAG):
            resource.setrlimit(resource.RLIMIT_CPU,
                               (RUN_TIMEOUT, RUN_TIMEOUT + 30))
    except Exception as exc:
        try:
            os.write(2, f"[testingoverworld] preexec rlimit failed: {exc!r}\n".encode())
        except Exception:
            pass


def process_is_sigabrt(rc):
    return rc in (-signal.SIGABRT, 128 + int(signal.SIGABRT))


def run_binary():
    binary = os.path.join(BUILD_DIR, BIN_NAME)
    if not os.path.isfile(binary):
        log_fail("binary", f"missing: {binary}")
        return False
    if not os.path.isdir(DATAPACK):
        log_fail("datapack", f"missing: {DATAPACK}")
        return False
    try:
        os.remove(OUTPUT_IMAGE)
    except OSError:
        pass
    timeout = diagnostic.scale_timeout(DIAG, RUN_TIMEOUT)
    wrapper = diagnostic.runtime_wrapper(DIAG)
    time_bin = "/usr/bin/time"
    use_time = os.path.isfile(time_bin) and not diagnostic.is_valgrind(DIAG)
    args = [binary, DATAPACK, OUTPUT_IMAGE]
    if use_time:
        cmd = NICE_PREFIX_RUNTIME + [time_bin, "-v"] + wrapper + args
    else:
        cmd = NICE_PREFIX_RUNTIME + wrapper + args
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

    if p.returncode and process_is_sigabrt(p.returncode):
        log_fail("abort", f"binary SIGABRT (rc={p.returncode}); see stderr above")
        if err_text:
            for line in err_text.splitlines()[-20:]:
                print(f"  ! {line}")

    any_fail = False
    for raw in out.splitlines():
        line = raw.strip()
        if line.startswith("PASS "):
            rest = line[5:].split(" ", 1)
            log_pass(rest[0], rest[1] if len(rest) > 1 else "")
        elif line.startswith("FAIL "):
            rest = line[5:].split(" ", 1)
            log_fail(rest[0], rest[1] if len(rest) > 1 else "")
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

    # The contact sheet is what the renderer would actually put on screen: a
    # wrong tile index shows up here even when every index is "in range".
    ok, detail = compare_with_reference("monster overworld contact sheet",
                                        OUTPUT_IMAGE, REFERENCE_IMAGE, DIFF_IMAGE)
    if ok:
        log_pass("image", detail)
    else:
        log_fail("image", detail)
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
    print("  CatchChallenger — Monster Overworld Sheet Test")
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
