#!/usr/bin/env python3
"""
testingwasm.py — CatchChallenger WebAssembly (emscripten) cross-compile.

Cross-compiles the qtopengl client (`client/`, OUTPUT_NAME catchchallenger)
to WebAssembly with Qt for WebAssembly + emscripten, then stages the files a
browser actually needs into one directory — the layout deploy/ publishes.

qtopengl and NOT qtcpu800x600: qtopengl is the client whose UI follows the
window size, so the showMaximized() in its main() fills Qt's #screen
container and the canvas covers 100% of the page. qtcpu800x600 is a fixed
800x600 layout and would sit in a corner of the canvas.

Phases:
  1. compile   — qt-cmake + build; assert the produced .wasm really is a
                 WebAssembly module, that Qt's loader files were emitted next
                 to it, and that the browser feature set was forced
                 (WebSockets ON / single-player OFF / no audio). NO packaging.
  2. stage     — copy catchchallenger.{html,js,wasm} + qtloader.js + qtlogo.svg
                 into <tmpfs_build_root>/web/catchchallenger/ and report the
                 total transfer size. That directory is the deploy input; the
                 test harness NEVER publishes it (see all.sh).

Self-skips cleanly when the Qt-for-WebAssembly prefix is absent, so all.sh on
a host without the toolchain reports a skip, not a hard failure. The toolchain
lives OUTSIDE the repo at <prefix> (default /mnt/data/perso/progs/wasm-qt,
override with $CC_WASM_PREFIX), mirroring the MXE prefix used by
testingcompilationwindows.py and the DJGPP one used by
testingcompilationmsdos.py. Build it with <prefix>/build-qt-wasm.sh.

Sibling of testingcompilation{windows,mac,msdos}.py — local-only (no ssh) and
compile-centric. There is no runtime phase: driving the client needs a real
browser, which this host does not run headless.
"""

import sys
sys.dont_write_bytecode = True

import glob, os, shutil, subprocess, time, multiprocessing
import build_paths
import diagnostic
import wall_cap
wall_cap.arm()
import cleanup_helpers
from cmd_helpers import clamp_local

build_paths.ensure_root()

# ── paths ───────────────────────────────────────────────────────────────────
ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

# Qt-for-WebAssembly + emsdk prefix (kept out of the repo, like MXE_PREFIX).
WASM_PREFIX = os.environ.get("CC_WASM_PREFIX", "/mnt/data/perso/progs/wasm-qt")
EMSDK_ROOT  = os.path.join(WASM_PREFIX, "emsdk")
QT_WASM     = os.path.join(WASM_PREFIX, "qt6-wasm")
QT_CMAKE    = os.path.join(QT_WASM, "bin", "qt-cmake")
EMSCRIPTEN  = os.path.join(EMSDK_ROOT, "upstream", "emscripten")
EMBUILDER   = os.path.join(EMSCRIPTEN, "embuilder")

CLIENT_SRC   = os.path.join(ROOT, "client")
CLIENT_BUILD = build_paths.build_path("client/qtopengl/build/testing-wasm")
cleanup_helpers.register_build_dir(CLIENT_BUILD)

# Deploy input. NOT under a registered build dir: cleanup_helpers wipes those
# on success, and deploy.sh must still find the bundle after a green run.
WEB_DIR = os.path.join(build_paths.TMPFS_BUILD_ROOT, "web", "catchchallenger")

# What a browser has to fetch. qtlogo.svg is the loading spinner referenced by
# Qt's wasm_shell.html — without it the page 404s while the module downloads.
WEB_FILES = ["catchchallenger.html", "catchchallenger.js", "catchchallenger.wasm",
             "qtloader.js", "qtlogo.svg"]

NICE_PREFIX_COMPILE = ["nice", "-n", "19", "ionice", "-c", "3"]

CONFIGURE_TIMEOUT = 600
COMPILE_TIMEOUT   = 2400
PORT_TIMEOUT      = 300

# ── colours / logging (same shape as the sibling scripts) ────────────────────
C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"
results = []
_last_log_time = [time.monotonic()]
total_expected = [0]
SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    return failed_cases is None or test_name in failed_cases


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
    now = time.monotonic(); elapsed = now - _last_log_time[0]; _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic(); elapsed = now - _last_log_time[0]; _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li]); li += 1


