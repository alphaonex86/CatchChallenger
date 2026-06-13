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
  4. Additionally, when a project carries a sibling CMakePresets.json,
     its buildPresets are statically sanity-checked. Configure (steps
     1-2) never exercises `cmake --build --preset`, but that IS Qt
     Creator's Build step — and a malformed build preset fails only
     there, invisibly to a `cmake -S/-B` configure. The one that bit
     us: `"jobs": 0`, which Qt Creator turns into `cmake --build <dir>
     --target all -j 0`, rejected by cmake ("The <jobs> value requires
     a positive integer argument"). So the project configured fine yet
     would not build out of the box in Qt Creator.

Args mirror the rest of the suite (--sanitize/--valgrind etc. are
forwarded but only used for diagnostics — configure timing isn't
sensitive to them).
"""

import sys
sys.dont_write_bytecode = True

import os, subprocess, multiprocessing, json, time, shutil
import diagnostic
import wall_cap
wall_cap.arm()
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
import phase_timer


def log_pass(label, secs=0.0):
    print(f"{phase_timer.t()}   [{C_GREEN}PASS{C_RESET}] {label}  ({secs:.1f}s)")
    phase_timer.record_event("pass", label, ok=True, dt=secs)

def log_fail(label, why, secs=0.0):
    print(f"{phase_timer.t()}   [{C_RED}FAIL{C_RESET}] {label}  {why}  ({secs:.1f}s)")
    phase_timer.record_event("fail", label, ok=False, dt=secs, detail=why)


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
        # ESP-IDF projects (server/cli/esp32) are NOT vanilla-cmake
        # configurable: they `include($ENV{IDF_PATH}/tools/cmake/
        # project.cmake)` and must be built via idf.py with IDF_PATH set.
        # Without ESP-IDF the include resolves to /tools/cmake/project.cmake
        # and configure fails. They are covered by testingcompilationESP32.py
        # (which self-skips when ESP-IDF is absent), so exclude them from the
        # "configures with plain cmake -S/-B" contract checked here.
        if "IDF_PATH" in content or "tools/cmake/project.cmake" in content:
            continue
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


def validate_build_presets(src_dir):
    """Static sanity-check of a project's CMakePresets.json buildPresets.

    Returns (ok, message). No CMakePresets.json -> (True, "").

    cmake maps a build preset's `jobs` straight onto `-j<jobs>`; anything
    but a positive integer makes `cmake --build --preset` (the command Qt
    Creator's Build step runs) fail. The repo convention is to omit `jobs`
    so the native build tool picks its own default parallelism — this
    catches a re-introduced `"jobs": 0` before it reaches a developer's
    Qt Creator."""
    presets = os.path.join(src_dir, "CMakePresets.json")
    if not os.path.isfile(presets):
        return (True, "")
    try:
        with open(presets) as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        return (False, f"CMakePresets.json is not valid JSON: {e}")
    build_presets = data.get("buildPresets", [])
    if not isinstance(build_presets, list):
        return (False, 'CMakePresets.json: "buildPresets" must be a list')
    bad = []
    for bp in build_presets:
        if isinstance(bp, dict) and "jobs" in bp:
            jobs = bp["jobs"]
            # bool is a subclass of int in Python — reject true/false too.
            if isinstance(jobs, bool) or not isinstance(jobs, int) or jobs < 1:
                bad.append(f'buildPreset "{bp.get("name", "?")}" has jobs={jobs!r}')
    if bad:
        return (False,
                "CMakePresets.json buildPreset jobs invalid "
                "(cmake --build --preset / Qt Creator Build fails with "
                "'-j <bad>'): " + "; ".join(bad)
                + ' — omit "jobs" for native default, or use a positive integer')
    return (True, "")


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
    # Configure succeeded — also gate on the build presets being usable,
    # so the "opens & builds in Qt Creator" contract is verified, not just
    # the `cmake -S/-B` configure path.
    preset_ok, preset_msg = validate_build_presets(src_dir)
    if not preset_ok:
        return (label, False, preset_msg, elapsed)
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

    # failed.json bookkeeping — same dict shape as the other testing*.py.
    # Each failure entry carries `cmd` (the cmake command that failed) +
    # `error` (last lines of cmake stderr, already in r[2]).
    import failed_cases as _fc
    failure_entries = []
    for label, ok, msg, _secs in failed:
        cmd = (f"cmake -S {os.path.relpath(os.path.dirname(label), ROOT) if os.path.isabs(label) else label}"
               f" -B <fresh build dir> -DCMAKE_BUILD_TYPE=Debug")
        # make_detail() has no `error` kwarg; the cmake configure output
        # belongs in `compile_output` (msg is already the detail string).
        # The old error=msg raised TypeError and crashed the whole script
        # instead of recording the failing project in failed.json.
        failure_entries.append(
            (label, _fc.make_detail(msg, cmd=cmd, compile_output=msg))
        )
    _fc.save(SCRIPT_NAME, failure_entries)

    sys.exit(0 if not failed else 1)


if __name__ == "__main__":
    main()
