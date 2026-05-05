#!/usr/bin/env python3
"""
testingcompilationwindows.py — CatchChallenger Windows cross-compile + run.

Steps:
  1. Probe the dedicated MXE install at <test_config.MXE_PREFIX>
     (built with MXE_TARGETS="x86_64-w64-mingw32.shared") and wine64.
  2. cmake (via MXE wrapper) + ninja build for qtcpu800x600 and qtopengl —
     opts into ccache for compile reuse and `-fuse-ld=mold` when mold is
     available on the host.
  3. Deploy Qt6 + plugins via wine64 windeployqt.exe when present (falls
     back to a manual DLL copy), and always layer MXE's mingw runtime DLLs
     (libstdc++/libgcc/libwinpthread) on top.
  4. Run each .exe under wine64 with QT_QPA_PLATFORM=offscreen — first
     --autosolo (no server), then a multi-mode run against the local
     server-filedb.

Self-skips when MXE qmake or wine64 is missing.

Sibling of testingcompilationmac.py / testingcompilationandroid.py.
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re, time
import build_paths
import diagnostic
from cmd_helpers import clamp_local

build_paths.ensure_root()

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

CLIENT_CPU_BUILD_WIN = build_paths.build_path("client/qtcpu800x600/build/testing-wine64")
CLIENT_GL_BUILD_WIN  = build_paths.build_path("client/qtopengl/build/testing-wine64")
WIN_EXE_NAME         = "catchchallenger.exe"

#local server-filedb that the multi-mode wine client connects back to.
SERVER_BUILD    = build_paths.build_path("server/epoll/build/testing-filedb")
SERVER_BIN_NAME = "catchchallenger-server-cli-epoll"

SERVER_HOST = _config["server_host"]
SERVER_PORT = str(_config["server_port"])

# ── MXE / wine64 ───────────────────────────────────────────────────────────
# Dedicated MXE install for CatchChallenger (built with
# MXE_TARGETS="x86_64-w64-mingw32.shared" MXE_PREFIX=<paths.mxe_prefix>).
# Path comes from the operator's config.json so it stays out of the
# public repo.
import test_config as _tc
MXE_PREFIX  = _tc.MXE_PREFIX
MXE_TARGET  = "x86_64-w64-mingw32.shared"
MXE_HOST    = "x86_64-pc-linux-gnu"
MXE_QMAKE   = MXE_PREFIX + "/" + MXE_TARGET + "/qt6/bin/qmake"
MXE_BIN     = MXE_PREFIX + "/bin"                               # cross wrappers
MXE_QT_BIN  = MXE_PREFIX + "/" + MXE_TARGET + "/qt6/bin"        # Qt6*.dll, .exe
MXE_RT_BIN  = MXE_PREFIX + "/" + MXE_TARGET + "/bin"            # libstdc++ etc.
MXE_CMAKE   = MXE_PREFIX + "/bin/" + MXE_TARGET + "-cmake"
MXE_NINJA   = MXE_PREFIX + "/" + MXE_HOST + "/bin/ninja"
MXE_HOST_BIN = MXE_PREFIX + "/" + MXE_HOST + "/bin"
WINE_BIN    = shutil.which("wine64") or "/etc/eselect/wine/bin/wine64"
CCACHE_BIN  = shutil.which("ccache")
# mingw gcc rejects -fuse-ld=mold ("unrecognized command-line option") because
# mold's PE/COFF backend is still experimental and the mingw driver hasn't
# learned about it.  lld supports PE/COFF natively and is what the mingw
# driver accepts via -fuse-ld=lld, so the MXE build prefers lld here even
# though native Linux builds prefer mold.
LLD_BIN     = shutil.which("ld.lld") or shutil.which("lld")
# windeployqt is a Windows binary — invoke under wine64.  MXE doesn't always
# build qttools, so this path may not exist; the deploy step then falls back
# to the manual DLL copy.
WINDEPLOYQT_EXE = MXE_QT_BIN + "/windeployqt.exe"

# NSIS host binary for installer generation; if absent we fall back to a
# portable .zip so the installer step still produces a single artefact.
MAKENSIS_BIN = (shutil.which("makensis")
                or (MXE_HOST_BIN + "/makensis"
                    if os.path.isfile(MXE_HOST_BIN + "/makensis") else None))

# ── MSI tooling (self-contained, no system install) ────────────────────────
# WiX 3.11 binaries (Windows .NET .exe) at <paths.msi_dir>/wix3/, driven
# via wine64 — the operator does not want anything installed in the host
# system, so the toolchain lives entirely under <paths.msi_dir>. Self-
# signed Authenticode test cert sits next to the WiX dir for code-
# signing the .exe / NSIS installer / .msi via osslsigncode.
MSI_DIR        = _tc.MSI_DIR
WIX_DIR        = MSI_DIR + "/wix3"
WIX_HEAT_EXE   = WIX_DIR + "/heat.exe"
WIX_CANDLE_EXE = WIX_DIR + "/candle.exe"
WIX_LIGHT_EXE  = WIX_DIR + "/light.exe"
SIGN_PFX       = MSI_DIR + "/test-codesign.pfx"
SIGN_PFX_PASS  = "catchchallenger"
OSSLSIGNCODE   = shutil.which("osslsigncode") or "/usr/bin/osslsigncode"
SIGN_TIMESTAMP = "http://timestamp.digicert.com"   # RFC3161 TSA; optional

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
from test_config import FAILED_JSON


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
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


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
    # qt-cmake doesn't add a separate filesystem marker, so qmake's
    # existence remains the cheap "MXE install is healthy" probe.
    return (os.path.isfile(MXE_QMAKE) and os.path.isfile(MXE_CMAKE)
            and os.path.isfile(WINE_BIN))


def find_built_exe(build_dir):
    """Locate catchchallenger.exe inside the MXE-cmake build dir. With
    self-contained per-binary CMakeLists.txt, the .exe lands at
    build_dir/<target>.exe directly (no nested output subdir)."""
    candidates = [build_dir,
                  os.path.join(build_dir, "release"),
                  os.path.join(build_dir, "debug")]
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


def run_windeployqt(exe_path, label):
    """Run windeployqt.exe under wine64 against the freshly built .exe.
    windeployqt copies the right Qt DLLs + plugins next to the binary.
    Returns True on success (and the manual deploy can be skipped),
    False if windeployqt is unavailable or fails — caller falls back to
    deploy_mxe_dependencies()."""
    name = f"windeployqt {label}"
    if not os.path.isfile(WINDEPLOYQT_EXE):
        log_info(f"windeployqt skipped ({WINDEPLOYQT_EXE} not built in MXE)")
        return False
    env = os.environ.copy()
    env["WINEPATH"]         = MXE_QT_BIN + ";" + MXE_RT_BIN
    env["WINEDEBUG"]        = "-all"
    env["WINEDLLOVERRIDES"] = "winedbg.exe=d"
    args = [WINE_BIN, WINDEPLOYQT_EXE,
            "--no-translations", "--no-system-d3d-compiler",
            "--no-opengl-sw", "--compiler-runtime",
            exe_path]
    log_info(f"wine64 windeployqt {os.path.basename(exe_path)}")
    rc, out = run_cmd(args, os.path.dirname(exe_path),
                      timeout=WINE_TIMEOUT, env=env)
    if rc != 0:
        log_fail(name, f"windeployqt failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return False
    log_pass(name)
    return True


def build_mxe_client(pro_file, build_dir, label):
    """Cross-compile for x86_64 mingw-w64 via MXE's
    `x86_64-w64-mingw32.shared-cmake` wrapper (which sets the right
    CMAKE_TOOLCHAIN_FILE so Qt6 / std libs are picked up from MXE).
    Uses ninja + ccache; opts into mold when present."""
    import cmake_helpers as _ch
    name = f"compile {label} (mxe-x86_64)"
    ensure_dir(build_dir)
    pro_rel = os.path.relpath(pro_file, ROOT).replace(os.sep, "/")
    try:
        target, configure_flags, source_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return None
    cmake_source = os.path.join(ROOT, source_subdir)
    env = os.environ.copy()
    # MXE_BIN provides the mingw cross compilers; MXE_HOST_BIN provides
    # the host ninja / cmake; MXE_QT_BIN keeps Qt host tools (moc/uic/rcc)
    # reachable in case the wrapper drops them.
    env["PATH"] = (MXE_BIN + ":" + MXE_HOST_BIN + ":" + MXE_QT_BIN
                   + ":" + env.get("PATH", ""))
    log_info(f"mxe-cmake configure {label} (ninja"
             + (", ccache" if CCACHE_BIN else "")
             + (", lld" if LLD_BIN else "")
             + ")")
    args = [MXE_CMAKE, "-S", cmake_source, "-B", build_dir,
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCATCHCHALLENGER_NOAUDIO=ON",
            "-DCATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS=ON",
            "-DCATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS=ON",
            # Enable in-process server so the --autosolo phase can launch
            # without depending on an external server binary. The
            # O_CLOEXEC #ifndef shim in ClientLoad/ClientHeavyLoadMirror.cpp
            # is what lets these targets compile under MXE/MinGW (no
            # POSIX O_CLOEXEC there). Without SINGLEPLAYER, --autosolo
            # falls through to the multi-player UI flow with no server
            # to connect to and the client gives up at stateChanged(0).
            "-DCATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON",
            "-DCATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER=ON"]
    if os.path.isfile(MXE_NINJA):
        args.extend(["-G", "Ninja",
                     "-DCMAKE_MAKE_PROGRAM=" + MXE_NINJA])
    if CCACHE_BIN:
        args.extend(["-DCMAKE_C_COMPILER_LAUNCHER=" + CCACHE_BIN,
                     "-DCMAKE_CXX_COMPILER_LAUNCHER=" + CCACHE_BIN])
    # -fno-lto disables link-time optimisation for the test build. MXE
    # mingw-gcc loads liblto_plugin.so by default; even when no .o has
    # LTO bytecode, every link spawns lto-wrapper which slows the test
    # cycle and confuses the build-load sampler. Pair the flag with the
    # CMAKE_C/CXX_FLAGS_INIT below so any compiler-side LTO from MXE
    # default flags is also suppressed.
    link_flags_extra = "-fno-lto"
    if LLD_BIN:
        # lld supports PE/COFF natively and is what the mingw driver
        # accepts via -fuse-ld=lld. mold's PE/COFF backend is still
        # experimental and the mingw driver doesn't recognise
        # -fuse-ld=mold, so prefer lld for MXE builds.
        link_flags_extra = "-fuse-ld=lld -fno-lto"
    args.extend(["-DCMAKE_EXE_LINKER_FLAGS=" + link_flags_extra,
                 "-DCMAKE_SHARED_LINKER_FLAGS=" + link_flags_extra,
                 "-DCMAKE_MODULE_LINKER_FLAGS=" + link_flags_extra,
                 "-DCMAKE_C_FLAGS_INIT=-fno-lto",
                 "-DCMAKE_CXX_FLAGS_INIT=-fno-lto"])
    args.extend(configure_flags)
    rc, out = run_cmd(args, build_dir, env=env)
    if rc != 0:
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_info(f"mxe-cmake --build -j{NPROC} {label}")
    rc, out = run_cmd([MXE_CMAKE, "--build", build_dir,
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
    # Try windeployqt first; on failure / absence, fall back to the
    # manual DLL copy so the wine run still has its dependencies.
    if not run_windeployqt(exe, label):
        deploy_mxe_dependencies(exe)
    else:
        # windeployqt only ships Qt + plugins; libstdc++ / libgcc / libwinpthread
        # come from MXE's mingw runtime and are NOT installed by windeployqt.
        # Layer the runtime DLLs on top so the .exe actually starts under wine.
        deploy_mxe_dependencies(exe)
    log_pass(name, f"-> {os.path.relpath(exe, ROOT)}")
    return exe


def build_installer(exe_path, label):
    """Package the deployed exe + Qt DLLs + datapack into a single
    installer artefact next to the build dir.  Prefers NSIS (real
    Windows installer .exe); falls back to a portable .zip when
    makensis isn't available on the host.

    Pre-condition: deploy_mxe_dependencies() and setup_datapack_client()
    have already populated <exe_dir> with everything the installer
    should ship — Qt6*.dll, plugin/ subdirs, runtime DLLs, and
    datapack/internal/."""
    name = f"installer {label}"
    exe_dir = os.path.dirname(exe_path)
    base    = "catchchallenger-" + label
    out_dir = os.path.dirname(exe_dir.rstrip(os.sep)) or exe_dir

    if MAKENSIS_BIN is not None:
        nsi_path = os.path.join(out_dir, base + ".nsi")
        installer_exe = os.path.join(out_dir, base + "-installer.exe")
        nsi = (
            'OutFile "' + installer_exe + '"\n'
            'InstallDir "$PROGRAMFILES64\\CatchChallenger"\n'
            'RequestExecutionLevel admin\n'
            'Section "MainSection" SEC01\n'
            '  SetOutPath "$INSTDIR"\n'
            '  File /r "' + exe_dir + os.sep + '*.*"\n'
            '  CreateShortCut "$DESKTOP\\CatchChallenger.lnk" '
            '"$INSTDIR\\' + WIN_EXE_NAME + '"\n'
            'SectionEnd\n'
        )
        with open(nsi_path, "w") as f:
            f.write(nsi)
        log_info(f"makensis {os.path.basename(nsi_path)}")
        rc, out = run_cmd([MAKENSIS_BIN, "-V2", nsi_path],
                          out_dir, timeout=COMPILE_TIMEOUT)
        if rc == 0 and os.path.isfile(installer_exe):
            log_pass(name, f"-> {os.path.relpath(installer_exe, ROOT)}")
            return installer_exe
        log_fail(name, f"makensis failed (rc={rc}); falling back to zip")
        if out.strip():
            print(out[-2000:])

    zip_base = os.path.join(out_dir, base + "-installer")
    log_info(f"zip {os.path.basename(zip_base)}.zip")
    try:
        archive = shutil.make_archive(zip_base, "zip",
                                      root_dir=os.path.dirname(exe_dir),
                                      base_dir=os.path.basename(exe_dir))
    except (OSError, shutil.Error) as e:
        log_fail(name, f"zip failed: {e}")
        return None
    log_pass(name, f"-> {os.path.relpath(archive, ROOT)}")
    return archive


# ── code signing (osslsigncode, Authenticode) ──────────────────────────────
def sign_authenticode(path, label, kind):
    """Authenticode-sign a PE (.exe) or MSI in place using osslsigncode +
    the self-signed test cert at <paths.msi_dir>/test-codesign.pfx (SIGN_PFX).

    The cert is self-signed, so Windows will still SmartScreen-warn on the
    output — the goal here is to prove the signing pipeline works end-to-end,
    not to ship a Microsoft-trusted artefact.  Replace SIGN_PFX with a real
    Authenticode cert + update SIGN_PFX_PASS for release builds.

    `kind` is just a tag for the log line ('exe' / 'installer' / 'msi')."""
    name = f"sign {kind} {label}"
    if not os.path.isfile(OSSLSIGNCODE):
        log_fail(name, f"osslsigncode not found at {OSSLSIGNCODE}")
        return False
    if not os.path.isfile(SIGN_PFX):
        log_fail(name, f"signing cert missing: {SIGN_PFX}")
        return False
    if not os.path.isfile(path):
        log_fail(name, f"file not found: {path}")
        return False
    signed = path + ".signed"
    args = [OSSLSIGNCODE, "sign",
            "-pkcs12", SIGN_PFX, "-pass", SIGN_PFX_PASS,
            "-h", "sha256",
            "-n", "CatchChallenger",
            "-i", "https://catchchallenger.com/",
            "-in",  path,
            "-out", signed]
    # Try to add a trusted timestamp — TSA may be unreachable behind a
    # restrictive firewall, in which case we fall back to a plain (un-
    # timestamped) signature.  An untimestamped signature is still a valid
    # Authenticode signature; it simply expires when the cert does.
    rc, out = run_cmd(args + ["-ts", SIGN_TIMESTAMP], os.path.dirname(path),
                      timeout=120)
    if rc != 0:
        log_info(f"timestamp failed (rc={rc}); signing without -ts")
        rc, out = run_cmd(args, os.path.dirname(path), timeout=60)
    if rc != 0:
        log_fail(name, f"osslsigncode rc={rc}")
        if out.strip():
            print(out[-1500:])
        return False
    try:
        os.replace(signed, path)
    except OSError as e:
        log_fail(name, f"replace failed: {e}")
        return False
    log_pass(name, f"-> {os.path.relpath(path, ROOT)}")
    return True


# ── MSI generation (WiX under wine64) ───────────────────────────────────────
def _wix_wine_env():
    """Wine env tuned for WiX runs: no debug noise, no winedbg pop-ups,
    Qt DLLs unreachable so WiX itself doesn't accidentally pick them up."""
    env = os.environ.copy()
    env["WINEDEBUG"]        = "-all"
    env["WINEDLLOVERRIDES"] = "winedbg.exe=d"
    env.pop("WINEPATH", None)
    return env


