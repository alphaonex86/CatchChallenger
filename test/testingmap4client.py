#!/usr/bin/env python3
"""
testingmap4client.py — qtopengl-client first-frame screenshot regression.

What it does, in three phases:
  1. Compile the qtopengl client with CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON
     so the in-process server is embedded (--autosolo path).
  2. Stage the test datapack at <build>/datapack/internal/, keeping only
     the "test" maincode (the same intentionally-broken fixture used by
     other testing*.py runs, see CLAUDE.md).
  3. Run the client headless under QT_QPA_PLATFORM=offscreen with
     `--autosolo --take-screenshot=<path>`. The client seeds rand() with
     a fixed value so random-tile patterns (lava etc.) and animation
     start frames are reproducible. Compare the resulting PNG against
     the reference at test/map-test-client.png — fail on any pixel
     diff that exceeds the same +/-10% per-channel tolerance used by
     testingmap2png.py.

This is the runtime counterpart of testingmap2png.py: testingmap2png.py
proves that the offline map renderer (tools/map2png) draws each tmx
correctly; testingmap4client.py proves the live client's MapVisualiser
composes the same scene at first-frame, including doors/objects that
map2png does not draw (and that wouldn't show up in a server-side test).
A regression in layer ordering, door-tile placement, or
animation-frame seeding will flip this from PASS to FAIL.
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, subprocess, json, time, shutil, multiprocessing, signal, threading
import diagnostic
import build_paths
from cmd_helpers import clamp_local

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()

# ── config ──────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config",
                            "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

CLIENT_SRC_DIR = os.path.join(ROOT, "client", "qtopengl")
BUILD_DIR = build_paths.build_path("client", "qtopengl", "build",
                                   "testing-screenshot" + diagnostic.build_dir_suffix(DIAG))
CLIENT_BIN = os.path.join(BUILD_DIR, "catchchallenger")

DATAPACK_SRC = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
MAINCODE = "test"
# Self-contained client/CMakeLists.txt drops the binary (and its datapack
# search dir) at the build dir top-level — no more nested client/qtopengl/.
DATAPACK_DST = os.path.join(BUILD_DIR, "datapack", "internal")

# Same path SoloDatabaseInit picks via QStandardPaths::AppLocalDataLocation.
SAVEGAME_DIR = os.path.expanduser("~/.local/share/CatchChallenger/client/solo")

import test_config as _tc
OUTPUT_IMAGE = _tc.SCREENSHOT_OUTPUT
REFERENCE_IMAGE = os.path.join(ROOT, "test", "map-test-client.png")
DIFF_IMAGE = _tc.SCREENSHOT_DIFF

COMPILE_TIMEOUT = 1200
RUN_TIMEOUT = 60

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

SCRIPT_NAME = "compile testingmap4client"
SCRIPT_RUN_NAME = "run testingmap4client"
from test_config import FAILED_JSON

results = []
_last_log_time = [time.monotonic()]


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")

def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    print(f"{C_GREEN}[PASS]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")

def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    print(f"{C_RED}[FAIL]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def load_failed_cases():
    """Return None to run every case, else the list of cases to re-run.
    Mirrors the pattern in every other testing*.py."""
    if not os.path.isfile(FAILED_JSON):
        return None
    try:
        with open(FAILED_JSON, "r") as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError):
        return None
    if SCRIPT_NAME not in data and SCRIPT_RUN_NAME not in data:
        return None
    return data.get(SCRIPT_NAME, []) + data.get(SCRIPT_RUN_NAME, [])


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    data = {}
    if os.path.isfile(FAILED_JSON):
        try:
            with open(FAILED_JSON, "r") as f:
                data = json.load(f)
        except (json.JSONDecodeError, IOError):
            data = {}
    failed_compile = []
    failed_run = []
    for name, ok, _detail, _elapsed in results:
        if not ok:
            if "compile" in name:
                failed_compile.append(name)
            else:
                failed_run.append(name)
    if failed_compile:
        data[SCRIPT_NAME] = failed_compile
    elif SCRIPT_NAME in data:
        del data[SCRIPT_NAME]
    if failed_run:
        data[SCRIPT_RUN_NAME] = failed_run
    elif SCRIPT_RUN_NAME in data:
        del data[SCRIPT_RUN_NAME]
    if data:
        with open(FAILED_JSON, "w") as f:
            json.dump(data, f, indent=2)
    elif os.path.isfile(FAILED_JSON):
        os.remove(FAILED_JSON)


def run_cmd(args, cwd, timeout, env=None):
    timeout = clamp_local(timeout)
    full_args = ["nice", "-n", "19", "ionice", "-c", "3"] + list(args)
    diagnostic.record_cmd(full_args, cwd)
    try:
        p = subprocess.run(
            full_args,
            cwd=cwd, timeout=timeout, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if e.stdout else ""
        return -1, f"TIMEOUT after {timeout}s\n{out}"
    except FileNotFoundError as e:
        return -1, f"command not found: {e}"


# Max share of pixels allowed to exceed the per-channel tolerance.
# The live client cannot match testingmap2png.py's strict zero-pixel rule
# because tile animations (lava, follower NPC, plant cycles) advance on a
# Per-image budget: zero — ANY pixel exceeding the per-channel band
# fails the test. The ±10%-per-channel pixel band itself is what
# absorbs PNG compression / quantisation jitter; the per-image budget
# does NOT exist to swallow rendering drift. If a render is genuinely
# unstable, fix the source (pin rand, freeze the animation frame),
# don't widen the budget here.
MAX_FAIL_PCT = 0.0


def image_compare(image_new, image_reference):
    """Per-pixel ±10%-per-channel match like testingmap2png.py, with a
    zero per-image budget — any pixel whose R/G/B/A diff exceeds 10%
    of the reference channel value FAILs the test. Returns (ok, detail).
    On failure also writes a black-on-white diff mask to DIFF_IMAGE."""
    try:
        from PIL import Image
    except ImportError:
        return False, "PIL (Pillow) not installed; required for image comparison"
    img_new = Image.open(image_new).convert("RGBA")
    img_ref = Image.open(image_reference).convert("RGBA")
    if img_new.size != img_ref.size:
        return False, (f"size mismatch: new {img_new.size[0]}x{img_new.size[1]} "
                       f"vs ref {img_ref.size[0]}x{img_ref.size[1]}")
    w, h = img_ref.size
    px_new = img_new.load()
    px_ref = img_ref.load()
    diff_bytes = bytearray(b"\xff\xff\xff" * (w * h))
    fail_count = 0
    first_fail = None
    y = 0
    while y < h:
        x = 0
        while x < w:
            n = px_new[x, y]
            r = px_ref[x, y]
            ci = 0
            while ci < 4:
                if abs(n[ci] - r[ci]) > 0.10 * r[ci]:
                    if first_fail is None:
                        first_fail = (x, y, ci, n[ci], r[ci])
                    fail_count += 1
                    idx = (y * w + x) * 3
                    diff_bytes[idx] = 0
                    diff_bytes[idx + 1] = 0
                    diff_bytes[idx + 2] = 0
                    break
                ci += 1
            x += 1
        y += 1
    total = w * h
    pct = (fail_count / total) * 100.0
    if fail_count == 0:
        return True, f"all {total} pixels within +/-10% per channel ({w}x{h})"
    Image.frombytes("RGB", (w, h), bytes(diff_bytes)).save(DIFF_IMAGE)
    fx, fy, fc, fn, fr = first_fail
    chan = "RGBA"[fc]
    return False, (f"{fail_count}/{total} pixels diff > 10% ({pct:.2f}%); "
                   f"first fail @ ({fx},{fy}) channel {chan}: "
                   f"new={fn} ref={fr}; diff mask saved to {DIFF_IMAGE}")


def setup_datapack():
    """Symlink the pre-staged datapack into <build>/datapack/internal/.
    The staged source is shared (don't mutate it), so maincode pruning
    is dropped — the embedded server loads only mainDatapackCode anyway.
    See CLAUDE.md "Datapack staging cache"."""
    import datapack_stage as _ds
    if not os.path.isdir(DATAPACK_SRC):
        return False, f"datapack source not found: {DATAPACK_SRC}"
    staged = _ds.staged_local(DATAPACK_SRC)
    if not os.path.isdir(staged):
        return False, (f"staged datapack missing at {staged} — "
                       f"was stage_datapacks.py run?")
    map_main_staged = os.path.join(staged, "map", "main")
    if not os.path.isdir(os.path.join(map_main_staged, MAINCODE)):
        return False, (f"maincode '{MAINCODE}' not present in "
                       f"{map_main_staged}")
    if os.path.islink(DATAPACK_DST) or os.path.isfile(DATAPACK_DST):
        os.remove(DATAPACK_DST)
    elif os.path.isdir(DATAPACK_DST):
        shutil.rmtree(DATAPACK_DST)
    os.makedirs(os.path.dirname(DATAPACK_DST), exist_ok=True)
    os.symlink(staged, DATAPACK_DST)
    log_info(f"symlinked datapack {DATAPACK_DST} -> {staged}")
    return True, ""


def wipe_savegame():
    if os.path.isdir(SAVEGAME_DIR):
        shutil.rmtree(SAVEGAME_DIR)
        log_info(f"removed savegames: {SAVEGAME_DIR}")
    else:
        log_info(f"no savegames to remove: {SAVEGAME_DIR}")


def test_compile():
    """Configure + build the qtopengl singleplayer client. The cmake
    helper in test/cmake_helpers.py doesn't expose
    CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER, so we drive cmake
    directly here."""
    name = SCRIPT_NAME

    cc, cxx, _ = ("clang", "clang++", "clang") if (
        diagnostic.compiler_name(DIAG) == "clang") else ("gcc", "g++", "gcc")

    os.makedirs(BUILD_DIR, exist_ok=True)

    # qtopengl client now lives at client/CMakeLists.txt as a
    # standalone CMake project (the old root project is gone).
    cmake_args = [
        "cmake", "-S", os.path.join(ROOT, "client"), "-B", BUILD_DIR,
        f"-DCMAKE_C_COMPILER={cc}",
        f"-DCMAKE_CXX_COMPILER={cxx}",
        "-DCMAKE_BUILD_TYPE=Debug",
        # -fno-lto: tests don't need LTO; the lto-wrapper fans out
        # extra compile passes that slow tests for no benefit.
        "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold -fno-lto",
        "-DCMAKE_SHARED_LINKER_FLAGS=-fno-lto",
        "-DCMAKE_MODULE_LINKER_FLAGS=-fno-lto",
        "-DCMAKE_C_FLAGS_INIT=-fno-lto",
        "-DCMAKE_CXX_FLAGS_INIT=-fno-lto",
        "-DCATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON",
        # Mirror the testingclient.py default — audio off, since the
        # offscreen Qt platform doesn't have an audio sink.
        "-DCATCHCHALLENGER_NOAUDIO=ON",
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
                       "--target", "catchchallenger", "-j", NPROC],
                      BUILD_DIR, COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"cmake build failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return False
    if not os.path.isfile(CLIENT_BIN):
        log_fail(name, f"binary not at expected path: {CLIENT_BIN}")
        return False
    log_pass(name)
    return True


def test_run():
    """Run the qtopengl client headless and capture --take-screenshot.
    Compare against REFERENCE_IMAGE; FAIL on any pixel diff >10%."""
    name = SCRIPT_RUN_NAME
    if not os.path.isfile(CLIENT_BIN):
        log_fail(name, f"binary missing: {CLIENT_BIN}")
        return False

    os.makedirs(os.path.dirname(OUTPUT_IMAGE), exist_ok=True)
    if os.path.isfile(OUTPUT_IMAGE):
        os.remove(OUTPUT_IMAGE)
    if os.path.isfile(DIFF_IMAGE):
        os.remove(DIFF_IMAGE)

    ok, detail = setup_datapack()
    if not ok:
        log_fail(name, detail)
        return False

    wipe_savegame()

    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v

    args = [CLIENT_BIN,
            "--autosolo",
            f"--main-datapack-code={MAINCODE}",
            f"--take-screenshot={OUTPUT_IMAGE}"]
    timeout = diagnostic.scale_timeout(DIAG, RUN_TIMEOUT)
    log_info(f"running: catchchallenger --autosolo --take-screenshot=…")
    cwd = os.path.dirname(CLIENT_BIN)
    wrapper = diagnostic.runtime_wrapper(DIAG)
    rc, out = run_cmd(wrapper + args, cwd, timeout, env=env)
    if rc != 0:
        log_fail(name, f"client exited rc={rc}")
        for line in out.splitlines()[-30:]:
            print(f"  | {line}")
        return False
    if not os.path.isfile(OUTPUT_IMAGE):
        log_fail(name, "screenshot file not produced")
        for line in out.splitlines()[-30:]:
            print(f"  | {line}")
        return False
    if not os.path.isfile(REFERENCE_IMAGE):
        log_fail(name, (f"reference image not found: {REFERENCE_IMAGE}. "
                        f"If this is the first run, vet the new "
                        f"{OUTPUT_IMAGE} and copy it to "
                        f"test/map-test-client.png as the new baseline."))
        return False

    match, detail = image_compare(OUTPUT_IMAGE, REFERENCE_IMAGE)
    if match:
        log_pass(name, detail)
        if os.path.isfile(OUTPUT_IMAGE):
            os.remove(OUTPUT_IMAGE)
        return True
    log_fail(name, detail)
    return False


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — qtopengl --take-screenshot regression")
    print(f"{'='*60}{C_RESET}\n")

    if not os.path.isdir(CLIENT_SRC_DIR):
        print(f"Error: client source not found: {CLIENT_SRC_DIR}")
        sys.exit(1)

    failed_cases = load_failed_cases()

    if should_run(SCRIPT_NAME, failed_cases):
        if not test_compile():
            save_failed_cases()
            summary()
            return
        print()
    elif not os.path.isfile(CLIENT_BIN):
        log_fail(SCRIPT_NAME, "binary missing from previous build")
        save_failed_cases()
        summary()
        return

    if should_run(SCRIPT_RUN_NAME, failed_cases):
        test_run()
        print()

    save_failed_cases()
    summary()


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for nm, ok, dt, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {nm}  ({elapsed:.1f}s)  {dt}")
    print(f"\n  total: {passed} passed, {failed} failed, "
          f"elapsed {total_elapsed:.1f}s")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
