#!/usr/bin/env python3
"""
testingcmake.py — verify every CMakeLists.txt in the repo configures as
a standalone CMake project.

Per CLAUDE.md "CMake project layout — one binary per CMakeLists.txt":
each binary subdir owns a self-contained CMakeLists.txt that opens
directly in Qt Creator / `cmake -S <subdir> -B <build>`. Library
fragments live in *.cmake includes, not CMakeLists.txt — so every
CMakeLists.txt with a `project(...)` declaration is expected to be
configurable on its own.

This script:
  1. Walks the tree, picks up every CMakeLists.txt with a top-level
     `project(...)` line (i.e. standalone projects, library sub-libs
     are excluded since they're INTERFACE/STATIC libs included via
     add_subdirectory or include()).
  2. For each one, runs `cmake -S <dir> -B <fresh_tmpfs_dir>` and
     reports PASS/FAIL.
  3. Configure-only on purpose — full builds of every target are
     covered by the other testing*.py scripts. The point here is to
     catch breakage of the standalone-open contract early (missing
     include() path, missing fragment, accidental cross-CMakeLists
     dependency).

Args mirror the rest of the suite (--sanitize/--valgrind etc. are
forwarded but only used for diagnostics — configure timing isn't
sensitive to them).
"""

import sys
sys.dont_write_bytecode = True

import os, subprocess, multiprocessing, json, time, shutil
import diagnostic
import build_paths
from cmd_helpers import clamp_local

build_paths.ensure_root()
DIAG = diagnostic.parse_diag_args()

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())
NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

CONFIG_TIMEOUT = 120  # cmake -S/-B per project; configure only

C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

SCRIPT_NAME = os.path.basename(__file__)
from test_config import FAILED_JSON, TMPFS_BUILD_ROOT


def log_pass(label, secs=0.0):
    print(f"  [{C_GREEN}PASS{C_RESET}] {label}  ({secs:.1f}s)")

def log_fail(label, why, secs=0.0):
    print(f"  [{C_RED}FAIL{C_RESET}] {label}  {why}  ({secs:.1f}s)")


def discover_standalone_cmakelists():
    """Find every CMakeLists.txt with a top-level project() declaration.
    Skip anything under a build/ tree to avoid configuring artefacts."""
    out = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        # Prune walk: don't descend into build dirs / hidden dirs / git.
        dirnames[:] = [
            d for d in dirnames
            if d not in ("build", ".git", "__pycache__")
               and not d.startswith(".qtcreator")
        ]
        if "CMakeLists.txt" not in filenames:
            continue
        fp = os.path.join(dirpath, "CMakeLists.txt")
        with open(fp) as f:
            content = f.read()
        # The check is "starts with `project(` after stripping the
        # leading `cmake_minimum_required(...)` line"; every standalone
        # has that, library .cmake fragments don't.
        for ln in content.splitlines():
            s = ln.strip()
            if s.startswith("#") or s == "":
                continue
            if s.startswith("project("):
                out.append(fp)
                break
            # Stop scanning once we hit non-comment, non-project content
            # that isn't cmake_minimum_required — keeps us from finding
            # `project(` deep inside conditional bodies.
            if not s.startswith("cmake_minimum_required"):
                # Allow blank lines + comments + cmake_minimum_required
                # before project(). Anything else and we move on.
                # But still scan ahead for project() on a later line —
                # config blocks like `set(...)` are uncommon at the very
                # top, but harmless to keep scanning.
                pass
    return sorted(out)