def _write_main_wxs(out_path, label, app_dir_basename):
    """Minimal Product .wxs template.  Hard-codes the install dir under
    ProgramFiles64Folder/CatchChallenger and references the harvested
    `ApplicationFiles` ComponentGroup that heat.exe produces from the
    deployed exe directory.  Generates a stable Product Id (sha-derived)
    so re-builds keep the same upgrade code."""
    import hashlib
    digest = hashlib.sha256(("catchchallenger-" + label).encode()).hexdigest()
    upgrade_code = (digest[0:8] + "-" + digest[8:12] + "-"
                    + digest[12:16] + "-" + digest[16:20] + "-"
                    + digest[20:32]).upper()
    wxs = (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">\n'
        '  <Product Id="*" Name="CatchChallenger ' + label + '" Language="1033" '
        'Version="1.0.0.0" Manufacturer="CatchChallenger" '
        'UpgradeCode="' + upgrade_code + '">\n'
        '    <Package InstallerVersion="500" Compressed="yes" '
        'InstallScope="perMachine" Platform="x64" />\n'
        '    <MajorUpgrade DowngradeErrorMessage="A newer version is already installed." />\n'
        '    <MediaTemplate EmbedCab="yes" />\n'
        '    <Directory Id="TARGETDIR" Name="SourceDir">\n'
        '      <Directory Id="ProgramFiles64Folder">\n'
        '        <Directory Id="INSTALLDIR" Name="CatchChallenger" />\n'
        '      </Directory>\n'
        '      <Directory Id="DesktopFolder" />\n'
        '    </Directory>\n'
        '    <Feature Id="MainFeature" Title="CatchChallenger" Level="1">\n'
        '      <ComponentGroupRef Id="ApplicationFiles" />\n'
        '      <ComponentRef Id="DesktopShortcutComponent" />\n'
        '    </Feature>\n'
        '    <DirectoryRef Id="DesktopFolder">\n'
        '      <Component Id="DesktopShortcutComponent" Guid="*">\n'
        '        <Shortcut Id="DesktopShortcut" '
        'Name="CatchChallenger ' + label + '" '
        'Target="[INSTALLDIR]' + WIN_EXE_NAME + '" '
        'WorkingDirectory="INSTALLDIR" />\n'
        '        <RemoveFolder Id="DesktopFolderRemove" On="uninstall" />\n'
        '        <RegistryValue Root="HKCU" Key="Software\\CatchChallenger\\' + label + '" '
        'Name="installed" Type="integer" Value="1" KeyPath="yes" />\n'
        '      </Component>\n'
        '    </DirectoryRef>\n'
        '  </Product>\n'
        '</Wix>\n')
    with open(out_path, "w") as f:
        f.write(wxs)


