#!/usr/bin/env python3
"""
testingcompilationwindows.py — CatchChallenger Windows cross-compile + run.

Steps:
  1. Probe local MXE x86_64 mingw-w64 (qmake) and wine64.
  2. qmake (MXE) + make for qtcpu800x600 and qtopengl, deploy Qt6 + runtime
     DLLs + plugins next to each .exe so wine64 doesn't need WINEPATH.
  3. Run each .exe under wine64 with QT_QPA_PLATFORM=offscreen — first
     --autosolo (no server), then a multi-mode run against the local
     server-filedb.

Self-skips when MXE qmake or wine64 is missing.

The MXE install at /mnt/data/perso/progs/mxe/x86_64/ is shared with other
projects (Ultracopier release scripts, etc.) — this script never modifies it.
Sibling of testingcompilationmac.py / testingcompilationandroid.py.
"""

import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re, time

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT          = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATAPACKS     = _config["paths"]["datapacks"]
NPROC         = str(multiprocessing.cpu_count())

CLIENT_CPU_PRO = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_GL_PRO  = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")

CLIENT_CPU_BUILD_WIN = os.path.join(ROOT, "client/qtcpu800x600/build/testing-wine64")
CLIENT_GL_BUILD_WIN  = os.path.join(ROOT, "client/qtopengl/build/testing-wine64")
WIN_EXE_NAME         = "catchchallenger.exe"

#local server-filedb that the multi-mode wine client connects back to.
SERVER_BUILD    = os.path.join(ROOT, "server/epoll/build/testing-filedb")
SERVER_BIN_NAME = "catchchallenger-server-cli-epoll"

SERVER_HOST = _config["server_host"]
SERVER_PORT = str(_config["server_port"])

# ── MXE / wine64 ───────────────────────────────────────────────────────────
MXE_ROOT   = "/mnt/data/perso/progs/mxe/x86_64"
MXE_QMAKE  = MXE_ROOT + "/usr/x86_64-w64-mingw32.shared/qt6/bin/qmake"
MXE_BIN    = MXE_ROOT + "/usr/bin"
MXE_QT_BIN = MXE_ROOT + "/usr/x86_64-w64-mingw32.shared/qt6/bin"
MXE_RT_BIN = MXE_ROOT + "/usr/x86_64-w64-mingw32.shared/bin"
WINE_BIN   = shutil.which("wine64") or "/etc/eselect/wine/bin/wine64"

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
WINE_TIMEOUT         = 120

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN = "\033[92m"
C_RED   = "\033[91m"
C_CYAN  = "\033[96m"
C_RESET = "\033[0m"

results = []
_last_log_time = [time.monotonic()]
total_expected = [0]
server_proc = None

SCRIPT_NAME = os.path.basename(__file__)
FAILED_JSON = "/mnt/data/perso/tmpfs/failed.json"


def load_failed_cases():
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
    if failed_cases is None:
        return True
    idx = 0
    while idx < len(failed_cases):
        if failed_cases[idx] == test_name:
            return True
        idx += 1
    return False


