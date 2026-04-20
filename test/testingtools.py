#!/usr/bin/env python3
"""
testingtools.py — Compile every .pro in tools/ and its subdirectories.

For each .pro found:
  1. Create a temporary build directory
  2. qmake + make
  3. make distclean to clean up
"""

import os, sys, subprocess, multiprocessing, json

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

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    results.append((name, True, detail))
    print(f"{C_GREEN}[PASS]{C_RESET} {name}  {detail}")


def log_fail(name, detail=""):
    results.append((name, False, detail))
    print(f"{C_RED}[FAIL]{C_RESET} {name}  {detail}")


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT):
    try:
        p = subprocess.run(
            args, cwd=cwd, timeout=timeout,
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
                  "-spec", "linux-g++", "CONFIG+=debug", "CONFIG+=qml_debug"]
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

    for pf in pro_files:
        test_compile(pf)
        print()

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
