#!/usr/bin/env python3
"""
testingtools.py — Compile every .pro in tools/ and its subdirectories.

For each .pro found:
  1. Create a temporary build directory
  2. qmake + make
  3. make distclean to clean up
"""

import os, sys, subprocess, multiprocessing, json
from remote_build import start_remote_builds, collect_remote_results

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
total_expected = [0]


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    results.append((name, True, detail))
    print(f"{C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}")


def log_fail(name, detail=""):
    results.append((name, False, detail))
    print(f"{C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}")


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
    build_dir = os.path.join(pro_dir, "build", "testing")
    os.makedirs(build_dir, exist_ok=True)
    clean_build_artifacts(build_dir)
    log_info(f"make distclean {rel}")
    run_cmd(["make", "distclean"], build_dir, timeout=60)

    label = rel

    # qmake
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", "linux-g++", "CONFIG+=debug", "CONFIG+=qml_debug",
                  "QMAKE_CXXFLAGS+=-g", "QMAKE_CFLAGS+=-g", "QMAKE_LFLAGS+=-g",
                  "QMAKE_LFLAGS+=-fuse-ld=mold", "LIBS+=-fuse-ld=mold"]
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
    from remote_build import REMOTE_SERVERS, GUI_PRO_FILES
    remote_build_count = 0
    ri = 0
    while ri < len(REMOTE_SERVERS):
        remote_build_count += 1  # rsync
        si = 0
        while si < len(remote_pro_rels):
            if REMOTE_SERVERS[ri][5] or remote_pro_rels[si] not in GUI_PRO_FILES:
                remote_build_count += 1
            si += 1
        ri += 1
    total_expected[0] = len(pro_files) + remote_build_count
    log_info(f"total expected: {total_expected[0]} ({len(pro_files)} local + {remote_build_count} remote)")

    remote_threads, remote_results, remote_lock = start_remote_builds(remote_pro_rels)
    log_info("remote builds started in background")

    for pf in pro_files:
        test_compile(pf)
        print()

    # collect remote results
    log_info("waiting for remote builds to finish...")
    remote = collect_remote_results(remote_threads, remote_results, remote_lock)
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
    passed = sum(1 for _, ok, _ in results if ok)
    failed = sum(1 for _, ok, _ in results if not ok)
    for name, ok, detail in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
