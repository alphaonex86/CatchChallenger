#!/usr/bin/env python3
"""
testingmap2png.py — Test map2png tool compilation and output verification.

1. Clean and compile map2png.pro
2. Run map2png --renderAll on city.tmx
3. Compare generated image with reference using pixel comparison
"""

import os, sys, subprocess, json, time, shutil
import multiprocessing
from remote_build import start_remote_builds, collect_remote_results, count_remote_tests
import diagnostic

DIAG = diagnostic.parse_diag_args()

# ── config ──────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAP2PNG_PRO = os.path.join(ROOT, "tools", "map2png", "map2png.pro")
QMAKE = _config["qmake"]
NPROC = str(multiprocessing.cpu_count())

COMPILE_TIMEOUT = 600
RUN_TIMEOUT = 60

OUTPUT_IMAGE = "/mnt/data/perso/tmpfs/catchchallenger-map2png.png"
REFERENCE_IMAGE = os.path.join(ROOT, "test", "map-test.png")
DIFF_IMAGE = "/mnt/data/perso/tmpfs/fail.png"
TEST_MAP = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack/map/main/test/city.tmx"

# ── colors ──────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

SCRIPT_NAME = "compile map2png.map2png.pro"
SCRIPT_RUN_NAME = "run map2png.map2png.pro"
FAILED_JSON = "/mnt/data/perso/tmpfs/failed.json"

results = []
_last_log_time = [time.monotonic()]

def load_failed_cases():
    """Load failed cases for this script from failed.json."""
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
    """Check if test should run. None=run all, []=skip all, [..]=only listed."""
    if failed_cases is None:
        return True
    return test_name in failed_cases

def save_failed_cases():
    """Update failed.json with current failures for this script."""
    data = {}
    if os.path.isfile(FAILED_JSON):
        try:
            with open(FAILED_JSON, "r") as f:
                data = json.load(f)
        except (json.JSONDecodeError, IOError):
            data = {}
    failed_compile = []
    failed_run = []
    for name, ok, detail, elapsed in results:
        if not ok:
            if "compile" in name:
                failed_compile.append(name)
            else:
                failed_run.append(name)
    if failed_compile:
        data[SCRIPT_NAME] = failed_compile
    if failed_run:
        data[SCRIPT_RUN_NAME] = failed_run
    with open(FAILED_JSON, "w") as f:
        json.dump(data, f, indent=2)

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