def save_failed_cases():
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


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    try:
        p = subprocess.run(args, cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def detect_maincodes(datapack_src):
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return []
    out = []
    entries = sorted(os.listdir(map_main))
    idx = 0
    while idx < len(entries):
        if os.path.isdir(os.path.join(map_main, entries[idx])):
            out.append(entries[idx])
        idx += 1
    return out


def setup_datapack_client(build_dir, datapack_src, maincode, label):
    """Copy datapack to <build_dir>/datapack/internal/, keep only maincode in map/main/."""
    dst = os.path.join(build_dir, "datapack", "internal")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    if not os.path.isdir(datapack_src):
        log_fail(f"datapack {label}", f"source not found: {datapack_src}")
        return False
    log_info(f"copying datapack to {dst}")
    shutil.copytree(datapack_src, dst, ignore=shutil.ignore_patterns(".git"))
    map_main = os.path.join(dst, "map", "main")
    if os.path.isdir(map_main):
        for entry in os.listdir(map_main):
            if entry == maincode:
                continue
            full = os.path.join(map_main, entry)
            if os.path.isdir(full):
                shutil.rmtree(full)
                log_info(f"removed map/main/{entry}")
    log_info(f"maincode: {maincode}")
    log_pass(f"datapack {label}")
    return True


def mxe_available():
    # Phase 5: we now drive the MXE cmake wrapper, but qmake6 in MXE is
    # still a useful "this MXE install is healthy" signal — qt-cmake
    # doesn't add a separate filesystem marker, so qmake's existence
    # remains the cheap probe.
    mxe_cmake = MXE_ROOT + "/usr/bin/x86_64-w64-mingw32.shared-cmake"
    return (os.path.isfile(MXE_QMAKE) and os.path.isfile(mxe_cmake)
            and os.path.isfile(WINE_BIN))


def find_built_exe(build_dir):
    """Locate catchchallenger.exe inside the MXE-cmake build dir. CMake
    drops the binary at build_dir/<output_subdir>/<target>.exe; the legacy
    qmake-era paths (build_dir/, release/, debug/) are still searched as
    a fallback during the migration."""
    candidates = [build_dir,
                  os.path.join(build_dir, "release"),
                  os.path.join(build_dir, "debug"),
                  os.path.join(build_dir, "client", "qtopengl"),
                  os.path.join(build_dir, "client", "qtcpu800x600")]
    idx = 0
    while idx < len(candidates):
        cand = os.path.join(candidates[idx], WIN_EXE_NAME)
        if os.path.isfile(cand):
            return cand
        idx += 1
    return None


def deploy_mxe_dependencies(exe_path):
    """Copy every DLL the .exe could need next to it (windeployqt-style),
    so the binary runs without WINEPATH gymnastics."""
    dst_dir = os.path.dirname(exe_path)

    def copy_dll_dir(src_dir, want_prefix=None):
        if not os.path.isdir(src_dir):
            return 0
        n = 0
        try:
            entries = os.listdir(src_dir)
        except OSError:
            return 0
        ei = 0
        while ei < len(entries):
            fname = entries[ei]
            ei += 1
            if not fname.endswith(".dll"):
                continue
            if want_prefix and not fname.startswith(want_prefix):
                continue
            src = os.path.join(src_dir, fname)
            dst = os.path.join(dst_dir, fname)
            try:
                shutil.copy2(src, dst)
                n += 1
            except (OSError, shutil.Error):
                pass
        return n

    qt_dlls = copy_dll_dir(MXE_QT_BIN, want_prefix="Qt6")
    rt_dlls = copy_dll_dir(MXE_RT_BIN)

    plugin_root = os.path.normpath(os.path.join(MXE_QT_BIN, os.pardir, "plugins"))
    plugin_cats = ("platforms", "imageformats", "iconengines", "styles",
                   "tls", "multimedia", "audio", "mediaservice",
                   "networkinformation", "sqldrivers", "generic")
    plugins_copied = 0
    pi = 0
    while pi < len(plugin_cats):
        cat = plugin_cats[pi]
        pi += 1
        src = os.path.join(plugin_root, cat)
        if not os.path.isdir(src):
            continue
        dst = os.path.join(dst_dir, cat)
        try:
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
            plugins_copied += 1
        except (OSError, shutil.Error):
            pass

    log_info(f"deployed Qt deps to {dst_dir}: "
             f"{qt_dlls} Qt6*.dll + {rt_dlls} runtime DLLs + "
             f"{plugins_copied} plugin categories")


def build_mxe_client(pro_file, build_dir, label):
    """Phase 5 (qmake -> CMake) cross-compile for x86_64 mingw-w64 via
    MXE's `x86_64-w64-mingw32.shared-cmake` wrapper. The wrapper sets
    the right CMAKE_TOOLCHAIN_FILE so Qt6 / std libs are picked up
    from MXE."""
    import cmake_helpers as _ch
    name = f"compile {label} (mxe-x86_64)"
    ensure_dir(build_dir)
    pro_rel = os.path.relpath(pro_file, ROOT).replace(os.sep, "/")
    try:
        target, configure_flags, _output_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return None
    env = os.environ.copy()
    env["PATH"] = MXE_BIN + ":" + MXE_QT_BIN + ":" + env.get("PATH", "")
    mxe_cmake = MXE_ROOT + "/usr/bin/x86_64-w64-mingw32.shared-cmake"
    log_info(f"mxe-cmake configure {label}")
    args = [mxe_cmake, "-S", ROOT, "-B", build_dir,
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCATCHCHALLENGER_NOAUDIO=ON",
            "-DCATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS=OFF",
            "-DCATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS=OFF"]
    args.extend(configure_flags)
    rc, out = run_cmd(args, build_dir, env=env)
    if rc != 0:
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_info(f"mxe-cmake --build -j{NPROC} {label}")
    rc, out = run_cmd([mxe_cmake, "--build", build_dir,
                       "--target", target, "-j", NPROC],
                      build_dir, env=env)
    if rc != 0:
        log_fail(name, f"cmake build failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    exe = find_built_exe(build_dir)
    if exe is None:
        log_fail(name, f"{WIN_EXE_NAME} not produced under {build_dir}")
        return None
    deploy_mxe_dependencies(exe)
    log_pass(name, f"-> {os.path.relpath(exe, ROOT)}")
    return exe


def run_wine_client(exe_path, label, args, timeout=WINE_TIMEOUT,
                    success_marker="MapVisualiserPlayer::mapDisplayedSlot()"):
    name = f"wine run {label}"
    log_info(f"wine64 {os.path.basename(exe_path)} {' '.join(args)}")
    env = os.environ.copy()
    env["WINEPATH"]         = MXE_QT_BIN + ";" + MXE_RT_BIN
    # Silence every wine debug channel.  +seh tracing was tried but it
    # prints the full register set for every frame during SEH unwinds,
    # which on a deep stack can take minutes and effectively prevent the
    # process from terminating in our harness.
    env["WINEDEBUG"]        = "-all"
    # Disable winedbg.exe so an unhandled crash does NOT spawn the
    # interactive crash-dialog GUI (which never auto-closes and would
    # block this harness until manually dismissed).  With winedbg gone
    # wine returns a non-zero exit code as soon as the unhandled
    # exception fires, which is exactly the signal the harness needs.
    env["WINEDLLOVERRIDES"] = "winedbg.exe=d"
    env["QT_QPA_PLATFORM"]  = "offscreen"
    proc = subprocess.Popen(
        [WINE_BIN, exe_path] + list(args),
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)
    output_lines = []
    found = threading.Event()

    def reader():
        while True:
            raw = proc.stdout.readline()
            if not raw:
                break
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if success_marker in line:
                found.set()

    threading.Thread(target=reader, daemon=True).start()
    elapsed = 0.0
    step = 0.5
    while elapsed < timeout:
        if proc.poll() is not None:
            break
        if found.is_set():
            break
        time.sleep(step)
        elapsed += step

    if found.is_set():
        log_pass(name, f"reached map ({success_marker})")
        ok = True
    elif proc.poll() is None:
        log_fail(name, f"timeout {timeout}s without success_marker")
        ok = False
    else:
        log_fail(name, f"exit code {proc.returncode} without success_marker")
        ok = False

    if proc.poll() is None:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            proc.wait(timeout=5)
    if not ok:
        li = max(0, len(output_lines) - 30)
        while li < len(output_lines):
            print(f"  | {output_lines[li]}")
            li += 1
    return ok


def set_http_datapack_mirror(build_dir, value):
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'httpDatapackMirror\s+value="[^"]*"',
                     f'httpDatapackMirror value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)


def start_local_server(build_dir, bin_name=SERVER_BIN_NAME):
    global server_proc
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("server start", f"{bin_name} not found in {build_dir}")
        return None
    log_info(f"starting local server: {binary}")
    server_proc = subprocess.Popen(
        ["nice", "-n", "19", "ionice", "-c", "3", binary], cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=os.setsid)
    ready = threading.Event()
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            if "correctly bind:" in line:
                ready.set()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if ready.wait(timeout=SERVER_READY_TIMEOUT):
        return server_proc
    log_fail("server start", "timeout waiting for 'correctly bind:'")
    stop_local_server()
    return None


def stop_local_server():
    global server_proc
    if server_proc is None:
        return
    try: os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError: pass
    try: server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        server_proc.wait(timeout=5)
    server_proc = None


def cleanup(*_args):
    stop_local_server()
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Windows MXE x86_64 + wine64 testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    if not mxe_available():
        log_info(f"MXE x86_64 ({MXE_QMAKE}) or wine64 ({WINE_BIN}) missing — skipping windows test")
        save_failed_cases()
        summary()
        return

    cpu_exe = None
    gl_exe  = None
    if should_run("compile qtcpu800x600 (mxe-x86_64)", failed_cases):
        cpu_exe = build_mxe_client(CLIENT_CPU_PRO, CLIENT_CPU_BUILD_WIN,
                                   "qtcpu800x600")
    else:
        cpu_exe = find_built_exe(CLIENT_CPU_BUILD_WIN)
    if should_run("compile qtopengl (mxe-x86_64)", failed_cases):
        gl_exe = build_mxe_client(CLIENT_GL_PRO, CLIENT_GL_BUILD_WIN,
                                  "qtopengl")
    else:
        gl_exe = find_built_exe(CLIENT_GL_BUILD_WIN)

    win_dp_src = DATAPACKS[0] if DATAPACKS else None
    win_mc = None
    if win_dp_src is not None:
        mcs = detect_maincodes(win_dp_src)
        if mcs:
            win_mc = mcs[0]
    if cpu_exe is not None and should_run("wine run qtcpu800x600 --autosolo", failed_cases):
        if win_dp_src and win_mc:
            setup_datapack_client(os.path.dirname(cpu_exe), win_dp_src,
                                  win_mc, f"wine qtcpu800x600 ({win_mc})")
        run_wine_client(cpu_exe, "qtcpu800x600 --autosolo",
                        ["--autosolo", "--closewhenonmap"])
    if gl_exe is not None and should_run("wine run qtopengl --autosolo", failed_cases):
        if win_dp_src and win_mc:
            setup_datapack_client(os.path.dirname(gl_exe), win_dp_src,
                                  win_mc, f"wine qtopengl ({win_mc})")
        run_wine_client(gl_exe, "qtopengl --autosolo",
                        ["--autosolo", "--closewhenonmap"])

    win_need_multi = ((cpu_exe is not None and should_run("wine run qtcpu800x600", failed_cases)) or
                      (gl_exe  is not None and should_run("wine run qtopengl", failed_cases)))
    if win_need_multi:
        srv = None
        if os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN_NAME)):
            set_http_datapack_mirror(SERVER_BUILD, "")
            srv = start_local_server(SERVER_BUILD)
        else:
            log_fail("wine run: server start",
                     f"{SERVER_BIN_NAME} not found in {SERVER_BUILD}")
        if srv is not None:
            if cpu_exe is not None and should_run("wine run qtcpu800x600", failed_cases):
                run_wine_client(cpu_exe, "qtcpu800x600",
                                ["--host", SERVER_HOST, "--port", SERVER_PORT,
                                 "--autologin", "--character", "WineCPU",
                                 "--closewhenonmap"])
            if gl_exe is not None and should_run("wine run qtopengl", failed_cases):
                run_wine_client(gl_exe, "qtopengl",
                                ["--host", SERVER_HOST, "--port", SERVER_PORT,
                                 "--autologin", "--character", "WineGL",
                                 "--closewhenonmap"])
            stop_local_server()

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