# ── env / probes ─────────────────────────────────────────────────────────────
def wasm_env():
    """Environment for the emscripten cross build. Built from scratch (env
    hygiene): sourcing emsdk_env.sh only exports EMSDK/EM_CONFIG/PATH, so we
    set those three directly and forward nothing else that could leak a host
    Qt or an unrelated toolchain into the build."""
    node_bin = sorted(glob.glob(os.path.join(EMSDK_ROOT, "node", "*", "bin")))
    path = [EMSCRIPTEN, EMSDK_ROOT]
    if node_bin:
        path.append(node_bin[-1])
    path += ["/usr/local/bin", "/usr/bin", "/bin"]
    env = {
        "PATH": ":".join(path),
        "EMSDK": EMSDK_ROOT,
        "EM_CONFIG": os.path.join(EMSDK_ROOT, ".emscripten"),
        "HOME": os.environ.get("HOME", "/root"),
        "USER": os.environ.get("USER", "root"),
        "LANG": os.environ.get("LANG", "C.UTF-8"),
        "TERM": os.environ.get("TERM", "dumb"),
        "TMPDIR": os.environ.get("TMPDIR", "/tmp"),
    }
    return build_paths.pycache_env(env)


def toolchain_available():
    """List missing prerequisites ([] == ready)."""
    missing = []
    for p in (QT_CMAKE, EMBUILDER, os.path.join(EMSCRIPTEN, "em++")):
        if not os.path.isfile(p):
            missing.append(os.path.relpath(p, WASM_PREFIX))
    if not os.path.isfile(os.path.join(QT_WASM, "lib", "libQt6WebSockets.a")):
        missing.append("qt6-wasm/lib/libQt6WebSockets.a")
    return missing


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(args, cwd)
    try:
        p = subprocess.run(args, cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def is_wasm_module(path):
    """True when `path` starts with the WebAssembly magic \\0asm + version 1."""
    if not os.path.isfile(path):
        return False, "missing"
    try:
        with open(path, "rb") as f:
            head = f.read(8)
    except OSError as exc:
        return False, str(exc)
    if head[:4] != b"\0asm":
        return False, f"bad magic {head[:4]!r}"
    if head[4:8] != b"\x01\x00\x00\x00":
        return False, f"unexpected wasm version {head[4:8]!r}"
    return True, "wasm v1"


def cache_flag(name):
    """Read one BOOL out of the build's CMakeCache.txt ('' when absent)."""
    cache = os.path.join(CLIENT_BUILD, "CMakeCache.txt")
    try:
        with open(cache, "r") as f:
            for line in f:
                if line.startswith(name + ":BOOL="):
                    return line.strip().split("=", 1)[1]
    except OSError:
        pass
    return ""


# ── phase 1: compile ─────────────────────────────────────────────────────────
def build_client():
    name = "compile catchchallenger qtopengl (emscripten)"
    env = wasm_env()

    # libtiled does find_package(ZLIB); emscripten ships zlib as a port that
    # only lands in the sysroot once it has been built. Idempotent and ~2 s
    # warm, so just always do it instead of documenting a manual step.
    rc, out = run_cmd([EMBUILDER, "build", "zlib"], WASM_PREFIX,
                      timeout=PORT_TIMEOUT, env=env)
    if rc != 0:
        _fc.set_extras(name, compile_output=(out or ""))
        log_fail(name, f"embuilder build zlib failed (rc={rc})")
        print(out[-3000:]); return None

    if os.path.isdir(CLIENT_BUILD):
        shutil.rmtree(CLIENT_BUILD, ignore_errors=True)
    os.makedirs(CLIENT_BUILD, exist_ok=True)

    log_info(f"qt-cmake configure (prefix={WASM_PREFIX})")
    # No feature flags here on purpose: client/CMakeLists.txt forces the
    # browser feature set under if(EMSCRIPTEN), so a plain
    # `qt-cmake -S client` outside this test builds the same way.
    cfg = NICE_PREFIX_COMPILE + [
        QT_CMAKE, "-S", CLIENT_SRC, "-B", CLIENT_BUILD,
        # Pin Release like the windows/mac/android/msdos cross scripts:
        # CCCommon.cmake defaults to Debug, and a Debug .wasm is several
        # times the size a browser would have to download.
        "-DCMAKE_BUILD_TYPE=Release",
    ]
    rc, out = run_cmd(cfg, CLIENT_BUILD, timeout=CONFIGURE_TIMEOUT, env=env)
    if rc != 0:
        _fc.set_extras(name, compile_output=(out or ""))
        log_fail(name, f"qt-cmake configure failed (rc={rc})")
        print(out[-3000:]); return None

    log_info(f"cmake --build -j{NPROC}")
    build = NICE_PREFIX_COMPILE + ["cmake", "--build", CLIENT_BUILD, "-j", NPROC]
    rc, out = run_cmd(build, CLIENT_BUILD, env=env)
    if rc != 0:
        # ccache flush + one retry (project policy for transient cache misses).
        if shutil.which("ccache"):
            log_info("build failed; flushing ccache and retrying once")
            try:
                subprocess.run(["ccache", "-C"], capture_output=True,
                               text=True, timeout=120)
            except (OSError, subprocess.SubprocessError):
                pass
            rc, out = run_cmd(build, CLIENT_BUILD, env=env)
    if rc != 0:
        _fc.set_extras(name, compile_output=(out or ""))
        log_fail(name, f"cmake build failed (rc={rc})")
        print(out[-3000:]); return None

    wasm = os.path.join(CLIENT_BUILD, "catchchallenger.wasm")
    ok, what = is_wasm_module(wasm)
    if not ok:
        log_fail(name, f"catchchallenger.wasm is not a WebAssembly module: {what}")
        return None
    log_pass(name, f"-> {os.path.relpath(wasm, build_paths.TMPFS_BUILD_ROOT)} "
                   f"({what}, {os.path.getsize(wasm)//1024} KiB)")
    return CLIENT_BUILD


def check_loader_files():
    """client/CMakeLists.txt builds the target with qt_add_executable(), so
    Qt's wasm finalizer emits the html + loader next to the module. Without
    them the .wasm is dead weight: nothing instantiates it."""
    name = "qt wasm loader files emitted"
    missing = [f for f in WEB_FILES
               if not os.path.isfile(os.path.join(CLIENT_BUILD, f))]
    if missing:
        log_fail(name, "missing: " + ", ".join(missing)); return
    # The shell html is generated from a template; if the export name did not
    # get substituted the page loads and then silently does nothing.
    html = os.path.join(CLIENT_BUILD, "catchchallenger.html")
    try:
        with open(html, "r") as f:
            body = f.read()
    except OSError as exc:
        log_fail(name, f"cannot read catchchallenger.html: {exc}"); return
    if "catchchallenger_entry" not in body or "@APPNAME@" in body:
        log_fail(name, "catchchallenger.html placeholders not substituted"); return
    log_pass(name, f"{len(WEB_FILES)} files, entry=catchchallenger_entry")


def check_browser_feature_set():
    """Guard the if(EMSCRIPTEN) block in client/CMakeLists.txt.
    A browser has no raw TCP socket, so a wasm build that silently kept
    single-player or dropped WebSockets would link but never connect."""
    name = "browser feature set forced"
    expected = {
        "CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS": "ON",
        "CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER": "OFF",
        "CATCHCHALLENGER_NOAUDIO": "ON",
    }
    wrong = []
    for key, want in expected.items():
        got = cache_flag(key)
        if got != want:
            wrong.append(f"{key}={got or '<unset>'} (want {want})")
    if wrong:
        log_fail(name, "; ".join(wrong)); return
    log_pass(name, "websockets ON, singleplayer OFF, noaudio ON")


# ── phase 2: stage the deploy bundle ─────────────────────────────────────────
def stage_web_bundle():
    """Assemble exactly what a web server has to publish. Kept outside the
    registered build dirs so it survives cleanup and deploy/ can pick it up."""
    name = "stage web bundle"
    if os.path.isdir(WEB_DIR):
        shutil.rmtree(WEB_DIR, ignore_errors=True)
    os.makedirs(WEB_DIR, exist_ok=True)
    total = 0
    for f in WEB_FILES:
        src = os.path.join(CLIENT_BUILD, f)
        if not os.path.isfile(src):
            log_fail(name, f"missing {f}"); return
        shutil.copy2(src, os.path.join(WEB_DIR, f))
        total += os.path.getsize(src)
    log_pass(name, f"-> {WEB_DIR} ({len(WEB_FILES)} files, "
                   f"{total//1024} KiB uncompressed)")
    log_info("deploy input: serve that directory; catchchallenger.html is the "
             "entry page. Gzip/brotli the .wasm at the web server — it is by "
             "far the largest object.")


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — WebAssembly (Qt for WebAssembly + emscripten)")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    missing = toolchain_available()
    if missing:
        log_info(f"Qt-for-WebAssembly prefix not usable at {WASM_PREFIX} "
                 f"(missing: {', '.join(missing)}) — skipping WebAssembly "
                 f"test. Build it with {WASM_PREFIX}/build-qt-wasm.sh.")
        save_failed_cases(); summary(); return

    built = None
    if should_run("compile catchchallenger qtopengl (emscripten)", failed_cases):
        built = build_client()
    elif os.path.isfile(os.path.join(CLIENT_BUILD, "catchchallenger.wasm")):
        built = CLIENT_BUILD

    if built is not None:
        if should_run("qt wasm loader files emitted", failed_cases):
            check_loader_files()
        if should_run("browser feature set forced", failed_cases):
            check_browser_feature_set()
        if should_run("stage web bundle", failed_cases):
            stage_web_bundle()

    save_failed_cases()
    summary()


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
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