def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT):
    try:
        p = subprocess.run(
            ["nice", "-n", "19", "ionice", "-c", "3"] + list(args),
            cwd=cwd, timeout=timeout,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"
    except FileNotFoundError as e:
        return -1, f"command not found: {e}"

def run_under_gdb(prog_args, cwd, timeout):
    """Run a program under gdb in batch mode.

    Uses --return-child-result so gdb propagates the child's exit code
    (or 128+signal on a fatal signal). On any abnormal termination (SEGV,
    abort(), assertion, ASan/UBSan trap, out-of-bound abort), gdb stops
    at the failing frame and the post-run commands dump a full backtrace
    of every thread. On normal exit gdb's "bt" prints "No stack." which
    is harmless and the caller ignores the output anyway (rc==0).

    Falls back to a plain run when gdb is missing on the host.
    """
    if shutil.which("gdb") is None:
        log_info("gdb not found on PATH — running map2png without backtrace wrapper")
        return run_cmd(prog_args, cwd, timeout=timeout)
    gdb_args = [
        "gdb", "--batch", "--return-child-result", "--nx", "-q",
        "-ex", "set pagination off",
        "-ex", "set print thread-events off",
        "-ex", "set confirm off",
        "-ex", "handle SIGPIPE nostop noprint pass",
        "-ex", "run",
        "-ex", "echo \\n=== gdb: program stopped ===\\n",
        "-ex", "info program",
        "-ex", "echo \\n=== gdb: registers ===\\n",
        "-ex", "info registers",
        "-ex", "echo \\n=== gdb: backtrace (current thread, full) ===\\n",
        "-ex", "bt full",
        "-ex", "echo \\n=== gdb: backtrace (all threads) ===\\n",
        "-ex", "thread apply all bt",
        "-ex", "quit",
        "--args",
    ] + list(prog_args)
    return run_cmd(gdb_args, cwd, timeout=timeout)

def parse_layout_lines(stdout_text):
    """Parse 'LAYOUT_MAP image_x=... image_y=... image_w=... image_h=...
    tilewidth=... tileheight=...' lines emitted by map2png after rendering.
    Returns a list of dicts with int values for each key."""
    needed = ("image_x", "image_y", "image_w", "image_h", "tilewidth", "tileheight")
    layouts = []
    for line in stdout_text.splitlines():
        if "LAYOUT_MAP" not in line:
            continue
        m = {}
        for token in line.split():
            if "=" not in token:
                continue
            k, v = token.split("=", 1)
            try:
                m[k] = int(v)
            except ValueError:
                pass
        ok = True
        ki = 0
        while ki < len(needed):
            if needed[ki] not in m:
                ok = False
                break
            ki += 1
        if ok:
            layouts.append(m)
    return layouts


def image_compare(image_new, image_reference, maps_layout=None):
    """PASS if width and height match exactly AND every pixel's R, G, B, and A
    channels are each within ±10% of the reference channel value. The 10% is
    relative to the *reference* value, so a reference channel of 0 requires an
    exact 0 in the new image (no tolerance). Returns (ok, detail).

    maps_layout: optional list of {'image_x','image_y','image_w','image_h',
    'tilewidth','tileheight'} dicts, one per map composing the render. When
    provided, the diff mask uses each map's own tile size for its checker
    background, aligned to the map's image-space top-left. When omitted the
    checker falls back to a 16x16 grid keyed off the reference's alpha mask.
    """
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
    fail_count = 0
    first_fail = None
    # Diff buffer:
    #   * outside any map: plain white, never read
    #   * inside a map: per-map checker (white tile + light-grey tile) using
    #     that map's own tilewidth/tileheight from the .tmx, aligned to the
    #     map's image-space top-left so the checker grid matches the rendered
    #     tile grid exactly. (0,0) of the per-map grid is the white tile.
    #   * any pixel that fails the per-channel comparison: flipped to black
    # Only saved to disk when sizes match but pixel comparison fails.
    diff_bytes = bytearray(b"\xff\xff\xff" * (w * h))
    GREY = (220, 220, 220)
    have_layout = bool(maps_layout)
    y = 0
    while y < h:
        x = 0
        while x < w:
            n = px_new[x, y]
            r = px_ref[x, y]
            # Decide whether the pixel sits on a map and compute its checker
            # parity from the matching map's own tile size.
            grey_here = False
            if have_layout:
                mi = 0
                while mi < len(maps_layout):
                    mm = maps_layout[mi]
                    mx = mm["image_x"]
                    my = mm["image_y"]
                    if mx <= x < mx + mm["image_w"] and my <= y < my + mm["image_h"]:
                        tx = (x - mx) // mm["tilewidth"]
                        ty = (y - my) // mm["tileheight"]
                        if (tx + ty) % 2 == 1:
                            grey_here = True
                        # First map wins; rendered overlap is undefined anyway.
                        break
                    mi += 1
            elif r[3] > 0:
                # Fallback when no layout was supplied: align a 16x16 checker
                # to the global image grid and only paint inside the reference
                # alpha mask.
                if ((x >> 4) + (y >> 4)) % 2 == 1:
                    grey_here = True
            if grey_here:
                idx = (y * w + x) * 3
                diff_bytes[idx] = GREY[0]
                diff_bytes[idx + 1] = GREY[1]
                diff_bytes[idx + 2] = GREY[2]
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
    if fail_count == 0:
        return True, f"all {total} pixels within +/-10% per channel ({w}x{h})"
    Image.frombytes("RGB", (w, h), bytes(diff_bytes)).save(DIFF_IMAGE)
    pct = (fail_count / total) * 100.0
    fx, fy, fc, fn, fr = first_fail
    chan = "RGBA"[fc]
    return False, (f"{fail_count}/{total} pixels diff > 10% ({pct:.2f}%); "
                   f"first fail @ ({fx},{fy}) channel {chan}: new={fn} ref={fr}; "
                   f"diff mask saved to {DIFF_IMAGE}")

def clean_output():
    """Remove OUTPUT_IMAGE. Called once before each run so a stale image from a
    previous run can't be mistaken for current output, and once after a
    successful run to leave the tmpfs clean. NOT called on failure paths:
    when test_run() fails, OUTPUT_IMAGE (if any was produced) and DIFF_IMAGE
    are both kept so the failure can be inspected. The next run wipes them
    again at its start."""
    if os.path.isfile(OUTPUT_IMAGE):
        os.remove(OUTPUT_IMAGE)

def clean_build_artifacts(build_dir):
    """Clean build artifacts from a directory."""
    if not os.path.isdir(build_dir):
        return
    import glob as _glob
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in _glob.glob(os.path.join(build_dir, pattern)):
            os.remove(f)

def test_compile():
    """Build map2png.pro via cmake, then clean up. Returns True on success."""
    import cmake_helpers as _ch
    suffix = diagnostic.build_dir_suffix(DIAG)
    build_dir = os.path.join(os.path.dirname(MAP2PNG_PRO), "build", "testing" + suffix)
    os.makedirs(build_dir, exist_ok=True)

    label = "map2png.pro" + diagnostic.label_suffix(DIAG)

    ok = _ch.build_project(
        MAP2PNG_PRO, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=lambda d: os.makedirs(d, exist_ok=True),
        run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )
    if not ok:
        # Wipe the build dir so retry starts clean (matches the qmake-era
        # `make distclean` on failure).
        import shutil
        shutil.rmtree(build_dir, ignore_errors=True)
        return False
    return True

def test_run():
    """Run map2png and compare output with reference. Returns True on success."""
    # Ensure output directory exists
    os.makedirs(os.path.dirname(OUTPUT_IMAGE), exist_ok=True)

    # Clean before (also drop any stale diff mask from a previous run)
    clean_output()
    if os.path.isfile(DIFF_IMAGE):
        os.remove(DIFF_IMAGE)
    
    # Build the tool first (if not already built)
    build_dir = os.path.join(os.path.dirname(MAP2PNG_PRO), "build", "testing" + diagnostic.build_dir_suffix(DIAG))
    map2png_binary = os.path.join(build_dir, "map2png")
    
    if not os.path.isfile(map2png_binary):
        # Need to build it (via cmake_helpers shared driver).
        import cmake_helpers as _ch
        log_info(f"Building map2png for runtime test")
        os.makedirs(build_dir, exist_ok=True)
        ok = _ch.build_project(
            MAP2PNG_PRO, build_dir, "map2png runtime test",
            root=ROOT, nproc=NPROC,
            log_info=log_info, log_pass=log_pass, log_fail=log_fail,
            ensure_dir=lambda d: os.makedirs(d, exist_ok=True),
            run_cmd=run_cmd,
            diag=DIAG, diag_module=diagnostic,
        )
        if not ok:
            log_fail(SCRIPT_RUN_NAME, "cmake build failed for runtime test")
            return False
    
    if not os.path.isfile(map2png_binary):
        log_fail(SCRIPT_RUN_NAME, "map2png binary not found after build")
        clean_output()
        return False
    
    # Run map2png. Wrap in gdb so any abnormal termination (SEGV, abort(),
    # ASan/UBSan trap, out-of-bound, rc != 0) drops a backtrace into the
    # captured output. Skip the gdb wrapper under --valgrind, which already
    # virtualises the binary and has its own error reporting.
    prog_args = [map2png_binary, "--renderAll", TEST_MAP, OUTPUT_IMAGE]
    run_timeout = diagnostic.scale_timeout(DIAG, RUN_TIMEOUT)
    if diagnostic.is_valgrind(DIAG):
        wrapped = diagnostic.runtime_wrapper(DIAG) + prog_args
        log_info(f"running (valgrind): map2png --renderAll {TEST_MAP} {OUTPUT_IMAGE}")
        rc, out = run_cmd(wrapped, cwd=os.path.dirname(MAP2PNG_PRO), timeout=run_timeout)
    else:
        log_info(f"running (gdb): map2png --renderAll {TEST_MAP} {OUTPUT_IMAGE}")
        rc, out = run_under_gdb(prog_args, cwd=os.path.dirname(MAP2PNG_PRO), timeout=run_timeout)

    if rc != 0:
        log_fail(SCRIPT_RUN_NAME, f"run failed (rc={rc})")
        if out.strip():
            # Dump the full output on failure so the gdb backtrace is visible.
            for line in out.splitlines():
                print(f"  | {line}")
        # Keep OUTPUT_IMAGE on failure (if any was produced) so it can be
        # inspected; the next run cleans it before re-rendering.
        return False
    
    # Check output exists
    if not os.path.isfile(OUTPUT_IMAGE):
        log_fail(SCRIPT_RUN_NAME, "output file not created")
        return False
    
    # Compare with reference. map2png prints one LAYOUT_MAP line per loaded
    # map (image-space rect + per-map tile size); pass them to image_compare
    # so each map's checker uses its own tilewidth / tileheight.
    maps_layout = parse_layout_lines(out)
    if os.path.isfile(REFERENCE_IMAGE):
        match, detail = image_compare(OUTPUT_IMAGE, REFERENCE_IMAGE, maps_layout=maps_layout)
        if match:
            log_pass(SCRIPT_RUN_NAME, detail)
        else:
            log_fail(SCRIPT_RUN_NAME, f"different from reference: {detail}")
            # Keep OUTPUT_IMAGE on failure so it can be compared visually
            # against the reference / diff mask.
            return False
    else:
        log_fail(SCRIPT_RUN_NAME, f"reference image not found: {REFERENCE_IMAGE}")
        return False
    
    # Clean after
    clean_output()
    
    return True

def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — map2png Tool Testing")
    print(f"{'='*60}{C_RESET}\n")
    
    # Check prerequisites
    if not os.path.isfile(MAP2PNG_PRO):
        print(f"Error: {MAP2PNG_PRO} not found")
        sys.exit(1)
    
    if not os.path.isfile(REFERENCE_IMAGE):
        print(f"Warning: Reference image not found at {REFERENCE_IMAGE}")
        print("  Test cannot verify output, but will still run compilation test.")
    
    failed_cases = load_failed_cases()
    
    # Test compilation
    if should_run(SCRIPT_NAME, failed_cases):
        test_compile()
        print()
    
    # Test runtime output comparison
    if should_run(SCRIPT_RUN_NAME, failed_cases):
        test_run()
        print()
    
    # Summary
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