def build_msi(exe_path, label):
    """Generate a signed .msi from the deployed exe directory using WiX
    (heat -> candle -> light) under wine64, then Authenticode-sign the
    resulting .msi.

    The exe directory must already contain the deployed Qt6*.dll +
    plugins + datapack/internal/ — that is the runtime layout the .msi
    will install verbatim under %ProgramFiles%\\CatchChallenger\\."""
    name = f"msi {label}"
    if not (os.path.isfile(WIX_CANDLE_EXE) and os.path.isfile(WIX_LIGHT_EXE)
            and os.path.isfile(WIX_HEAT_EXE)):
        log_fail(name, f"WiX not found under {WIX_DIR}")
        return None
    exe_dir = os.path.dirname(exe_path)
    out_dir = os.path.dirname(exe_dir.rstrip(os.sep)) or exe_dir
    work    = os.path.join(out_dir, "msi-" + label)
    if os.path.isdir(work):
        shutil.rmtree(work)
    os.makedirs(work)
    msi_path = os.path.join(out_dir, "catchchallenger-" + label + ".msi")
    main_wxs  = os.path.join(work, "main.wxs")
    files_wxs = os.path.join(work, "files.wxs")
    _write_main_wxs(main_wxs, label, os.path.basename(exe_dir))
    env = _wix_wine_env()

    # 1) heat.exe — harvest exe_dir into a ComponentGroup fragment.
    # heat.exe runs as a Windows binary under wine; its CLI parser treats
    # `/` as a switch indicator, so `-out /mnt/...` looks like `-out`
    # followed by the switch `/mnt`, and heat reports HEAT0114 ("the
    # parameter '-out' must be followed by a file path"). The fix: pass
    # the output as a path RELATIVE to cwd (heat resolves it from there)
    # — there's no leading `/` for the parser to mis-interpret. Same
    # applies to the input dir; convert to a relative path against work.
    files_wxs_rel = os.path.relpath(files_wxs, work)
    exe_dir_rel = os.path.relpath(exe_dir, work)
    log_info(f"wine64 heat.exe dir {os.path.basename(exe_dir)} ({label})")
    rc, out = run_cmd(
        [WINE_BIN, WIX_HEAT_EXE, "dir", exe_dir_rel,
         "-nologo", "-gg", "-srd", "-sreg", "-scom", "-ke", "-sfrag", "-suid",
         "-cg", "ApplicationFiles",
         "-dr", "INSTALLDIR",
         "-var", "var.SourceDir",
         "-out", files_wxs_rel],
        work, timeout=COMPILE_TIMEOUT, env=env)
    if rc != 0 or not os.path.isfile(files_wxs):
        log_fail(name, f"heat failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None

    # 2) candle.exe — compile both .wxs into .wixobj. Run with cwd=work
    # and use relative paths for the inputs *and* -dSourceDir so the
    # leading `/` of any absolute path never reaches candle's Windows-
    # style switch parser. Without this, candle sees `/mnt/...` as a
    # `/mnt` switch and silently skips the source — producing no .wixobj
    # output and making the next light.exe invocation fail with
    # LGHT0103 ("cannot find main.wixobj"). We also pass an explicit
    # output dir of "." (relative); candle defaults to writing .wixobj
    # next to .wxs, which is what light expects.
    main_wxs_rel = os.path.relpath(main_wxs, work)
    files_wxs_rel = os.path.relpath(files_wxs, work)
    exe_dir_rel = os.path.relpath(exe_dir, work)
    log_info(f"wine64 candle.exe ({label})")
    rc, out = run_cmd(
        [WINE_BIN, WIX_CANDLE_EXE, "-nologo",
         "-arch", "x64",
         "-dSourceDir=" + exe_dir_rel,
         main_wxs_rel, files_wxs_rel],
        work, timeout=COMPILE_TIMEOUT, env=env)
    if rc != 0:
        log_fail(name, f"candle failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None

    # 3) light.exe — link .wixobj -> .msi.  -sice:* suppresses harmless
    # ICE warnings (e.g. ICE61 about MajorUpgrade w/ same-ver) that would
    # otherwise be flagged on a self-built test package.  -sval skips the
    # MSI validation suite which can't run reliably under wine.
    # Same Linux-path / Windows-switch-parser issue as heat.exe — `-out
    # /mnt/...` makes light treat `/mnt` as a switch and emit LGHT0114
    # ("must be followed by a file path"). Pass a path RELATIVE to cwd
    # so the leading `/` never reaches the parser; the .wixobj inputs
    # are also relative for the same reason.
    msi_path_rel = os.path.relpath(msi_path, work)
    main_wixobj_rel = os.path.relpath(os.path.join(work, "main.wixobj"), work)
    files_wixobj_rel = os.path.relpath(os.path.join(work, "files.wixobj"), work)
    log_info(f"wine64 light.exe -> {os.path.basename(msi_path)}")
    rc, out = run_cmd(
        [WINE_BIN, WIX_LIGHT_EXE, "-nologo",
         "-sval", "-spdb",
         "-sice:ICE61", "-sice:ICE69",
         "-out", msi_path_rel,
         main_wixobj_rel,
         files_wixobj_rel],
        work, timeout=COMPILE_TIMEOUT, env=env)
    if rc != 0 or not os.path.isfile(msi_path):
        log_fail(name, f"light failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_pass(name, f"-> {os.path.relpath(msi_path, ROOT)}")

    # 4) sign the .msi (osslsigncode handles MSI just like PE).
    sign_authenticode(msi_path, label, "msi")
    return msi_path


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
    # Even with QT_QPA_PLATFORM=offscreen, wine's winex11.drv attaches to
    # the host X server when DISPLAY is set, so Qt's offscreen platform
    # still ends up sharing an X connection and the host sees windows
    # flicker on the desktop. Strip DISPLAY (and the matching XAUTHORITY
    # / WAYLAND_DISPLAY) so wine has no GUI server to talk to and the
    # offscreen run is truly headless. WINEDLLOVERRIDES + winedbg=d
    # handles the unhandled-exception dialog already; removing the
    # display only kills *real* window creation, not the harness.
    for _gui_var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(_gui_var, None)
    win_args = [WINE_BIN, exe_path] + list(args)
    diagnostic.record_cmd(win_args, None)
    proc = subprocess.Popen(
        win_args,
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
    srv_args = ["nice", "-n", "19", "ionice", "-c", "3", binary]
    diagnostic.record_cmd(srv_args, build_dir)
    server_proc = subprocess.Popen(
        srv_args, cwd=build_dir,
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

    # Sign the catchchallenger.exe BEFORE bundling — both the NSIS
    # installer and the MSI then ship a signed inner binary.  Done after
    # the autosolo wine run so the unsigned binary is what we exercise
    # for runtime smoke; signing only mutates the embedded Authenticode
    # blob, but doing it post-run keeps build-vs-test steps separable.
    if cpu_exe is not None and should_run("sign exe qtcpu800x600", failed_cases):
        sign_authenticode(cpu_exe, "qtcpu800x600", "exe")
    if gl_exe is not None and should_run("sign exe qtopengl", failed_cases):
        sign_authenticode(gl_exe, "qtopengl", "exe")

    # Installer step — runs after the autosolo phase so the staged
    # <exe_dir>/ already has Qt DLLs (windeployqt + manual) AND
    # datapack/internal/ in place.  The installer ships the whole dir.
    cpu_installer = None
    gl_installer  = None
    if cpu_exe is not None and should_run("installer qtcpu800x600", failed_cases):
        cpu_installer = build_installer(cpu_exe, "qtcpu800x600")
    if gl_exe is not None and should_run("installer qtopengl", failed_cases):
        gl_installer = build_installer(gl_exe, "qtopengl")
    # Sign the NSIS .exe installer (if NSIS produced one — the .zip
    # fallback is just an archive and isn't an Authenticode target).
    if cpu_installer and cpu_installer.endswith(".exe") \
       and should_run("sign installer qtcpu800x600", failed_cases):
        sign_authenticode(cpu_installer, "qtcpu800x600", "installer")
    if gl_installer and gl_installer.endswith(".exe") \
       and should_run("sign installer qtopengl", failed_cases):
        sign_authenticode(gl_installer, "qtopengl", "installer")

    # MSI step — same staged exe_dir as the NSIS installer, packaged via
    # WiX under wine64.  build_msi() signs the .msi internally.
    if cpu_exe is not None and should_run("msi qtcpu800x600", failed_cases):
        build_msi(cpu_exe, "qtcpu800x600")
    if gl_exe is not None and should_run("msi qtopengl", failed_cases):
        build_msi(gl_exe, "qtopengl")

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
