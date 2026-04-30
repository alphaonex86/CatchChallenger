#!/usr/bin/env python3
"""
testingtools.py — Compile every .pro in tools/ and its subdirectories.

For each .pro found:
  1. Create a temporary build directory
  2. qmake + make
  3. make distclean to clean up
"""

import os, sys, subprocess, multiprocessing, json, time
from remote_build import (start_remote_builds, collect_remote_results,
                          SERVER_MAKE_SCRIPTS, build_server_via_make,
                          compare_qmake_make, count_remote_tests)
import diagnostic

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


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT):
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
    """Recursively find all .pro files under tools/."""
    pro_files = []
    for dirpath, _dirs, files in os.walk(TOOLS_DIR):
        for f in sorted(files):
            if f.endswith(".pro"):
                pro_files.append(os.path.join(dirpath, f))
    return pro_files


def test_compile(pro_file):
    """Build a .pro file, then distclean.  Returns True on success."""
    rel = os.path.relpath(pro_file, ROOT)
    pro_dir = os.path.dirname(pro_file)
    pro_name = os.path.basename(pro_file)

    # build in a subdirectory to avoid polluting the source tree
    suffix = diagnostic.build_dir_suffix(DIAG)
    build_dir = os.path.join(pro_dir, "build", "testing" + suffix)
    os.makedirs(build_dir, exist_ok=True)
    log_info(f"make distclean {rel}")
    run_cmd(["make", "distclean"], build_dir, timeout=60)
    clean_build_artifacts(build_dir)

    label = rel + diagnostic.label_suffix(DIAG)

    # qmake
    spec = diagnostic.compiler_spec(DIAG) or "linux-g++"
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", spec, "CONFIG+=debug", "CONFIG+=qml_debug",
                  "QMAKE_CXXFLAGS+=-g", "QMAKE_CFLAGS+=-g", "QMAKE_LFLAGS+=-g",
                  "QMAKE_LFLAGS+=-fuse-ld=mold", "LIBS+=-fuse-ld=mold"]
    qmake_args.extend(diagnostic.qmake_extra_args(DIAG))
    log_info(f"qmake {label}")
    rc, out = run_cmd(qmake_args, build_dir)
    if rc != 0:
        log_fail(f"compile {label}", f"qmake failed (rc={rc})")
        if out.strip():
            for line in out.splitlines()[-15:]:
                print(f"  | {line}")
        # still try to clean up
        run_cmd(["make", "distclean"], build_dir, timeout=60)
        return False

    # make
    log_info(f"make -j{NPROC} {label}")
    rc, out = run_cmd(["make", f"-j{NPROC}"], build_dir)
    if rc != 0:
        log_fail(f"compile {label}", f"make failed (rc={rc})")
        if out.strip():
            for line in out.splitlines()[-20:]:
                print(f"  | {line}")
        # clean up
        run_cmd(["make", "distclean"], build_dir, timeout=60)
        return False

    log_pass(f"compile {label}")

    # distclean
    log_info(f"make distclean {label}")
    run_cmd(["make", "distclean"], build_dir, timeout=60)

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
        build_dir = os.path.join(os.path.dirname(pf), "build", "testing")
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

    # ── make.py build + qmake-vs-make.py parity check (for tools with make.py)
    log_info("tools with make.py: build + parity check")
    for pf in pro_files:
        rel = os.path.relpath(pf, ROOT)
        if rel not in SERVER_MAKE_SCRIPTS:
            continue
        tool_dir = os.path.dirname(pf)
        suffix = diagnostic.build_dir_suffix(DIAG)
        bd_make = os.path.join(tool_dir, "build", "testing-make.py" + suffix)
        bd_q = os.path.join(tool_dir, "build", "cmp-qmake" + suffix)
        bd_m = os.path.join(tool_dir, "build", "cmp-make" + suffix)
        label = os.path.basename(tool_dir)
        # build
        test_name = f"compile {rel} (make.py)"
        if should_run(test_name, failed_cases):
            log_info(f"make.py {label}")
            rc, out = build_server_via_make(rel, bd_make, label, diag=DIAG)
            if rc == 0:
                log_pass(test_name)
            else:
                log_fail(test_name, f"rc={rc}")
                if out.strip():
                    for line in out.splitlines()[-20:]:
                        print(f"  | {line}")
        # parity
        cmp_results = compare_qmake_make(rel, bd_q, bd_m, label=label)
        ri = 0
        while ri < len(cmp_results):
            name, ok, detail = cmp_results[ri]
            if should_run(name, failed_cases):
                if ok:
                    log_pass(name, detail)
                else:
                    log_fail(name, detail)
            ri += 1

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
