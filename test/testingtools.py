#!/usr/bin/env python3
"""
testingtools.py — Compile every .pro in tools/ and its subdirectories.

For each .pro found:
  1. Create a temporary build directory
  2. qmake + make
  3. make distclean to clean up
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, subprocess, multiprocessing, json, time
from remote_build import (start_remote_builds, collect_remote_results,
                          count_remote_tests)
import diagnostic
import build_paths
from cmd_helpers import clamp_local

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT      = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_DIR = os.path.join(ROOT, "tools")
QMAKE     = _config["qmake"]
NPROC     = str(multiprocessing.cpu_count())

COMPILE_TIMEOUT = 600

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


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(
            NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def clean_build_artifacts(build_dir):
    if not os.path.isdir(build_dir):
        return
    import glob as _glob
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in _glob.glob(os.path.join(build_dir, pattern)):
            os.remove(f)


def find_pro_files():
    """Return the legacy .pro paths for every tool that has a CMake target
    in cmake_helpers._PRO_TO_CMAKE. The .pro files no longer exist on disk
    after the qmake -> CMake migration, but the build_project driver keys
    its target lookup on those paths, so we still pass them through."""
    import cmake_helpers as _ch
    out = []
    for pro_rel in sorted(_ch._PRO_TO_CMAKE.keys()):
        if not pro_rel.startswith("tools/"):
            continue
        out.append(os.path.join(ROOT, pro_rel))
    return out


def test_compile(pro_file):
    """Build a .pro file via cmake. Returns True on success."""
    import cmake_helpers as _ch
    rel = os.path.relpath(pro_file, ROOT)
    pro_dir = os.path.dirname(pro_file)
    suffix = diagnostic.build_dir_suffix(DIAG)
    build_dir = build_paths.src_to_build(pro_dir, ROOT, "build", "testing" + suffix)
    os.makedirs(build_dir, exist_ok=True)

    label = rel + diagnostic.label_suffix(DIAG)

    ok = _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=lambda d: os.makedirs(d, exist_ok=True),
        run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )
    if not ok:
        # Wipe so retry starts clean (matches old `make distclean` on fail).
        import shutil
        shutil.rmtree(build_dir, ignore_errors=True)
        return False
    return True


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Tools Compilation Testing")
    print(f"{'='*60}{C_RESET}\n")

    pro_files = find_pro_files()
    if not pro_files:
        print("No .pro files found under tools/")
        sys.exit(1)

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete test/failed.json for full re-run)")
        return

    # clean all tool build dirs upfront
    for pf in pro_files:
        build_dir = build_paths.src_to_build(os.path.dirname(pf), ROOT, "build", "testing")
        clean_build_artifacts(build_dir)

    log_info(f"found {len(pro_files)} .pro file(s) in tools/\n")
    for pf in pro_files:
        print(f"  {os.path.relpath(pf, ROOT)}")
    print()

    # start remote builds in parallel with local builds
    remote_pro_rels = [
        "tools/stats/stats.pro",
        "tools/map2png/map2png.pro",
        "tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro",
    ]
    # total = local builds + remote (rsync per server + builds per server)
    # Remote count comes from the same per-node iteration start_remote_builds
    # uses, so disabled/added nodes update the total automatically.
    remote_build_count = count_remote_tests(remote_pro_rels, diag=DIAG)
    total_expected[0] = len(pro_files) + remote_build_count
    log_info(f"total expected: {total_expected[0]} ({len(pro_files)} local + {remote_build_count} remote)")

    if diagnostic.is_active(DIAG):
        log_info(f"diagnostic mode: {diagnostic.describe(DIAG)}")
    if failed_cases is None:
        remote_threads, remote_results, remote_lock = start_remote_builds(
            remote_pro_rels, diag=DIAG)
        log_info("remote builds started in background")
    else:
        remote_threads, remote_results, remote_lock = [], [], None
        log_info("resume mode: skipping remote builds")

    for pf in pro_files:
        rel = os.path.relpath(pf, ROOT)
        if should_run(f"compile {rel}", failed_cases):
            test_compile(pf)
        print()

# collect remote results
    if failed_cases is None:
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

    # summary
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