def configure_one(cmakelists_path):
    """Configure a single CMakeLists.txt into a fresh build dir.
    Returns (label, ok, message, elapsed_seconds)."""
    src_dir = os.path.dirname(cmakelists_path)
    rel = os.path.relpath(src_dir, ROOT)
    label = rel if rel != "." else "."
    # Per-project build dir under tmpfs build root, namespaced by the
    # source-relative path with slashes flattened.
    safe = rel.replace("/", "_").replace(os.sep, "_") or "root"
    build_dir = os.path.join(TMPFS_BUILD_ROOT, "cmake-standalone", safe)
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir, ignore_errors=True)
    os.makedirs(build_dir, exist_ok=True)
    cmd = NICE_PREFIX + [
        "cmake", "-S", src_dir, "-B", build_dir,
        "-DCMAKE_BUILD_TYPE=Debug",
    ]
    t0 = time.monotonic()
    try:
        proc = subprocess.run(
            cmd, capture_output=True, text=True,
            timeout=clamp_local(CONFIG_TIMEOUT),
        )
    except subprocess.TimeoutExpired:
        return (label, False, f"cmake configure timed out (>{CONFIG_TIMEOUT}s)",
                time.monotonic() - t0)
    elapsed = time.monotonic() - t0
    if proc.returncode != 0:
        # Trim stderr to last few lines so the failure message stays
        # readable. Full output already went to the build dir's
        # CMakeFiles/CMakeError.log if cmake produced one.
        tail = "\n    ".join((proc.stderr or proc.stdout or "").rstrip().splitlines()[-8:])
        return (label, False, f"cmake exit {proc.returncode}\n    {tail}", elapsed)
    return (label, True, "", elapsed)


def main():
    # Resume support — failed.json says "only retry these" when present.
    failed_cases = None
    if os.path.isfile(FAILED_JSON):
        try:
            with open(FAILED_JSON) as f:
                data = json.load(f)
            if SCRIPT_NAME in data:
                failed_cases = data[SCRIPT_NAME]
        except (json.JSONDecodeError, IOError):
            pass

    cmakelists = discover_standalone_cmakelists()
    if failed_cases is not None:
        cmakelists = [
            cl for cl in cmakelists
            if os.path.relpath(os.path.dirname(cl), ROOT) in failed_cases
        ]

    print(f"\n{C_CYAN}========================================{C_RESET}")
    print(f"{C_CYAN}  CMakeLists.txt standalone-configure check{C_RESET}")
    print(f"{C_CYAN}========================================{C_RESET}\n")
    print(f"  {len(cmakelists)} project(s) to verify:")
    for cl in cmakelists:
        print(f"    - {os.path.relpath(cl, ROOT)}")
    print()

    # Configures are independent — run them in parallel. Each cmake
    # configure is single-threaded internally so we can run as many at
    # once as cores. Cap at 8 to leave headroom for the rest of all.sh.
    pool_size = min(8, len(cmakelists)) or 1
    t_start = time.monotonic()
    results = []
    if pool_size > 1:
        with multiprocessing.Pool(pool_size) as pool:
            for r in pool.imap_unordered(configure_one, cmakelists):
                results.append(r)
                label, ok, msg, secs = r
                if ok:
                    log_pass(f"configure {label}", secs)
                else:
                    log_fail(f"configure {label}", msg, secs)
    else:
        for cl in cmakelists:
            r = configure_one(cl)
            results.append(r)
            label, ok, msg, secs = r
            if ok:
                log_pass(f"configure {label}", secs)
            else:
                log_fail(f"configure {label}", msg, secs)
    total = time.monotonic() - t_start

    print(f"\n{C_CYAN}============================================================{C_RESET}")
    print(f"{C_CYAN}  Summary{C_RESET}")
    print(f"{C_CYAN}============================================================{C_RESET}")
    passed = [r for r in results if r[1]]
    failed = [r for r in results if not r[1]]
    print(f"  total elapsed: {total:.1f}s")
    print(f"  {C_GREEN}{len(passed)} passed{C_RESET}, {C_RED}{len(failed)} failed{C_RESET}\n")

    # failed.json bookkeeping — same pattern as other testing*.py
    data = {}
    if os.path.isfile(FAILED_JSON):
        try:
            with open(FAILED_JSON) as f:
                data = json.load(f)
        except (json.JSONDecodeError, IOError):
            data = {}
    if failed:
        data[SCRIPT_NAME] = [r[0] for r in failed]
    else:
        data.pop(SCRIPT_NAME, None)
    if data:
        with open(FAILED_JSON, "w") as f:
            json.dump(data, f, indent=2)
    elif os.path.isfile(FAILED_JSON):
        # Don't delete the file if other scripts have entries; but if it's
        # empty post-pop, write an empty object so the file shape stays valid.
        with open(FAILED_JSON, "w") as f:
            json.dump(data, f, indent=2)

    sys.exit(0 if not failed else 1)


if __name__ == "__main__":
    main()
