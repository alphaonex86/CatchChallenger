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
import process_helpers
import wall_cap
wall_cap.arm()
import cleanup_helpers
sys.dont_write_bytecode = True


import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re, time, base64, tempfile
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
# server-gui.pro is a virtual key — see cmake_helpers._PRO_TO_CMAKE
# for the real CMakeLists.txt / target mapping.
SERVER_GUI_PRO = os.path.join(ROOT, "server/server-gui.pro")

CLIENT_CPU_BUILD_WIN = build_paths.build_path("client/qtcpu800x600/build/testing-wine64")
CLIENT_GL_BUILD_WIN  = build_paths.build_path("client/qtopengl/build/testing-wine64")
SERVER_GUI_BUILD_WIN = build_paths.build_path("server/build/testing-wine64")
WIN_EXE_NAME         = "catchchallenger.exe"
# Output filenames inside the combined installer payload (post-stage
# rename — see _stage_combined_payload). qtopengl + qtcpu800x600 both
# emit a binary literally named "catchchallenger.exe" from cmake's
# OUTPUT_NAME override; one of them has to be renamed to avoid a
# filesystem collision when they sit in the same install directory.
COMBINED_BIN_CPU = "catchchallengercpu800x600.exe"   # qtcpu800x600
COMBINED_BIN_GL  = "catchchallenger.exe"             # qtopengl, primary
COMBINED_BIN_SRV = "catchchallenger-server-gui.exe"

#local server-filedb that the multi-mode wine client connects back to.
SERVER_BUILD    = build_paths.build_path("server/cli/build/testing-filedb")
SERVER_BIN_NAME = "catchchallenger-server-cli"

# Combined-payload + installer/MSI scratch dirs live at the parent
# of CLIENT_GL_BUILD_WIN so the post-success sweep cleans them with
# everything else. The actual installer + .msi land at the
# /mnt/data/perso/tmpfs root via cleanup_helpers.promote_artifact.
COMBINED_STAGE_DIR = build_paths.build_path("client/build/combined-install-stage")
COMBINED_NSIS_DIR  = build_paths.build_path("client/build/combined-installer")
COMBINED_MSI_DIR   = build_paths.build_path("client/build/combined-msi")

# Register PRIVATE build dirs for atexit cleanup. SERVER_BUILD
# (= testing-filedb) is shared with native-Linux tests and is NOT
# registered here; the all.sh post-success sweep wipes it.
cleanup_helpers.register_build_dir(CLIENT_CPU_BUILD_WIN)
cleanup_helpers.register_build_dir(CLIENT_GL_BUILD_WIN)
cleanup_helpers.register_build_dir(SERVER_GUI_BUILD_WIN)
cleanup_helpers.register_build_dir(CLIENT_CPU_BUILD_WIN + "-install-stage")
cleanup_helpers.register_build_dir(CLIENT_GL_BUILD_WIN  + "-install-stage")
cleanup_helpers.register_build_dir(COMBINED_STAGE_DIR)
cleanup_helpers.register_build_dir(COMBINED_NSIS_DIR)
cleanup_helpers.register_build_dir(COMBINED_MSI_DIR)

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

# Standard WINEDLLOVERRIDES applied to every wine64 launch performed
# by this script. Three categories rolled into one string:
#   - winedbg.exe=d   : disable wine's interactive crash dialog so an
#                       unhandled exception lands as a non-zero exit
#                       code instead of blocking the harness on a GUI
#                       prompt that no operator is there to dismiss.
#   - mmdevapi=d      : Multimedia Device API. Disabling it makes wine
#                       enumerate zero audio endpoints, so QAudioOutput
#                       / DirectSound / WASAPI initialisation returns
#                       cleanly without binding any host backend.
#   - winealsa.drv,winepulse.drv,wineoss.drv,winecoreaudio.drv=d :
#                       belt-and-braces — even with mmdevapi gone, the
#                       wine driver DLLs can be probed at startup and
#                       emit ALSA / PulseAudio errors on the host's
#                       sound stack. Disabling them silences those.
#   - winemenubuilder.exe=d : winemenubuilder is the wine component that
#                       runs on EVERY .exe launch and writes Start-Menu /
#                       file-association `.desktop` entries into the HOST's
#                       ~/.local/share/applications (and ~/Desktop), printing
#                       as it goes. On a headless test box that is pure
#                       spam + desktop pollution (hundreds of
#                       wine-extension-*.desktop files). Disabling it stops
#                       wine from touching the host desktop at all.
# Segments separated by `;`; each segment is `<comma-list>=<mode>`.
# `mode=d` means "do not load this module at all".
WINE_DLLOVERRIDES_BASE = (
    "winedbg.exe=d;"
    "winemenubuilder.exe=d;"
    "mmdevapi=d;"
    "winealsa.drv,winepulse.drv,wineoss.drv,winecoreaudio.drv=d"
)
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
# MXE actually ships makensis as a Linux ELF in the target/bin/ tree
# (`<MXE>/x86_64-w64-mingw32.shared/bin/makensis`), even though the
# directory name suggests cross output. Probe the host bin tree first
# (some operators install a system makensis), then both MXE locations.
def _resolve_makensis():
    cand = shutil.which("makensis")
    if cand:
        return cand
    for p in (MXE_HOST_BIN + "/makensis",
              MXE_RT_BIN + "/makensis"):
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return None
MAKENSIS_BIN = _resolve_makensis()

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
# Cold-cache server boot on this host emits ~80–100 "Unable to open
# the xml file" warnings (one per skill in the test datapack — see
# CLAUDE.md "datapack/map/main/test/" intentional bugs) before the
# bind line lands; that takes ~60s on a busy box. 60s used to be the
# limit and we'd miss bind by a few seconds. 180s leaves ample
# margin without making a stuck server noticeably worse — the start
# helper kills the binary on the upper bound either way.
SERVER_READY_TIMEOUT = 180
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


import failed_cases as _fc
import phase_timer


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


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
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def force_headless_env(env=None):
    """Return a copy of `env` (or os.environ) forced fully HEADLESS so NO
    spawned command — wine .exe, Qt binary, build tool — can open a window
    on, or even talk to, the host X / Wayland server.

    Every command this harness runs is meant to be headless; a GUI binary
    under wine attaches winex11.drv to the host display whenever DISPLAY is
    set, which floods the operator console with fixme/err spam and pops
    flickering windows on the desktop. So unconditionally:
      * DISPLAY=""               — no X display to connect to
      * QT_QPA_PLATFORM=offscreen — Qt uses the offscreen platform plugin
      * drop XAUTHORITY / WAYLAND_DISPLAY — leave no other GUI handle
      * WINEDEBUG defaults to -all (kept if the caller set it, e.g. the
        +loaddll DLL-trace harness needs its own value)
    Applied at run_cmd and every subprocess site so it covers EVERY command."""
    e = dict(env) if env is not None else dict(os.environ)
    e["DISPLAY"] = ""
    e["QT_QPA_PLATFORM"] = "offscreen"
    e.pop("XAUTHORITY", None)
    e.pop("WAYLAND_DISPLAY", None)
    e.setdefault("WINEDEBUG", "-all")
    # Disable winemenubuilder & co on EVERY wine launch (incl. paths whose
    # env builder forgot to set WINEDLLOVERRIDES) so wine never writes
    # .desktop spam onto the host desktop. setdefault → an explicit
    # WINEDLLOVERRIDES (e.g. the WiX run that also disables mscoree/mshtml)
    # is preserved untouched.
    e.setdefault("WINEDLLOVERRIDES", WINE_DLLOVERRIDES_BASE)
    return e


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(args, cwd)
    try:
        p = subprocess.run(args, cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=force_headless_env(env))
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


# Datapack file-extension whitelist for cross-build installers.
# Operator policy: the windows installer / macOS .dmg / android .apk
# all ship a *minimal* datapack — only files the client actually
# loads at runtime. README.md / *.po / *.ts / *.git / build-script
# leftovers are excluded.
_DATAPACK_KEEP_EXT = {
    "tmx", "xml", "tsx", "js",
    "png", "jpg", "gif",
    "ogg", "opus",
}


def _datapack_ignore(src, names):
    """shutil.copytree(ignore=…) callable. Rules, in order:
      1. Any entry starting with `.` (unix-hidden) → EXCLUDED. This
         covers .git, .gitignore, .qt, .ninja_*, .DS_Store, …
         regardless of whether it's a file or a directory.
      2. Directories that survive (1) → KEPT (recurse). Without
         passing dirs through, the whole subtree disappears.
      3. Regular files → kept only when the extension is in
         _DATAPACK_KEEP_EXT (lower-cased, leading dot stripped).
         Everything else is EXCLUDED."""
    out = []
    ni = 0
    while ni < len(names):
        n = names[ni]
        ni += 1
        if n.startswith("."):
            out.append(n)
            continue
        full = os.path.join(src, n)
        if os.path.isdir(full):
            continue
        ext = os.path.splitext(n)[1].lower().lstrip(".")
        if ext not in _DATAPACK_KEEP_EXT:
            out.append(n)
    return out


def setup_datapack_client(build_dir, datapack_src, maincode, label):
    """Copy datapack to <build_dir>/datapack/, keep only maincode in map/main/.
    Filters by file extension via _datapack_ignore — installer-bound
    datapacks drop README.md, .po, .ts and other non-runtime files.

    Shipped layout in the installer is `<install>/datapack/` (no
    `internal/` level). The server-gui binary searches
    `<bin_dir>/datapack/internal/` then `<bin_dir>/datapack/` and
    picks whichever exists — so the installer-flat layout is the
    fallback. The client (autosolo) reads from
    `applicationDirPath()/datapack/` directly."""
    dst = os.path.join(build_dir, "datapack")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    if not os.path.isdir(datapack_src):
        log_fail(f"datapack {label}", f"source not found: {datapack_src}")
        return False
    log_info(f"copying datapack to {dst} (ext whitelist: "
             f"{','.join(sorted(_DATAPACK_KEEP_EXT))})")
    shutil.copytree(datapack_src, dst, ignore=_datapack_ignore)
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


def find_built_exe(build_dir, exe_name=WIN_EXE_NAME):
    """Locate <exe_name> inside the MXE-cmake build dir. With
    self-contained per-binary CMakeLists.txt, the .exe lands at
    build_dir/<target>.exe directly (no nested output subdir).
    Caller can override exe_name for binaries whose target name
    differs from `catchchallenger` (e.g. server-gui ships as
    `catchchallenger-server-gui.exe`)."""
    candidates = [build_dir,
                  os.path.join(build_dir, "release"),
                  os.path.join(build_dir, "debug")]
    idx = 0
    while idx < len(candidates):
        cand = os.path.join(candidates[idx], exe_name)
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


def _deploy_mingw_runtime(exe_path):
    """Copy the MinGW C/C++ runtime DLLs that windeployqt does NOT
    handle (it scopes itself to Qt). Without these the .exe fails to
    start under wine with "DLL not found". Specifically:
    libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll, plus
    libssp-0.dll when MXE was built with -fstack-protector.

    Cheaper alternative to deploy_mxe_dependencies(): we only copy this
    small fixed set, not every .dll under MXE_QT_BIN + MXE_RT_BIN."""
    dst_dir = os.path.dirname(exe_path)
    runtime = ("libgcc_s_seh-1.dll", "libstdc++-6.dll",
               "libwinpthread-1.dll", "libssp-0.dll")
    copied = 0
    for fn in runtime:
        for src_root in (MXE_RT_BIN, MXE_QT_BIN):
            src = os.path.join(src_root, fn)
            if os.path.isfile(src):
                try:
                    shutil.copy2(src, os.path.join(dst_dir, fn))
                    copied += 1
                except (OSError, shutil.Error):
                    pass
                break
    log_info(f"deployed MinGW runtime: {copied}/{len(runtime)} dll(s) "
             f"to {dst_dir}")


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
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
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
            "-DCMAKE_BUILD_TYPE=Release",
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
        import shlex as _shlex
        cmd_str = " ".join(_shlex.quote(a) for a in args)
        _fc.set_extras(name, cmd=cmd_str, compile_output=(out or ""))
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_info(f"mxe-cmake --build -j{NPROC} {label}")
    build_args = [MXE_CMAKE, "--build", build_dir, "--target", target, "-j", NPROC]
    rc, out = run_cmd(build_args, build_dir, env=env)
    if rc != 0:
        if shutil.which("ccache"):
            log_info(f"MXE build failed; flushing ccache and retrying once ({label})")
            try:
                subprocess.run(["ccache", "-C"], capture_output=True,
                               text=True, timeout=120)
            except (OSError, subprocess.SubprocessError) as exc:
                log_info(f"ccache -C raised {exc!r}; retrying anyway")
            rc, out = run_cmd(build_args, build_dir, env=env)
        if rc != 0:
            import shlex as _shlex
            cmd_str = " ".join(_shlex.quote(a) for a in build_args)
            _fc.set_extras(name, cmd=cmd_str, compile_output=(out or ""))
            log_fail(name, f"cmake build failed (rc={rc}) [retried after ccache -C]")
            if out.strip():
                print(out[-2000:])
            return None
    # server-gui's CMake target name is catchchallenger-server-gui,
    # not catchchallenger; pass the right filename to find_built_exe.
    expected_exe = (f"{target}.exe"
                    if target == "catchchallenger-server-gui"
                    else WIN_EXE_NAME)
    exe = find_built_exe(build_dir, exe_name=expected_exe)
    if exe is None:
        log_fail(name, f"{expected_exe} not produced under {build_dir}")
        return None
    # Prefer windeployqt — it knows precisely which Qt6 DLLs + which
    # plugin sub-dirs a given .exe needs and deposits ONLY those next to
    # the binary. deploy_mxe_dependencies is the fallback: it dumps the
    # whole MXE bin/ (Qt + ffmpeg + hdf5 + GLEW + glfw3 + ALURE32 + …),
    # which bloats the installer past the 100 MiB shipping ceiling.
    # When windeployqt succeeds we therefore only add the small MinGW
    # runtime trio that windeployqt does not ship.
    if run_windeployqt(exe, label):
        _deploy_mingw_runtime(exe)
    else:
        deploy_mxe_dependencies(exe)
    log_pass(name, f"-> {os.path.relpath(exe, ROOT)}")
    _verify_exe_size(label, exe)
    return exe


# ── shipping-artifact size guards ────────────────────────────────────
# Each step that produces a deployable artefact (.exe / .zip / .msi)
# checks the result against a static baseline in size_check.py:
# regression > 25% or absolute size < 10 MiB → log_fail. This catches
# silent breakage where the artefact builds successfully but is empty
# (e.g. windeployqt skipped, a Qt plugin dir missed, msi without the
# datapack). Lazy import keeps the module-load surface lean for the
# 99% of runs where size_check isn't touched.
def _verify_exe_size(label, exe_path):
    import size_check
    sk = "windows.exe." + label
    ok, detail = size_check.verify(sk, exe_path)
    (log_pass if ok else log_fail)(f"size {sk}", detail)


def _verify_installer_size(label, installer_path):
    import size_check
    sk = "windows.installer." + label
    ok, detail = size_check.verify(sk, installer_path)
    (log_pass if ok else log_fail)(f"size {sk}", detail)


def _verify_msi_size(label, msi_path):
    import size_check
    sk = "windows.msi." + label
    ok, detail = size_check.verify(sk, msi_path)
    (log_pass if ok else log_fail)(f"size {sk}", detail)


_INSTALL_EXCLUDE_DIRS = {
    "CMakeFiles", ".qt", "_libzstd",
    "catchchallenger_qfakesocket_autogen",
    "catchchallenger_qt_lib_autogen",
    "catchchallenger_tiled_autogen",
    "catchchallenger800x600_autogen",
    "catchchallenger_qtopengl_autogen",
}
_INSTALL_EXCLUDE_DIR_SUFFIXES = ("_autogen",)
_INSTALL_EXCLUDE_FILES = {
    ".ninja_deps", ".ninja_log", "build.ninja",
    "cmake_install.cmake", "CMakeCache.txt", "compile_commands.json",
}
# Build-time artefacts ALWAYS excluded by file extension, regardless of
# their basename. Static-library archives in particular were being
# shipped in the installer: libcatchchallenger_qt_lib.a alone is
# 319 MiB (it contains every Qt-AUTOGEN moc/uic/rcc object the client
# was linked from); libcatchchallenger_tiled.a adds another 112 MiB.
# Neither is loaded at runtime by the .exe — they're inputs to the
# linker, not inputs to the loader. Excluding them drops the
# installer from ~125 MiB to ~30 MiB.
_INSTALL_EXCLUDE_FILE_SUFFIXES = (
    ".a",        # static library archive (mingw / MXE ar output)
    ".lib",      # static lib (MS-style; rare on MXE but cheap to gate)
    ".obj",      # bare COFF object
    ".o",        # ELF object (shouldn't be in a MXE build dir, but…)
    ".rsp",      # ninja link-response files
    ".d",        # gcc dependency files
    ".dwo",      # split-debug dwarf objects
    ".pdb",      # MSVC-style debug info (mingw doesn't emit these,
                 #  but harmless to gate)
    ".manifest", # .exe.manifest sidecars (MSVC pkg metadata)
)

# .dll list is computed at runtime per build (NOT hardcoded). Two passes:
#  1) Static — PE imports of the .exe and of every recursively-needed
#     .dll, harvested via `objdump -p`. Catches DLLs the linker put in
#     the import directory.
#  2) Runtime — run the .exe once under wine with WINEDEBUG=+loaddll
#     (autosolo for the GUI client; --version for the server CLI) and
#     parse the "load_dll" trace lines. Catches LoadLibrary'd DLLs that
#     aren't in any PE import table (Qt plugins, GL/audio backends,
#     etc.).
# The union of (1) and (2) is the install allowlist for THIS build.

_OBJDUMP = shutil.which("x86_64-w64-mingw32.shared-objdump") \
        or shutil.which("x86_64-w64-mingw32-objdump") \
        or shutil.which("objdump")


def _pe_imports(pe_path):
    """Return a set of lowercased DLL basenames imported by pe_path's
    PE import directory. Empty set if objdump is missing or fails."""
    if _OBJDUMP is None or not os.path.isfile(pe_path):
        return set()
    rc, out = run_cmd([_OBJDUMP, "-p", pe_path],
                      os.path.dirname(pe_path), timeout=30)
    if rc != 0:
        return set()
    deps = set()
    li = 0
    lines = out.splitlines()
    while li < len(lines):
        line = lines[li].strip()
        li += 1
        # objdump format:  "\tDLL Name: foo.dll"
        if line.startswith("DLL Name:"):
            name = line.split(":", 1)[1].strip().lower()
            if name:
                deps.add(name)
    return deps


def _pe_imports_recursive(exe_path, search_dir):
    """Walk PE imports starting at exe_path, resolving each named DLL
    inside search_dir, and recursing into any DLL we ship. System DLLs
    (kernel32.dll, advapi32.dll, …) live in C:\\Windows and are never
    bundled — they're skipped silently. Returns the closure set of
    lowercased DLL basenames that DO live in search_dir."""
    seen = set()
    todo = [exe_path]
    while todo:
        pe = todo.pop()
        for dep in _pe_imports(pe):
            if dep in seen:
                continue
            local = os.path.join(search_dir, dep)
            if not os.path.isfile(local):
                # Best-effort case-insensitive match (objdump preserves
                # the casing from the import directory; the file on disk
                # may differ).
                hit = None
                try:
                    for cand in os.listdir(search_dir):
                        if cand.lower() == dep:
                            hit = os.path.join(search_dir, cand); break
                except OSError:
                    pass
                if hit is None:
                    continue
                local = hit
                dep = os.path.basename(local).lower()
            seen.add(dep)
            todo.append(local)
    return seen


def _wine_runtime_dlls(exe_path, exe_dir, args):
    """Run exe_path under wine64 with WINEDEBUG=+loaddll and harvest
    the lowercased basenames of every DLL wine actually mapped during
    that run. Used to catch LoadLibrary'd DLLs that aren't in the .exe's
    PE imports (Qt plugins, audio/GL backends, sqldrivers). args is the
    command-line tail (e.g. ["--version"] or ["--autosolo"]).
    Returns an empty set if wine64 isn't available.

    Hard 3-second wall-clock cap: --version is a one-shot path; if the
    GUI client ignores it and falls through to showMaximized() we still
    capture enough load-DLL trace inside 3 s. Also strips DISPLAY/
    XAUTHORITY/WAYLAND_DISPLAY and adds the Qt CLI `-platform offscreen`
    flag so wine's winex11.drv has no host display to attach to, even
    when QT_QPA_PLATFORM env doesn't propagate through wine."""
    if not WINE_BIN or not os.path.isfile(WINE_BIN):
        return set()
    if not os.path.isfile(exe_path):
        return set()
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"]  = "offscreen"
    env["WINEDEBUG"]        = "+loaddll"
    env["WINEDLLOVERRIDES"] = "mscoree,mshtml=d;" + WINE_DLLOVERRIDES_BASE
    for _gui_var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(_gui_var, None)
    win_args = [WINE_BIN, exe_path, "-platform", "offscreen"] + list(args)
    rc, out = run_cmd(win_args, exe_dir, timeout=3, env=env)
    # wine traces look like:
    #   trace:loaddll:load_native_dll Loaded L"/.../qt6core.dll" at 0x...:
    #   trace:module:load_builtin_dll Loaded L"C:\\windows\\system32\\..."
    # We only want DLLs we could legitimately ship — those that live
    # inside exe_dir's tree (the MXE-deployed runtime + Qt deps).
    dlls = set()
    li = 0
    for line in out.splitlines():
        if "Loaded" not in line:
            continue
        # Pick out the quoted path. Wine uses L"..." or plain "...".
        q1 = line.find('"')
        q2 = line.find('"', q1 + 1) if q1 >= 0 else -1
        if q1 < 0 or q2 < 0:
            continue
        path = line[q1+1:q2]
        base = os.path.basename(path).lower()
        if base.endswith(".dll"):
            dlls.add(base)
    return dlls


_PLUGIN_SUBDIRS = ("platforms", "imageformats", "iconengines", "styles",
                   "tls", "multimedia", "audio", "mediaservice",
                   "networkinformation", "sqldrivers", "generic")


def _plugin_dep_closure(exe_dir):
    """Top-level DLLs that the shipped PLUGINS (platforms/qoffscreen.dll,
    sqldrivers/qsqlite.dll, …) depend on.

    Plugins are loaded at runtime via QPluginLoader, so they never appear
    in the .exe's PE import table — but their own dependencies must still
    ship next to the .exe or the plugin fails to load. The concrete bug
    this closes: sqldrivers/qsqlite.dll imports libsqlite3-0.dll; without
    it the SQLite driver reports "Driver not loaded" and the server's DB
    init fails. Walk every plugin DLL's import closure against exe_dir and
    return the deps that live there."""
    deps = set()
    si = 0
    while si < len(_PLUGIN_SUBDIRS):
        sub = os.path.join(exe_dir, _PLUGIN_SUBDIRS[si])
        si += 1
        if not os.path.isdir(sub):
            continue
        try:
            entries = os.listdir(sub)
        except OSError:
            entries = []
        ei = 0
        while ei < len(entries):
            pe = entries[ei]
            ei += 1
            if pe.lower().endswith(".dll"):
                deps |= _pe_imports_recursive(os.path.join(sub, pe), exe_dir)
    return deps


def _compute_dll_allowlist(exe_path, exe_dir):
    """Combine static PE-import closure (1) with runtime wine-loaded set
    (2) and the plugin dependency closure (3). Returned set is the .dll
    basenames (lowercase) the installer must ship. Plugins themselves go
    through directory pass-through and aren't on this list — only the
    top-level .dll files alongside the .exe."""
    static = _pe_imports_recursive(exe_path, exe_dir)
    static |= _plugin_dep_closure(exe_dir)
    # Runtime pass: --version is a 1-shot path that still loads every
    # Qt platform plugin + GL backend on the GUI client (the splash
    # screen ctor pulls them in even when we exit early). --autosolo
    # would be more thorough but costs ~30 s; --version captures the
    # essential init path and finishes in <2 s.
    runtime = _wine_runtime_dlls(exe_path, exe_dir, ["--version"])
    # Filter the runtime set: keep only DLLs that live in exe_dir
    # (system DLLs from C:\Windows aren't shippable and aren't ours).
    runtime_local = set()
    try:
        on_disk = {n.lower() for n in os.listdir(exe_dir)}
    except OSError:
        on_disk = set()
    for d in runtime:
        if d in on_disk:
            runtime_local.add(d)
    return static | runtime_local


def _stage_install_payload(src_dir, exe_name=WIN_EXE_NAME):
    """Mirror src_dir to a sibling clean-install dir, dropping every
    build-system artefact (CMakeFiles, *_autogen, .ninja_*, CMakeCache,
    compile_commands.json) and every DLL not in the per-build allowlist.
    The .exe, the datapack/, and every Qt plugin sub-dir are kept
    verbatim. Returns the staging dir path.

    exe_name is the binary whose import closure seeds the allowlist; it
    defaults to the client (WIN_EXE_NAME) but MUST be passed for the
    server-gui part (catchchallenger-server-gui.exe) — otherwise the
    allowlist is computed against a non-existent catchchallenger.exe,
    comes back empty, and the server's unique DLLs (e.g. libsqlite3-0.dll)
    never ship."""
    # Source build dir can legitimately be gone before/while we stage: it's
    # registered for cleanup-on-exit, and a concurrent run (or all.sh) that
    # finishes can sweep the shared tmpfs dir mid-package. Return None
    # instead of letting an unhandled FileNotFoundError abort the whole
    # script; the caller treats None as "couldn't stage this part".
    if not os.path.isdir(src_dir):
        log_info(f"install stage: source build dir missing ({src_dir}) — "
                 "cannot stage this part")
        return None
    stage = src_dir.rstrip(os.sep) + "-install-stage"
    if os.path.isdir(stage):
        shutil.rmtree(stage, ignore_errors=True)
    os.makedirs(stage, exist_ok=True)
    exe_path = os.path.join(src_dir, exe_name)
    allow = _compute_dll_allowlist(exe_path, src_dir)
    log_info(f"install allowlist: {len(allow)} DLL(s) "
             f"({sum(1 for x in allow if x.startswith('qt6'))} Qt6, "
             f"{sum(1 for x in allow if not x.startswith('qt6'))} runtime)")
    try:
        entries = os.listdir(src_dir)
    except FileNotFoundError:
        log_info(f"install stage: {src_dir} vanished mid-stage — failing part")
        return None
    for entry in entries:
        s = os.path.join(src_dir, entry)
        d = os.path.join(stage, entry)
        try:
            if os.path.isdir(s):
                if entry in _INSTALL_EXCLUDE_DIRS:
                    continue
                if any(entry.endswith(suf) for suf in _INSTALL_EXCLUDE_DIR_SUFFIXES):
                    continue
                shutil.copytree(s, d, symlinks=True)
                continue
            if entry in _INSTALL_EXCLUDE_FILES:
                continue
            low = entry.lower()
            if any(low.endswith(suf) for suf in _INSTALL_EXCLUDE_FILE_SUFFIXES):
                continue
            # screenshot-*.png are wine-smoke-test artefacts produced by
            # the QT_QPA_PLATFORM=offscreen runs (see run_wine_screenshot).
            # They live next to the binary in the build dir but must NOT
            # ship inside the installer / .msi.
            if low.startswith("screenshot-") and low.endswith(".png"):
                continue
            if low.endswith(".dll"):
                if low not in allow:
                    continue
            shutil.copy2(s, d, follow_symlinks=True)
        except (OSError, shutil.Error):
            # entry vanished mid-copy (concurrent cleanup) or unreadable —
            # skip it; a fully-vanished src is caught by the exe post-check.
            pass
    # If the primary .exe didn't make it into the stage the payload is
    # unusable (src was being wiped while we copied) — signal failure so
    # the caller skips the combined installer rather than shipping a
    # half-empty one.
    if not os.path.isfile(os.path.join(stage, exe_name)):
        log_info(f"install stage: {exe_name} missing from stage "
                 f"({src_dir} incomplete) — failing part")
        return None
    return stage


_MXE_STRIP   = MXE_BIN + "/" + MXE_TARGET + "-strip"
_MXE_OBJCOPY = MXE_BIN + "/" + MXE_TARGET + "-objcopy"


def _strip_debug_symbols(stage_dir):
    """Split debug info out of every .exe / .dll in stage_dir before
    packaging:

      1. `objcopy --only-keep-debug <bin> <bin>.debug` → captures
         every DWARF section in a sibling file located OUTSIDE the
         stage tree (under `<stage_dir>-debug/<relpath>.debug`), so
         the installer never sees it.
      2. `strip --strip-unneeded -x <bin>` → removes the same DWARF
         + local symbols from <bin>. `--strip-unneeded` keeps the
         export table intact (other DLLs that import from
         Qt6Core.dll keep resolving at load time); `-x` drops
         locals.
      3. `objcopy --add-gnu-debuglink=<bin>.debug <bin>` → embeds a
         pointer + CRC into the now-stripped <bin> so gdb finds the
         sidecar when looking up symbols.

    Called AFTER the wine smoke test (which still has full symbols
    in the un-staged build dir for crash diagnostics) and AFTER
    `_strip_test_only_plugins`, but BEFORE NSIS / WiX / zip
    packaging. The sibling `<stage_dir>-debug/` tree is left on
    disk so operators can collect it for post-mortem analysis;
    test/all.sh's tmpfs sweep wipes it after the run."""
    if not os.path.isfile(_MXE_STRIP):
        log_info(f"strip skipped: {_MXE_STRIP} not found")
        return
    has_objcopy = os.path.isfile(_MXE_OBJCOPY)
    debug_root  = stage_dir.rstrip(os.sep) + "-debug"
    if has_objcopy:
        if os.path.isdir(debug_root):
            shutil.rmtree(debug_root, ignore_errors=True)
        os.makedirs(debug_root, exist_ok=True)
    targets = []
    for root, _, files in os.walk(stage_dir):
        for name in files:
            low = name.lower()
            if low.endswith(".exe") or low.endswith(".dll"):
                targets.append(os.path.join(root, name))
    if not targets:
        return
    stripped = 0
    split = 0
    for path in targets:
        try:
            before = os.path.getsize(path)
            if has_objcopy:
                rel = os.path.relpath(path, stage_dir)
                dbg = os.path.join(debug_root, rel + ".debug")
                os.makedirs(os.path.dirname(dbg), exist_ok=True)
                rc = subprocess.run(
                    [_MXE_OBJCOPY, "--only-keep-debug", path, dbg],
                    check=False, capture_output=True, timeout=60).returncode
                if rc == 0 and os.path.isfile(dbg):
                    split += 1
            subprocess.run([_MXE_STRIP, "--strip-unneeded", "-x", path],
                           check=False, capture_output=True, timeout=60)
            if has_objcopy and os.path.isfile(dbg):
                # debug-link references the basename, so gdb/objdump
                # look for the sidecar next to the binary OR under a
                # `/.debug/` subdir OR under `--debug-file-directory`.
                # The installer ships only <bin>; the sidecar stays in
                # <stage_dir>-debug/.
                subprocess.run(
                    [_MXE_OBJCOPY,
                     "--add-gnu-debuglink=" + dbg, path],
                    check=False, capture_output=True, timeout=60)
            after = os.path.getsize(path)
            if after < before:
                stripped += 1
        except (OSError, subprocess.SubprocessError):
            pass
    log_info(f"stripped debug symbols from {stripped}/{len(targets)} binaries in stage; "
             f"sidecars in {os.path.relpath(debug_root, ROOT)} "
             f"({split} .debug files)" if has_objcopy
             else f"stripped debug symbols from {stripped}/{len(targets)} binaries in stage")


def _verify_png_support(stage_dir):
    """Confirm the staged payload still has PNG decode/encode support.

    The CatchChallenger UI loads PNG art (icons, world tiles, splash
    screens) at startup — without a working PNG path the client errors
    out before MapVisualiserPlayer::mapDisplayedSlot() ever fires. PNG
    arrives via two routes in Qt6 + MXE:

      1. libpng16-16.dll      → backing library used by both Qt6Gui's
                                built-in PNG handler AND any explicit
                                qpng image plugin.
      2. imageformats/        → directory of optional QImageIOPlugin
                                .dlls (qjpeg, qico, qsvg, …). Qt6 ships
                                PNG built into Qt6Gui, but the
                                imageformats dir must still exist for
                                non-PNG formats the splash/menu use.

    Verify both. Returns True on success; on failure logs the missing
    component so the operator knows what to copy in."""
    missing = []
    libpng = os.path.join(stage_dir, "libpng16-16.dll")
    if not os.path.isfile(libpng):
        missing.append("libpng16-16.dll (top-level)")
    imgfmt = os.path.join(stage_dir, "imageformats")
    if not os.path.isdir(imgfmt):
        missing.append("imageformats/ directory")
    else:
        try:
            plugs = [p for p in os.listdir(imgfmt) if p.lower().endswith(".dll")]
        except OSError:
            plugs = []
        if not plugs:
            missing.append("imageformats/*.dll (windeployqt produced an empty dir)")
    if missing:
        log_info("png support check FAILED: missing " + ", ".join(missing))
        return False
    return True


def _strip_test_only_plugins(stage_dir):
    """Delete Qt platform plugins that are only useful for the wine
    smoke test (offscreen, minimal). Real end-user Windows machines
    always use the native windows platform plugin; shipping
    qoffscreen.dll / qminimal.dll just inflates the installer.

    Called AFTER `_verify_staged_exe_runs` has validated the staged
    payload with `-platform offscreen` and BEFORE the NSIS / WiX /
    zip packaging step. Removes both lowercase and titlecase variants
    just in case windeployqt's naming changes between Qt versions."""
    plugins_dir = os.path.join(stage_dir, "platforms")
    if not os.path.isdir(plugins_dir):
        return
    removed = []
    for entry in os.listdir(plugins_dir):
        low = entry.lower()
        if low.startswith("qoffscreen") or low.startswith("qminimal"):
            try:
                os.remove(os.path.join(plugins_dir, entry))
                removed.append(entry)
            except OSError:
                pass
    if removed:
        log_info(f"strip test-only Qt plugins: {', '.join(removed)}")


def _verify_staged_exe_runs(stage_dir):
    """Run the staged .exe under wine64 with QT_QPA_PLATFORM=offscreen
    to confirm the trimmed DLL set is still enough to start the
    process. Returns True when wine actually loaded the binary — either
    the .exe exited cleanly OR our 3-second wall-clock cap fired while
    it was still running (which means it got past every DLL-load
    LoadLibrary call; that's all we need this smoke test to prove).
    Hard fail only when wine itself reports a non-timeout error (e.g.
    missing DLL → 0xC0000135 → wine prints an err: and exits non-zero
    immediately). Strips DISPLAY/XAUTHORITY/WAYLAND_DISPLAY and forces
    `-platform offscreen` on the CLI so wine cannot pop a window on the
    host X server during the staged-payload smoke test."""
    if not WINE_BIN or not os.path.isfile(WINE_BIN):
        return True
    exe = os.path.join(stage_dir, WIN_EXE_NAME)
    if not os.path.isfile(exe):
        return False
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"]  = "offscreen"
    env["WINEDEBUG"]        = "-all"
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
    for _gui_var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(_gui_var, None)
    rc, _ = run_cmd([WINE_BIN, exe, "-platform", "offscreen", "--version"],
                    stage_dir, timeout=3, env=env)
    # rc == 0  → clean --version exit
    # rc == -1 → wall-clock timeout (run_cmd's TIMEOUT sentinel). The
    #            .exe is alive at the 3 s cap; LoadLibrary calls all
    #            succeeded or wine would have crashed earlier.
    return rc == 0 or rc == -1


def _stage_combined_payload(parts):
    """Merge the per-binary stage dirs (already built + filtered by
    _stage_install_payload) into a single COMBINED_STAGE_DIR, with
    each .exe renamed to its target shipped name and every other
    file deduped by basename. Returns COMBINED_STAGE_DIR on success.

    parts is a list of dicts:
        [{"stage": <path>, "exe_src": <basename>, "exe_dst": <basename>}]

    The first part is the "primary" — its shortcut becomes the
    default in the Start Menu. All three .exe files end up next to
    each other in the install dir, sharing one copy of every Qt6
    DLL / plugin / datapack tree (the three stages produce the same
    Qt6 layout because they're built against the same MXE Qt)."""
    dst = COMBINED_STAGE_DIR
    if os.path.isdir(dst):
        shutil.rmtree(dst, ignore_errors=True)
    os.makedirs(dst, exist_ok=True)
    # Copy each part's tree onto the same dst — first one wins for
    # any duplicated DLL/plugin path; subsequent parts overwrite
    # only if missing (shutil.copy2 unconditionally overwrites, so
    # we walk + skip-if-exists for non-exe entries).
    seen = set()
    pi = 0
    while pi < len(parts):
        p = parts[pi]
        pi += 1
        src = p["stage"]
        if not os.path.isdir(src):
            continue
        for root, dirs, files in os.walk(src):
            rel = os.path.relpath(root, src)
            target_dir = dst if rel in (".", "") else os.path.join(dst, rel)
            os.makedirs(target_dir, exist_ok=True)
            fi = 0
            while fi < len(files):
                n = files[fi]
                fi += 1
                # Rename the part's primary .exe to its shipping
                # filename (e.g. qtcpu800x600's catchchallenger.exe
                # → catchchallenger800x600.exe).
                if rel in (".", "") and n == p["exe_src"]:
                    out_name = p["exe_dst"]
                else:
                    out_name = n
                target = os.path.join(target_dir, out_name)
                key = os.path.relpath(target, dst)
                if key in seen:
                    continue
                seen.add(key)
                try:
                    shutil.copy2(os.path.join(root, n), target,
                                 follow_symlinks=True)
                except OSError:
                    pass
    # Drop a minimal qt.conf at the install root so Qt6 resolves
    # plugin paths relative to applicationDirPath() instead of the
    # MXE compile-time path (which on a user's machine doesn't
    # exist, so imageformats/qjpeg.dll never registers and JPEG
    # decoding via QPixmap silently fails — "Could not create
    # pixmap from :/images/background.jpg"). windeployqt doesn't
    # always emit qt.conf for cross builds; write it ourselves so
    # the shipped installer never depends on a host-only path.
    try:
        with open(os.path.join(dst, "qt.conf"), "w") as f:
            f.write("[Paths]\nPlugins = .\n")
    except OSError:
        pass
    return dst


def _stage_combined_part(exe, pro_file, build_dir, label, exe_name, exe_dst):
    """Stage one binary for the combined installer, rebuilding it once if
    its build dir was swept.

    The Windows build dirs live in tmpfs and are registered for
    cleanup-on-exit; a concurrent testing run (or all.sh) finishing can
    rmtree the shared dir between this script's compile and packaging
    phases, which previously surfaced as an unhandled FileNotFoundError
    that aborted the whole script. Rebuild on demand (ccache makes it
    cheap) and return None only if it still can't be produced — the caller
    then skips the combined installer with a clean FAIL instead of
    crashing."""
    src = os.path.dirname(exe) if exe else build_dir
    if not (src and os.path.isdir(src)
            and os.path.isfile(os.path.join(src, exe_name))):
        log_info(f"combined stage: {label} build dir/exe missing — rebuilding")
        rebuilt = build_mxe_client(pro_file, build_dir, label)
        if rebuilt is None:
            return None
        src = os.path.dirname(rebuilt)
        exe_name = os.path.basename(rebuilt)
    stage = _stage_install_payload(src, exe_name)
    if stage is None:
        return None
    return {"stage": stage, "exe_src": exe_name, "exe_dst": exe_dst}


def build_combined_installer(parts):
    """Single NSIS installer covering qtcpu800x600 + qtopengl +
    server-gui. Output: <tmpfs_root>/catchchallenger-installer.exe.

    Three Start-Menu shortcuts go into a "CatchChallenger" submenu:
      - CatchChallenger             → catchchallenger.exe (qtopengl)
      - CatchChallenger 800x600     → catchchallenger800x600.exe
      - CatchChallenger Server GUI  → catchchallenger-server-gui.exe
    The desktop shortcut points at the primary (qtopengl) only —
    pinning every variant to the desktop would pollute it."""
    name = "installer combined"
    stage = _stage_combined_payload(parts)
    if not _verify_png_support(stage):
        log_fail(name, f"combined payload lacks PNG support — "
                       f"Qt6Gui/libpng or imageformats/ missing in {stage}")
        return None
    _strip_test_only_plugins(stage)
    _strip_debug_symbols(stage)

    if MAKENSIS_BIN is None:
        log_fail(name, f"makensis not found at {MAKENSIS_BIN}; "
                       f"NSIS installer step skipped")
        return None
    out_dir = COMBINED_NSIS_DIR
    os.makedirs(out_dir, exist_ok=True)
    nsi_path = os.path.join(out_dir, "catchchallenger.nsi")
    installer_exe = os.path.join(out_dir, "catchchallenger-installer.exe")
    nsi = (
        'OutFile "' + installer_exe + '"\n'
        'InstallDir "$PROGRAMFILES64\\CatchChallenger"\n'
        'RequestExecutionLevel admin\n'
        'Section "MainSection" SEC01\n'
        '  SetOutPath "$INSTDIR"\n'
        '  File /r "' + stage + os.sep + '*.*"\n'
        '  CreateDirectory "$SMPROGRAMS\\CatchChallenger"\n'
        '  CreateShortCut "$SMPROGRAMS\\CatchChallenger\\CatchChallenger.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_GL + '"\n'
        '  CreateShortCut "$SMPROGRAMS\\CatchChallenger\\CatchChallenger CPU 800x600.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_CPU + '"\n'
        '  CreateShortCut "$SMPROGRAMS\\CatchChallenger\\CatchChallenger Server.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_SRV + '"\n'
        '  CreateShortCut "$DESKTOP\\CatchChallenger.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_GL + '"\n'
        'SectionEnd\n'
    )
    with open(nsi_path, "w") as f:
        f.write(nsi)
    log_info(f"makensis {os.path.basename(nsi_path)} (combined)")
    rc, out = run_cmd([MAKENSIS_BIN, "-V2", nsi_path],
                      out_dir, timeout=COMPILE_TIMEOUT)
    if rc == 0 and os.path.isfile(installer_exe):
        log_pass(name, f"-> {os.path.relpath(installer_exe, ROOT)}")
        _verify_installer_size("combined", installer_exe)
        return installer_exe
    log_fail(name, f"makensis failed (rc={rc})")
    if out.strip():
        print(out[-2000:])
    return None


def build_combined_installer_user(parts):
    """User-level NSIS installer for the real-Windows test node (non-admin).

    Identical payload to build_combined_installer() but installs to
    $LOCALAPPDATA\\CatchChallenger with RequestExecutionLevel user so it
    runs without UAC elevation. Used only by run_on_real_windows()."""
    name = "installer combined (user)"
    stage = _stage_combined_payload(parts)
    if not _verify_png_support(stage):
        log_fail(name, f"combined payload lacks PNG support")
        return None
    _strip_test_only_plugins(stage)
    _strip_debug_symbols(stage)
    if MAKENSIS_BIN is None:
        log_fail(name, "makensis not found; user installer skipped")
        return None
    out_dir = COMBINED_NSIS_DIR
    os.makedirs(out_dir, exist_ok=True)
    nsi_path      = os.path.join(out_dir, "catchchallenger-user.nsi")
    installer_exe = os.path.join(out_dir, "catchchallenger-installer-user.exe")
    nsi = (
        'OutFile "' + installer_exe + '"\n'
        'InstallDir "$LOCALAPPDATA\\CatchChallenger"\n'
        'RequestExecutionLevel user\n'
        'Section "MainSection" SEC01\n'
        '  SetOutPath "$INSTDIR"\n'
        '  File /r "' + stage + os.sep + '*.*"\n'
        '  CreateDirectory "$SMPROGRAMS\\CatchChallenger"\n'
        '  CreateShortCut "$SMPROGRAMS\\CatchChallenger\\CatchChallenger.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_GL + '"\n'
        '  CreateShortCut "$SMPROGRAMS\\CatchChallenger\\CatchChallenger CPU 800x600.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_CPU + '"\n'
        '  CreateShortCut "$SMPROGRAMS\\CatchChallenger\\CatchChallenger Server.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_SRV + '"\n'
        '  CreateShortCut "$DESKTOP\\CatchChallenger.lnk" '
        '"$INSTDIR\\' + COMBINED_BIN_GL + '"\n'
        'SectionEnd\n'
    )
    with open(nsi_path, "w") as f:
        f.write(nsi)
    log_info(f"makensis {os.path.basename(nsi_path)} (combined user-level)")
    rc, out = run_cmd([MAKENSIS_BIN, "-V2", nsi_path],
                      out_dir, timeout=COMPILE_TIMEOUT)
    if rc == 0 and os.path.isfile(installer_exe):
        log_pass(name, f"-> {os.path.relpath(installer_exe, ROOT)}")
        return installer_exe
    log_fail(name, f"makensis failed (rc={rc})")
    if out.strip():
        print(out[-2000:])
    return None


def build_combined_msi(parts):
    """Single .msi covering qtcpu800x600 + qtopengl + server-gui.
    Output: <tmpfs_root>/catchchallenger.msi.

    Same 3-shortcut Start Menu layout as build_combined_installer."""
    name = "msi combined"
    stage = _stage_combined_payload(parts)
    if not _verify_png_support(stage):
        log_fail(name, f"combined payload lacks PNG support in {stage}")
        return None
    _strip_test_only_plugins(stage)
    _strip_debug_symbols(stage)

    if not (os.path.isfile(WIX_CANDLE_EXE) and os.path.isfile(WIX_LIGHT_EXE)
            and os.path.isfile(WIX_HEAT_EXE)):
        log_fail(name, f"WiX not found under {WIX_DIR}; skipped")
        return None
    work = COMBINED_MSI_DIR
    if os.path.isdir(work):
        shutil.rmtree(work)
    os.makedirs(work)
    msi_path = os.path.join(work, "catchchallenger.msi")
    main_wxs  = os.path.join(work, "main.wxs")
    files_wxs = os.path.join(work, "files.wxs")
    _write_combined_main_wxs(main_wxs, os.path.basename(stage))
    env = _wix_wine_env()

    # heat.exe — same incantation the per-binary build_msi uses (see
    # the wine64-relpath comment there for why a RELATIVE input path
    # matters): emit `$(var.SourceDir)/...` references so candle
    # later resolves them against -dSourceDir=, instead of hard-
    # coding the absolute stage path into files.wxs. Without
    # `-var var.SourceDir`, light.exe fails LGHT0103 trying to read
    # files at a build-host path that doesn't exist on the wine
    # filesystem mapping.
    files_wxs_rel = os.path.relpath(files_wxs, work)
    stage_rel     = os.path.relpath(stage, work)
    log_info(f"wine64 heat.exe dir {os.path.basename(stage)} (combined)")
    rc, out = run_cmd([WINE_BIN, WIX_HEAT_EXE,
                       "dir", stage_rel,
                       "-nologo", "-gg", "-srd", "-sreg",
                       "-scom", "-ke", "-sfrag", "-suid",
                       "-cg", "AppPayloadGroup",
                       "-dr", "INSTALLFOLDER",
                       "-var", "var.SourceDir",
                       "-out", files_wxs_rel],
                      work, timeout=COMPILE_TIMEOUT, env=env)
    if rc != 0:
        log_fail(name, f"heat failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None

    log_info("wine64 candle.exe (combined)")
    rc, out = run_cmd([WINE_BIN, WIX_CANDLE_EXE,
                       "-nologo",
                       "-dSourceDir=" + os.path.relpath(stage, work),
                       os.path.relpath(main_wxs, work),
                       os.path.relpath(files_wxs, work)],
                      work, timeout=COMPILE_TIMEOUT, env=env)
    if rc != 0:
        log_fail(name, f"candle failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None

    log_info(f"wine64 light.exe -> {os.path.basename(msi_path)}")
    rc, out = run_cmd([WINE_BIN, WIX_LIGHT_EXE,
                       "-nologo", "-sval", "-spdb",
                       "-sice:ICE61", "-sice:ICE69",
                       "-out", os.path.relpath(msi_path, work),
                       "main.wixobj", "files.wixobj"],
                      work, timeout=COMPILE_TIMEOUT, env=env)
    if rc != 0 or not os.path.isfile(msi_path):
        log_fail(name, f"light failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_pass(name, f"-> {os.path.relpath(msi_path, ROOT)}")
    _verify_msi_size("combined", msi_path)
    sign_authenticode(msi_path, "combined", "msi")
    import cleanup_helpers as _ch
    _ch.promote_artifact(msi_path, "catchchallenger.msi")
    return msi_path


def _write_combined_main_wxs(out_path, source_dir_basename):
    """Same per-product header as _write_main_wxs but with the 3
    Start-Menu shortcuts the combined installer needs. heat.exe's
    ComponentGroup output (files.wxs) supplies AppPayloadGroup
    referenced from <Feature> in the main file."""
    upgrade_code = "12345678-1111-2222-3333-444455556666"
    wxs = (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">\n'
        '  <Product Id="*" Name="CatchChallenger" '
        'Language="1033" Version="4.0.0.0" '
        'Manufacturer="CatchChallenger" '
        'UpgradeCode="' + upgrade_code + '">\n'
        '    <Package InstallerVersion="200" Compressed="yes" '
        'InstallScope="perMachine"/>\n'
        '    <Media Id="1" Cabinet="cc.cab" EmbedCab="yes"/>\n'
        '    <Directory Id="TARGETDIR" Name="SourceDir">\n'
        '      <Directory Id="ProgramFiles64Folder">\n'
        '        <Directory Id="INSTALLFOLDER" Name="CatchChallenger"/>\n'
        '      </Directory>\n'
        '      <Directory Id="ProgramMenuFolder">\n'
        '        <Directory Id="ApplicationProgramsFolder" Name="CatchChallenger"/>\n'
        '      </Directory>\n'
        '    </Directory>\n'
        '    <DirectoryRef Id="ApplicationProgramsFolder">\n'
        '      <Component Id="ApplicationShortcuts" Guid="*">\n'
        '        <Shortcut Id="GLShortcut" Name="CatchChallenger" '
        'Target="[INSTALLFOLDER]' + COMBINED_BIN_GL + '" '
        'WorkingDirectory="INSTALLFOLDER"/>\n'
        '        <Shortcut Id="CPUShortcut" Name="CatchChallenger CPU 800x600" '
        'Target="[INSTALLFOLDER]' + COMBINED_BIN_CPU + '" '
        'WorkingDirectory="INSTALLFOLDER"/>\n'
        '        <Shortcut Id="SRVShortcut" Name="CatchChallenger Server" '
        'Target="[INSTALLFOLDER]' + COMBINED_BIN_SRV + '" '
        'WorkingDirectory="INSTALLFOLDER"/>\n'
        '        <RemoveFolder Id="CleanUpShortcutFolder" '
        'Directory="ApplicationProgramsFolder" On="uninstall"/>\n'
        '        <RegistryValue Root="HKCU" '
        'Key="Software\\CatchChallenger" Name="installed" Type="integer" '
        'Value="1" KeyPath="yes"/>\n'
        '      </Component>\n'
        '    </DirectoryRef>\n'
        '    <Feature Id="ProductFeature" Title="CatchChallenger" Level="1">\n'
        '      <ComponentGroupRef Id="AppPayloadGroup"/>\n'
        '      <ComponentRef Id="ApplicationShortcuts"/>\n'
        '    </Feature>\n'
        '  </Product>\n'
        '</Wix>\n'
    )
    with open(out_path, "w") as f:
        f.write(wxs)


def build_installer(exe_path, label):
    """Package the deployed exe + Qt DLLs + datapack into a single
    installer artefact next to the build dir.  Prefers NSIS (real
    Windows installer .exe); falls back to a portable .zip when
    makensis isn't available on the host.

    Pre-condition: deploy_mxe_dependencies() and setup_datapack_client()
    have already populated <exe_dir> with everything the installer
    should ship — Qt6*.dll, plugin/ subdirs, runtime DLLs, and
    datapack/internal/.

    The packaging step stages the exe_dir into a sibling "-install-stage"
    directory first, dropping CMakeFiles/*_autogen/.qt/ninja-metadata and
    any .dll that's not on the runtime allowlist. NSIS then picks up the
    cleaned tree, so the produced installer stays under the 100 MiB
    shipping-artefact ceiling."""
    name = f"installer {label}"
    exe_dir = os.path.dirname(exe_path)
    base    = "catchchallenger-" + label
    out_dir = os.path.dirname(exe_dir.rstrip(os.sep)) or exe_dir

    stage_dir = _stage_install_payload(exe_dir)
    if not _verify_staged_exe_runs(stage_dir):
        log_fail(name, f"staged .exe failed wine smoke test in {stage_dir}; "
                       f"missing a runtime .dll — adjust "
                       f"_INSTALL_ALLOWLIST_DLLS")
        return None
    if not _verify_png_support(stage_dir):
        log_fail(name, f"staged payload lacks PNG support — Qt6Gui/libpng "
                       f"or imageformats/ missing in {stage_dir}")
        return None
    # Smoke test passed — purge the offscreen/minimal Qt platform
    # plugins so the installer that ships to end users doesn't carry
    # them. Real Windows targets use the windows platform plugin.
    _strip_test_only_plugins(stage_dir)
    # The build is Debug (kept that way to speed test cycles and to
    # give the wine smoke test useful symbols on crash); strip debug
    # symbols off every .exe/.dll just before packaging to fit under
    # the 100 MiB shipping-artifact ceiling.
    _strip_debug_symbols(stage_dir)

    if MAKENSIS_BIN is not None:
        nsi_path = os.path.join(out_dir, base + ".nsi")
        installer_exe = os.path.join(out_dir, base + "-installer.exe")
        nsi = (
            'OutFile "' + installer_exe + '"\n'
            'InstallDir "$PROGRAMFILES64\\CatchChallenger"\n'
            'RequestExecutionLevel admin\n'
            'Section "MainSection" SEC01\n'
            '  SetOutPath "$INSTDIR"\n'
            '  File /r "' + stage_dir + os.sep + '*.*"\n'
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
            _verify_installer_size(label, installer_exe)
            # Promote to tmpfs root so it survives build-dir teardown.
            import cleanup_helpers as _ch
            _ch.promote_artifact(installer_exe,
                                 f"catchchallenger-{label}-installer.exe")
            return installer_exe
        log_fail(name, f"makensis failed (rc={rc}); falling back to zip")
        if out.strip():
            print(out[-2000:])

    zip_base = os.path.join(out_dir, base + "-installer")
    log_info(f"zip {os.path.basename(zip_base)}.zip")
    try:
        archive = shutil.make_archive(zip_base, "zip",
                                      root_dir=os.path.dirname(stage_dir),
                                      base_dir=os.path.basename(stage_dir))
    except (OSError, shutil.Error) as e:
        log_fail(name, f"zip failed: {e}")
        return None
    log_pass(name, f"-> {os.path.relpath(archive, ROOT)}")
    _verify_installer_size(label, archive)
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
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
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
    # Harvest the cleaned install staging dir (no CMakeFiles, no
    # *_autogen, no build-system metadata, no non-runtime DLL) — the
    # same payload the NSIS installer ships. Without this the .msi
    # would balloon past 100 MiB with cmake_install.cmake, .ninja_*,
    # CMakeCache.txt, compile_commands.json, autogen MOC trees, ffmpeg
    # / hdf5 / GLEW / glfw DLLs from the MXE bin/ dir, etc. (see
    # _stage_install_payload doc).
    payload_dir = _stage_install_payload(exe_dir)
    if not _verify_staged_exe_runs(payload_dir):
        log_fail(name, f"staged .exe failed wine smoke test in "
                       f"{payload_dir}; missing a runtime .dll — adjust "
                       f"_INSTALL_ALLOWLIST_DLLS")
        return None
    if not _verify_png_support(payload_dir):
        log_fail(name, f"staged payload lacks PNG support — Qt6Gui/libpng "
                       f"or imageformats/ missing in {payload_dir}")
        return None
    # Smoke test passed — purge the offscreen/minimal Qt platform
    # plugins so the .msi we ship doesn't carry them. Same reason as
    # build_installer.
    _strip_test_only_plugins(payload_dir)
    _strip_debug_symbols(payload_dir)
    _write_main_wxs(main_wxs, label, os.path.basename(payload_dir))
    env = _wix_wine_env()

    # 1) heat.exe — harvest payload_dir into a ComponentGroup fragment.
    # heat.exe runs as a Windows binary under wine; its CLI parser treats
    # `/` as a switch indicator, so `-out /mnt/...` looks like `-out`
    # followed by the switch `/mnt`, and heat reports HEAT0114 ("the
    # parameter '-out' must be followed by a file path"). The fix: pass
    # the output as a path RELATIVE to cwd (heat resolves it from there)
    # — there's no leading `/` for the parser to mis-interpret. Same
    # applies to the input dir; convert to a relative path against work.
    files_wxs_rel = os.path.relpath(files_wxs, work)
    payload_dir_rel = os.path.relpath(payload_dir, work)
    log_info(f"wine64 heat.exe dir {os.path.basename(payload_dir)} ({label})")
    rc, out = run_cmd(
        [WINE_BIN, WIX_HEAT_EXE, "dir", payload_dir_rel,
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
    payload_dir_rel = os.path.relpath(payload_dir, work)
    log_info(f"wine64 candle.exe ({label})")
    rc, out = run_cmd(
        [WINE_BIN, WIX_CANDLE_EXE, "-nologo",
         "-arch", "x64",
         "-dSourceDir=" + payload_dir_rel,
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
    _verify_msi_size(label, msi_path)

    # 4) sign the .msi (osslsigncode handles MSI just like PE).
    sign_authenticode(msi_path, label, "msi")
    # Promote to tmpfs root so the .msi survives build-dir teardown.
    import cleanup_helpers as _ch
    _ch.promote_artifact(msi_path, f"catchchallenger-{label}.msi")
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
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
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
    # Force the offscreen platform on the Qt CLI too, not just via
    # QT_QPA_PLATFORM: that env var does NOT reliably propagate through wine
    # into the Windows process, so an env-only run still pops a real window on
    # the host desktop. Qt parses+strips "-platform offscreen" before the
    # app's own args (--host/--autologin/...), matching the version/screenshot
    # launches that already pass it.
    win_args = [WINE_BIN, exe_path, "-platform", "offscreen"] + list(args)
    # Wine inherits Linux CWD as the Windows-side CWD (mapped via Z:\).
    # The in-process server (--autosolo) loads the datapack from a
    # *relative* path "./datapack/"; setup_datapack_client() already
    # staged that tree under os.path.dirname(exe_path). Without cwd
    # pinned to exe's dir the server resolves "./datapack/" against
    # whatever the python script's CWD was (test/), the path is
    # missing, and the .exe exits with rc=1 emitting "Datapack
    # directory missing: /datapack/".
    cwd = os.path.dirname(exe_path)
    diagnostic.record_cmd(win_args, cwd)
    proc = subprocess.Popen(
        win_args,
        cwd=cwd,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=force_headless_env(env),
        preexec_fn=process_helpers.setsid_and_pdeathsig)
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


_DLL_FAIL_PATTERNS = (
    "could not load the qt platform plugin",
    "qt: could not initialize",
    "this application failed to start",
    "the procedure entry point",
)
# Wine emits `warn:module:load_dll Failed to load module L"winemac.drv"`
# (and winex11.drv, winewayland.drv) on every Linux wine startup
# while probing the available display drivers — that is normal, not
# a failure, so the smoke check only counts `err:module:` /
# `err:dll:` traces, which are the channels wine reserves for the
# actually-broken load paths.
_DLL_FAIL_TRACE_TAGS = ("err:module:", "err:dll:")


def run_wine_server_smoke(exe_path, label, wait_seconds=5):
    """Launch a staged server-gui.exe under wine64 for ~5 s with
    `WINEDEBUG=+loaddll,+module` so wine emits every LoadLibraryEx
    attempt + every dependency-resolution failure on stderr. The
    harness scans the captured output for the standard "missing DLL"
    patterns (see _DLL_FAIL_PATTERNS) and the server's own
    "correctly bind:" success marker.

    PASS iff
      - the process is still alive when `wait_seconds` elapses AND
        `correctly bind:` was emitted; OR
      - the process exits with rc=0 within `wait_seconds` (rare for
        a daemon, but possible if a CLI subcommand short-circuits).

    FAIL otherwise (timeout without bind, non-zero rc, missing-DLL
    pattern in output). The full last 60 lines are echoed on FAIL so
    operators see the offending wine trace without having to re-run
    locally."""
    name = f"wine server smoke {label}"
    log_info(f"wine64 {os.path.basename(exe_path)} (smoke, {wait_seconds}s)")
    env = os.environ.copy()
    env["WINEPATH"]         = MXE_QT_BIN + ";" + MXE_RT_BIN
    # +loaddll : every LoadLibraryEx call, success or failure.
    # +module  : module resolution + dep walking — catches the case
    #            where a transitive dep is missing.
    # Keep -all on the other channels so the trace stays readable.
    env["WINEDEBUG"]        = "+loaddll,+module,-all"
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
    env["QT_QPA_PLATFORM"]  = "offscreen"
    for _gui_var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(_gui_var, None)
    cwd = os.path.dirname(exe_path)
    # CLI offscreen too (QT_QPA_PLATFORM doesn't reliably cross wine) so the
    # server-gui smoke never pops a host window.
    win_args = [WINE_BIN, exe_path, "-platform", "offscreen"]
    diagnostic.record_cmd(win_args, cwd)
    proc = subprocess.Popen(
        win_args, cwd=cwd, stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=force_headless_env(env),
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    output_lines = []
    bound = threading.Event()

    def reader():
        while True:
            raw = proc.stdout.readline()
            if not raw:
                break
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if "correctly bind:" in line:
                bound.set()

    threading.Thread(target=reader, daemon=True).start()
    elapsed = 0.0
    step = 0.25
    while elapsed < wait_seconds:
        if proc.poll() is not None:
            break
        time.sleep(step)
        elapsed += step

    rc = proc.poll()
    if rc is None:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            proc.wait(timeout=3)

    joined = "\n".join(output_lines)
    low = joined.lower()
    dll_hits = []
    for pat in _DLL_FAIL_PATTERNS:
        if pat in low:
            dll_hits.append(pat)
    for tag in _DLL_FAIL_TRACE_TAGS:
        if tag in low:
            dll_hits.append(tag)

    ok = False
    detail = ""
    if dll_hits:
        detail = (f"DLL-load failure pattern(s) hit: "
                  f"{', '.join(dll_hits)}")
    elif bound.is_set():
        ok = True
        detail = f"bound within {elapsed:.1f}s"
    elif rc is None:
        # server-gui is a Qt Widgets application; it does NOT auto-
        # bind on startup (the operator presses "Start" in the UI to
        # begin listening). Reaching the 5s wait WITHOUT crashing is
        # therefore the success condition for the smoke check — it
        # means every DLL + Qt plugin + datapack loaded cleanly and
        # the main window message loop is running.
        ok = True
        detail = f"alive after {wait_seconds:.1f}s, no DLL-load errors"
    elif rc == 0:
        ok = True
        detail = f"exited rc=0 within {elapsed:.1f}s"
    else:
        detail = f"exited rc={rc} within {elapsed:.1f}s"
        if process_helpers.is_sigabrt(rc):
            if process_helpers.looks_like_protocol_parse_failure(joined):
                detail += " (looks like protocol-parse abort)"
            else:
                detail += " (SIGABRT)"

    if ok:
        log_pass(name, detail)
    else:
        log_fail(name, detail)
        li = max(0, len(output_lines) - 60)
        while li < len(output_lines):
            print(f"  | {output_lines[li]}")
            li += 1
    return ok


# ── installed-payload end-to-end (real NSIS install under wine) ─────────────
# The wine smoke / screenshot tests above all run the *staged* build
# dir directly. They never exercise what the end user actually gets:
# the NSIS installer writing into C:\Program Files\CatchChallenger and
# the .exe then loading Qt plugins (platforms, sqldrivers, …) from that
# laid-down tree. The server-gui "Start" path — which opens the bundled
# SQLite driver plugin — is completely unexercised by
# run_wine_server_smoke (server-gui does NOT auto-bind; the operator
# presses Start), so a missing/broken sqldrivers plugin ships green and
# the operator gets a server that binds then silently fails with
# "Driver not loaded". These five steps close that gap.

E2E_PORT = "61918"   # distinct from SERVER_PORT (linux multi server)

# The 5 case names run_installed_payload_e2e emits, in order. Kept as a
# module constant so main()'s should_run() gating + the missing-binary
# FAIL fan-out + _needs_compile() stay in lockstep with the function.
_E2E_CASE_NAMES = (
    "wine uninstall pre-clean",
    "wine install combined installer",
    "windows installed exe icons distinct",
    "wine installed server-gui start (no crash)",
    "wine installed qtcpu800x600 to map",
)


def _wine_prefix():
    """Resolve the wine prefix the same way wine64 itself does:
    $WINEPREFIX when set, else ~/.wine. The harness never sets
    WINEPREFIX, so this is ~/.wine in practice."""
    return os.environ.get("WINEPREFIX") or os.path.expanduser("~/.wine")


def _wine_install_dirs():
    """Candidate on-disk paths for the NSIS InstallDir
    ($PROGRAMFILES64\\CatchChallenger). A win64 prefix exposes it as
    "Program Files"; list the (x86) sibling too so a 32-bit prefix or
    an installer that fell back to PROGRAMFILES still resolves."""
    dc = os.path.join(_wine_prefix(), "drive_c")
    return [os.path.join(dc, "Program Files", "CatchChallenger"),
            os.path.join(dc, "Program Files (x86)", "CatchChallenger")]


def _wine_env(extra_qt_path=True):
    """The standard headless-wine environment shared by the e2e steps
    (mirrors run_wine_client's setup): no debug channels, winedbg
    disabled so a crash returns a non-zero rc instead of popping the
    interactive debugger, offscreen QPA, and no host display."""
    env = os.environ.copy()
    if extra_qt_path:
        env["WINEPATH"] = MXE_QT_BIN + ";" + MXE_RT_BIN
    env["WINEDEBUG"]        = "-all"
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
    env["QT_QPA_PLATFORM"]  = "offscreen"
    for _gui_var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(_gui_var, None)
    return env


def _pe_icon_digest(path):
    """Return a sha256 hex digest of every RT_ICON (resource type 3)
    image blob embedded in the PE at `path`, or None when the file has
    no parseable icon resource. Pure-stdlib PE walk — no pefile /
    icoutils dependency (project policy: 0 extra deps). Two .exe with
    the same digest carry the *same* embedded icon."""
    import struct, hashlib
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError:
        return None
    if len(data) < 0x40 or data[:2] != b"MZ":
        return None
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
        return None
    coff = e_lfanew + 4
    num_sections = struct.unpack_from("<H", data, coff + 2)[0]
    opt_size = struct.unpack_from("<H", data, coff + 16)[0]
    sect_tbl = coff + 20 + opt_size
    rsrc = None
    si = 0
    while si < num_sections:
        base = sect_tbl + si * 40
        sname = data[base:base + 8].rstrip(b"\0")
        if sname == b".rsrc":
            rsrc = (struct.unpack_from("<I", data, base + 12)[0],   # VA
                    struct.unpack_from("<I", data, base + 20)[0])   # raw ptr
            break
        si += 1
    if rsrc is None:
        return None
    rsrc_va, rsrc_ptr = rsrc
    blobs = []

    def walk(dir_off, level, in_icon):
        # Resource directory: 16-byte header then 8-byte entries.
        hdr = rsrc_ptr + dir_off
        n_named = struct.unpack_from("<H", data, hdr + 12)[0]
        n_id    = struct.unpack_from("<H", data, hdr + 14)[0]
        ei = 0
        total = n_named + n_id
        while ei < total:
            ent = hdr + 16 + ei * 8
            ei += 1
            name_id = struct.unpack_from("<I", data, ent)[0]
            off     = struct.unpack_from("<I", data, ent + 4)[0]
            if level == 0:
                # Type level: keep only RT_ICON (id 3). The high bit is
                # the "name (not id)" flag — never set for RT_ICON.
                if (name_id & 0x80000000) or name_id != 3:
                    continue
                child_icon = True
            else:
                child_icon = in_icon
            if off & 0x80000000:
                walk(off & 0x7FFFFFFF, level + 1, child_icon)
            elif in_icon or child_icon:
                de = rsrc_ptr + off
                d_rva = struct.unpack_from("<I", data, de)[0]
                d_sz  = struct.unpack_from("<I", data, de + 4)[0]
                d_ptr = d_rva - rsrc_va + rsrc_ptr
                if 0 <= d_ptr <= len(data) - d_sz:
                    blobs.append(data[d_ptr:d_ptr + d_sz])

    try:
        walk(0, 0, False)
    except (struct.error, IndexError):
        return None
    if not blobs:
        return None
    blobs.sort()
    h = hashlib.sha256()
    bi = 0
    while bi < len(blobs):
        h.update(blobs[bi])
        bi += 1
    return h.hexdigest()


def _write_server_gui_properties(install_dir, maincode):
    """Pin server-gui's runtime config inside the installed tree so
    --autostart binds a known port and auto-creates accounts. Keys live
    at the XML root (game-server schema — NOT inside <general>; see
    server/CLAUDE.md). DB left unset → loadOneDbGroup defaults to
    SQLite (database.sqlite), schema auto-created from the embedded
    resource on first open."""
    xml_path = os.path.join(install_dir, "server-properties.xml")
    with open(xml_path, "w") as xf:
        xf.write(
            '<?xml version="1.0"?>\n'
            '<configuration>\n'
            f'    <server-port value="{E2E_PORT}"/>\n'
            '    <server-ip value=""/>\n'
            '    <httpDatapackMirror value=""/>\n'
            '    <max-players value="10"/>\n'
            '    <pvp value="true"/>\n'
            '    <automatic_account_creation value="true"/>\n'
            '    <content>\n'
            f'        <mainDatapackCode value="{maincode}"/>\n'
            '        <subDatapackCode value=""/>\n'
            '    </content>\n'
            '</configuration>\n'
        )


def _kill_proc(proc):
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


def _spawn_installed_server_gui(server_exe):
    """Launch the installed server-gui.exe under wine with --autostart
    (programmatically clicks Start, exercising the DB / sqldrivers
    plugin path). Returns (proc, out_lines, listen_evt, dberr_evt).
    Caller waits then terminates proc."""
    env = _wine_env()
    cwd = os.path.dirname(server_exe)
    # INSTALLED server-gui: its payload has qoffscreen.dll stripped by
    # _strip_test_only_plugins(), so "-platform offscreen" would fail (no
    # plugin) — keep env-only headless here, unlike the build-dir launches.
    win_args = [WINE_BIN, server_exe, "--autostart"]
    diagnostic.record_cmd(win_args, cwd)
    proc = subprocess.Popen(
        win_args, cwd=cwd, stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=force_headless_env(env),
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    out_lines = []
    listening = threading.Event()
    db_failed = threading.Event()
    # "connected to sqlite" fires mid-init (DB just opened) — the socket
    # is bound but the server is still synchronously loading the
    # datapack + maps (hundreds of log lines, several seconds) and
    # CANNOT serve a game client yet. preload_finish() prints "Waiting
    # connection on port N" only once it is truly ready. name4 (no-
    # crash smoke) is happy with the early signal; name5 (client must
    # reach the map) MUST wait for this one, else the client connects
    # into a server still blocked in its load and the sync stalls.
    serving = threading.Event()

    def reader():
        while True:
            raw = proc.stdout.readline()
            if not raw:
                break
            line = raw.decode(errors="replace").rstrip("\n")
            out_lines.append(line)
            low = line.lower()
            # "Connected to SQLite" is emitted by
            # BaseServer::initialize_the_database AFTER the sqldrivers
            # plugin opened the DB — that, not the earlier socket
            # "Listen", is the real "Start did not crash" signal.
            if "connected to sqlite" in low \
                    or "waiting connection on port" in low:
                listening.set()
            if "waiting connection on port" in low:
                serving.set()
            if "driver not loaded" in low \
                    or ("unable to connect to the database" in low):
                db_failed.set()

    threading.Thread(target=reader, daemon=True).start()
    return proc, out_lines, listening, db_failed, serving


def run_installed_payload_e2e(installer_exe, win_dp_src, win_mc):
    """Five sequential steps against the REAL installed payload:

      1. wine: remove a prior C:\\Program Files\\CatchChallenger
      2. wine: silent-install from the combined NSIS installer
      3. the 3 installed .exe carry distinct embedded icons
      4. installed server-gui.exe --autostart opens the DB without
         crashing (the sqldrivers-plugin regression the user hit)
      5. installed qtcpu800x600.exe reaches the map against (4)

    Each step emits its own PASS/FAIL row so the total test count is
    fixed and a regression in any one is isolated. Dependent steps
    FAIL (never silently skip) when a prerequisite failed."""

    # ── 1. uninstall / pre-clean ────────────────────────────────────
    name1 = "wine uninstall pre-clean"
    log_info("removing prior C:\\Program Files\\CatchChallenger")
    rm_err = None
    for d in _wine_install_dirs():
        if os.path.isdir(d):
            try:
                shutil.rmtree(d)
            except OSError as e:
                rm_err = f"{d}: {e}"
                break
    still = [d for d in _wine_install_dirs() if os.path.isdir(d)]
    if rm_err is not None:
        log_fail(name1, f"could not remove {rm_err}")
    elif still:
        log_fail(name1, f"still present after rmtree: {still}")
    else:
        log_pass(name1, "no stale install dir")

    # ── 2. install from the combined installer ──────────────────────
    name2 = "wine install combined installer"
    install_dir = None
    if installer_exe is None or not os.path.isfile(installer_exe):
        log_fail(name2, "combined installer .exe not built")
    else:
        env = _wine_env(extra_qt_path=False)
        # NSIS /S = silent (sections run, no UI). Default InstallDir
        # ($PROGRAMFILES64\CatchChallenger) is what an end user gets,
        # so don't override with /D — we WANT to test that path.
        win_args = [WINE_BIN, installer_exe, "/S"]
        diagnostic.record_cmd(win_args, os.path.dirname(installer_exe))
        log_info(f"wine64 {os.path.basename(installer_exe)} /S")
        proc = subprocess.Popen(
            win_args, cwd=os.path.dirname(installer_exe),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=force_headless_env(env),
            preexec_fn=process_helpers.setsid_and_pdeathsig)
        try:
            proc.communicate(timeout=COMPILE_TIMEOUT)
        except subprocess.TimeoutExpired:
            _kill_proc(proc)
        # NSIS silent installers can return before the child finishes
        # laying files down — poll for the three exes to materialise.
        waited = 0.0
        while waited < 60.0:
            for d in _wine_install_dirs():
                if (os.path.isfile(os.path.join(d, COMBINED_BIN_SRV))
                        and os.path.isfile(os.path.join(d, COMBINED_BIN_CPU))
                        and os.path.isfile(os.path.join(d, COMBINED_BIN_GL))):
                    install_dir = d
                    break
            if install_dir is not None:
                break
            time.sleep(1.0)
            waited += 1.0
        if install_dir is not None:
            log_pass(name2, f"installed -> {install_dir}")
        else:
            log_fail(name2, "3 exes not found under Program Files "
                            "after silent install")

    # ── 3. the 3 .exe must have distinct icons ──────────────────────
    name3 = "windows installed exe icons distinct"
    if install_dir is None:
        log_fail(name3, "install failed; cannot check icons")
    else:
        digs = {}
        none_for = []
        for fn in (COMBINED_BIN_GL, COMBINED_BIN_CPU, COMBINED_BIN_SRV):
            dg = _pe_icon_digest(os.path.join(install_dir, fn))
            if dg is None:
                none_for.append(fn)
            digs[fn] = dg
        if none_for:
            log_fail(name3, f"no embedded RT_ICON in: "
                            f"{', '.join(none_for)}")
        elif len(set(digs.values())) == 3:
            log_pass(name3, "3 distinct embedded icons")
        else:
            same = {}
            for fn, dg in digs.items():
                same.setdefault(dg, []).append(fn)
            dup = [v for v in same.values() if len(v) > 1]
            log_fail(name3, f"shared icon between: {dup}")

    # ── 4. installed server-gui opens the DB without crashing ───────
    name4 = "wine installed server-gui start (no crash)"
    srv_proc = None
    server_started = False
    if install_dir is None:
        log_fail(name4, "install failed; server-gui not run")
    elif not win_mc:
        log_fail(name4, "no datapack maincode to seed server-properties")
    else:
        srv_exe_inst = os.path.join(install_dir, COMBINED_BIN_SRV)
        _write_server_gui_properties(install_dir, win_mc)
        srv_proc, srv_out, srv_listen, srv_dberr, srv_serving = \
            _spawn_installed_server_gui(srv_exe_inst)
        waited = 0.0
        while waited < 60.0:
            if srv_proc.poll() is not None:
                break
            if srv_listen.is_set() or srv_dberr.is_set():
                break
            time.sleep(0.5)
            waited += 0.5
        rc = srv_proc.poll()
        crashed = rc is not None and rc != 0 \
            and process_helpers.is_sigabrt(rc)
        if srv_listen.is_set() and not srv_dberr.is_set():
            server_started = True
            log_pass(name4, "DB opened, server bound (Start OK)")
        elif srv_dberr.is_set():
            # The exact failure the user reported: socket binds, then
            # "Unable to connect to the database: Driver not loaded"
            # because sqldrivers/qsqlite.dll is missing from the
            # installed tree.
            log_fail(name4, "DB driver not loaded — installer is "
                            "missing sqldrivers/qsqlite.dll")
        elif crashed:
            log_fail(name4, f"SIGABRT on Start (rc={rc})")
        elif rc is not None:
            log_fail(name4, f"exited rc={rc} before opening DB")
        else:
            log_fail(name4, "timeout: never opened DB nor errored "
                            "(Start hung)")
        if not server_started:
            li = max(0, len(srv_out) - 50)
            while li < len(srv_out):
                print(f"  | {srv_out[li]}")
                li += 1

    # ── 5. installed qtcpu800x600 reaches the map vs (4) ────────────
    name5 = "wine installed qtcpu800x600 to map"
    if not server_started:
        log_fail(name5, "server-gui not running; client step skipped")
    else:
        # Deterministic start: wipe the client's per-server *synced*
        # datapack cache. It lives in the wine prefix user profile
        # (…/AppData/Local/CatchChallenger/<client>/datapack/argument-…),
        # NOT in install_dir, so a fresh `wine install` does NOT reset
        # it — it persists across every all.sh run. A previous run that
        # timed out mid datapack-sync leaves a *partial* cache there;
        # the next run's client then sees a partial/mismatched base
        # datapack, re-syncs, and stalls again → name5 fails forever
        # even though a genuinely fresh user (clean cache) reaches the
        # map in seconds. Matches the "behave as if from a fresh
        # post-restart shell" rule in test/CLAUDE.md.
        import glob as _glob
        _users_glob = os.path.join(_wine_prefix(), "drive_c", "users", "*",
                                   "AppData", "Local", "CatchChallenger")
        _wiped = 0
        for _cc in _glob.glob(_users_glob):
            for _client in ("client-qtcpu800x600", "client", "client-qtopengl"):
                _dp = os.path.join(_cc, _client, "datapack")
                if os.path.isdir(_dp):
                    shutil.rmtree(_dp, ignore_errors=True)
                    _wiped += 1
        if _wiped:
            log_info(f"wiped {_wiped} stale client datapack cache dir(s) "
                     f"for a deterministic sync")
        # name4 passed on the early "connected to sqlite" signal, but
        # the server-gui then spends several more seconds synchronously
        # loading the datapack + maps before preload_finish() prints
        # "Waiting connection on port" and it can actually serve a game
        # client. Launching the client before that → it connects into a
        # server still blocked in its load, the datapack sync never
        # completes and name5 times out at 120s (while a manual run
        # that waits for true-ready reaches the map in seconds). Block
        # until the real serving marker, bounded so a genuinely stuck
        # server still fails fast with a clear reason.
        _sw = 0.0
        while _sw < 90.0 and not srv_serving.is_set():
            if srv_proc.poll() is not None:
                break
            time.sleep(0.5)
            _sw += 0.5
        if not srv_serving.is_set():
            log_fail(name5, "server-gui never reached 'Waiting connection "
                            "on port' (datapack/map load stuck) — see "
                            "server console below")
            li = max(0, len(srv_out) - 50)
            while li < len(srv_out):
                print(f"  | {srv_out[li]}")
                li += 1
            _kill_proc(srv_proc)
            return
        cpu_exe_inst = os.path.join(install_dir, COMBINED_BIN_CPU)
        env = _wine_env()
        # NB: this runs the INSTALLED binary, whose payload has qoffscreen.dll
        # stripped by _strip_test_only_plugins(), so "-platform offscreen" can't
        # be forced here (no plugin) — rely on the env + virtual display.
        win_args = [WINE_BIN, cpu_exe_inst,
                    "--host", "127.0.0.1", "--port", E2E_PORT,
                    "--autologin", "--character", "WineE2E",
                    "--closewhenonmap"]
        diagnostic.record_cmd(win_args, install_dir)
        log_info(f"wine64 {COMBINED_BIN_CPU} --host 127.0.0.1 "
                 f"--port {E2E_PORT}")
        cproc = subprocess.Popen(
            win_args, cwd=install_dir, stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=force_headless_env(env),
            preexec_fn=process_helpers.setsid_and_pdeathsig)
        cli_out = []
        on_map = threading.Event()

        def creader():
            while True:
                raw = cproc.stdout.readline()
                if not raw:
                    break
                line = raw.decode(errors="replace").rstrip("\n")
                cli_out.append(line)
                if "MapVisualiserPlayer::mapDisplayedSlot()" in line:
                    on_map.set()

        threading.Thread(target=creader, daemon=True).start()
        elapsed = 0.0
        while elapsed < WINE_TIMEOUT:
            if cproc.poll() is not None:
                break
            if on_map.is_set():
                break
            time.sleep(0.5)
            elapsed += 0.5
        if on_map.is_set():
            log_pass(name5, "reached map vs installed server-gui")
        elif cproc.poll() is None:
            log_fail(name5, f"timeout {WINE_TIMEOUT}s, never on map")
        else:
            log_fail(name5, f"client exit rc={cproc.returncode} "
                            f"before map")
        if not on_map.is_set():
            li = max(0, len(cli_out) - 50)
            while li < len(cli_out):
                print(f"  | {cli_out[li]}")
                li += 1
        _kill_proc(cproc)

    # Tear the server down regardless of how the client fared.
    if srv_proc is not None:
        _kill_proc(srv_proc)


def run_wine_screenshot(exe_path, label, mode, screenshot_path,
                        timeout=8):
    """Spawn the .exe under wine64 with --take-screenshot=PATH and
    (for qtopengl with autosolo) --fixed so the start-screen / on-map
    PNG is byte-stable across runs. Waits up to `timeout` s for the
    screenshot file to appear, then kills wine.

    mode == "start"    → no --autosolo; the qtopengl 2 s fallback in
                         ScreenTransition.cpp grabs the start screen
                         and exits. qtcpu800x600 doesn't support the
                         flag yet so it just runs the regular UI
                         until kill — the test will fail on missing
                         file with a useful message instead of
                         silently passing.
    mode == "autosolo" → --autosolo --closewhenonmap; client enters
                         solo game, the on-map grab in
                         ScreenTransition.cpp:965 fires and quits.

    Returns True on success (screenshot file produced)."""
    if mode not in ("start", "autosolo"):
        raise ValueError(f"unknown screenshot mode: {mode}")
    # Drop a stale screenshot from the prior run so a stuck wine
    # invocation can't false-pass by leaving the previous PNG behind.
    if os.path.isfile(screenshot_path):
        try:
            os.remove(screenshot_path)
        except OSError:
            pass
    args = [f"--take-screenshot={screenshot_path}"]
    if mode == "autosolo":
        args.append("--autosolo")
        args.append("--closewhenonmap")
        # Pin the autosolo maincode to "test" — the dedicated
        # tiny-map fixture under
        # datapack/map/main/test/ is what the reference PNG was
        # captured from. Without this the in-process server picks
        # the first directory alphabetically (typically "official"),
        # which renders a completely different scene and breaks the
        # per-pixel diff against the blessed reference.
        args.append("--main-datapack-code=test")
    # --fixed pins CCBackground's animation timers — qtopengl only.
    # qtcpu800x600's UI has no animated background (no parallax, no
    # cloud/grass cycles) so the flag doesn't exist there; the
    # AutoArgs parser would reject an unknown arg.
    if "qtopengl" in label:
        args.append("--fixed")
    env = os.environ.copy()
    env["WINEPATH"]         = MXE_QT_BIN + ";" + MXE_RT_BIN
    env["WINEDEBUG"]        = "-all"
    env["WINEDLLOVERRIDES"] = WINE_DLLOVERRIDES_BASE
    env["QT_QPA_PLATFORM"]  = "offscreen"
    for _gui_var in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY"):
        env.pop(_gui_var, None)
    win_args = [WINE_BIN, exe_path, "-platform", "offscreen"] + args
    cwd = os.path.dirname(exe_path)
    log_info(f"wine64 screenshot {label} ({mode}) -> "
             f"{os.path.basename(screenshot_path)}")
    diagnostic.record_cmd(win_args, cwd)
    proc = subprocess.Popen(
        win_args, cwd=cwd, stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=force_headless_env(env),
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if os.path.isfile(screenshot_path) \
           and os.path.getsize(screenshot_path) > 100:
            # Give the client a beat to flush the PNG header → close
            # the file → exit. Polling .isfile alone races on a
            # half-written file.
            time.sleep(0.3)
            break
        if proc.poll() is not None:
            break
        time.sleep(0.2)
    if proc.poll() is None:
        try: os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError: pass
        try: proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            try: os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError: pass
            proc.wait(timeout=2)
    return os.path.isfile(screenshot_path)


def run_wine_screenshot_and_compare(exe_path, label, mode, reference_image):
    """Take a screenshot via wine64 then diff against `reference_image`
    using the shared image_compare helper (±10% per channel). Names
    the test "wine screenshot {label} ({mode})"; PASS / FAIL is
    routed through log_pass / log_fail like the rest of the harness."""
    import image_compare as _ic
    name = f"wine screenshot {label} ({mode})"
    screenshot_path = os.path.join(os.path.dirname(exe_path),
                                   f"screenshot-{label}-{mode}.png")
    diff_path = os.path.join(os.path.dirname(exe_path),
                             f"screenshot-{label}-{mode}-diff.png")
    if not run_wine_screenshot(exe_path, label, mode, screenshot_path):
        log_fail(name, "wine screenshot run produced no PNG file")
        return False
    ok, detail = _ic.compare_with_reference(
        name, screenshot_path, reference_image, diff_path=diff_path)
    if ok:
        log_pass(name, detail)
    else:
        log_fail(name, detail)
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
        env=force_headless_env(None),
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    ready = threading.Event()
    srv_lines = []
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            srv_lines.append(line)
            if "correctly bind:" in line:
                ready.set()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if ready.wait(timeout=SERVER_READY_TIMEOUT):
        return server_proc
    log_fail("server start", "timeout waiting for 'correctly bind:'")
    for l in srv_lines[-30:]:
        print(f"  | {l}")
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


# ── real Windows laptop (non-admin, user-level install) ─────────────────────
WIN_REAL_HOST      = "192.168.158.110"
WIN_REAL_USER      = "ASUS"
WIN_REAL_PORT      = 22
WIN_REAL_WORK_DIR  = "C:/cc-test"        # forward slashes work in PowerShell
WIN_REAL_SERVER_PORT = "61920"           # distinct from E2E_PORT / SERVER_PORT

_WIN_REAL_CASE_NAMES = (
    "real-win uninstall pre-clean",
    "real-win install user installer",
    "real-win server-gui start",
    "real-win qtopengl --autosolo",
    "real-win qtcpu800x600 --autosolo",
)

_SSH_WIN = ["ssh",
            "-o", "StrictHostKeyChecking=no",
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=10",
            # LogLevel=ERROR silences the OpenSSH post-quantum "store now,
            # decrypt later" advisory the newer client prints to stderr
            # against the node's older sshd — pure noise that otherwise
            # pollutes every captured command output / failure detail.
            "-o", "LogLevel=ERROR",
            "-p", str(WIN_REAL_PORT),
            f"{WIN_REAL_USER}@{WIN_REAL_HOST}"]


def _ps_encoded(ps_cmd):
    """argv tail to run ps_cmd via `powershell -EncodedCommand`.

    The Windows node's default ssh shell is cmd.exe, which parses the
    remote command line BEFORE PowerShell sees it — so any cmd metachar in
    the script ('|', '(', ')', '&', '<', '>', '^', ';') is mangled or
    eaten. Base64(UTF-16LE) contains only [A-Za-z0-9+/=], so cmd passes it
    through verbatim and PowerShell decodes the original script intact."""
    enc = base64.b64encode(ps_cmd.encode("utf-16-le")).decode("ascii")
    return ["powershell", "-NonInteractive", "-EncodedCommand", enc]

def _win_real_reachable():
    """Return True when the real Windows node answers SSH within 10 s."""
    try:
        p = subprocess.run(
            _SSH_WIN + ["echo ok"],
            capture_output=True, timeout=15)
        return p.returncode == 0 and b"ok" in p.stdout
    except Exception:
        return False


def _win_ssh(ps_cmd, timeout=60):
    """Run a PowerShell one-liner on the real Windows node via SSH.

    Returns (rc, output_str). Wraps the command in powershell -Command
    so the remote shell is PowerShell regardless of the sshd DefaultShell
    setting."""
    # $ProgressPreference=SilentlyContinue suppresses the "Preparando
    # módulos para el primer uso" progress records PowerShell otherwise
    # serialises as #< CLIXML noise onto stderr, polluting every captured
    # output and failure detail.
    args = _SSH_WIN + _ps_encoded(
        "$ProgressPreference='SilentlyContinue'; " + ps_cmd)
    try:
        p = subprocess.run(args, capture_output=True, timeout=timeout)
        out = p.stdout.decode(errors="replace") + p.stderr.decode(errors="replace")
        return p.returncode, out
    except subprocess.TimeoutExpired:
        return -1, "TIMEOUT"
    except Exception as exc:
        return -1, str(exc)


def _win_rsync(src, dst_rel, timeout=300):
    """Copy a local file/dir to WIN_REAL_WORK_DIR/<dst_rel> on the Windows
    node via scp.

    rsync-over-ssh fails here because the stock Windows OpenSSH server has
    no rsync.exe on PATH — the remote `rsync --server` exec dies with the
    Windows "program or batch file not found" error and the client sees a
    code-12 protocol abort. scp ships with OpenSSH itself (the same daemon
    that already answered the SSH probe), so it always resolves.

    dst_rel uses forward slashes (e.g. 'catchchallenger-installer-user.exe').
    For a directory source the destination is wiped first so scp -r recreates
    it from src's CONTENTS (cp -r semantics when the target does not pre-exist),
    matching the old rsync 'src/ -> dst/' behaviour rather than nesting
    src under dst/."""
    src = src.rstrip("/")
    dst_rel = dst_rel.rstrip("/")
    remote_root = WIN_REAL_WORK_DIR.replace("\\", "/")          # C:/cc-test
    remote_path = f"{remote_root}/{dst_rel}"
    is_dir = os.path.isdir(src)
    # Ensure the work dir exists; for a dir target wipe any stale copy so
    # scp -r lands src's contents AS the target instead of inside it.
    win_root = remote_root.replace("/", "\\")
    win_dst = remote_path.replace("/", "\\")
    # NOTE: the Windows node's default ssh shell is cmd.exe, and _win_ssh
    # passes the PowerShell command unquoted, so any '|' is grabbed by cmd
    # (which has no Out-Null) before PowerShell sees it. Use [void](...) to
    # discard New-Item's output without a pipe.
    prep = f"[void](New-Item -ItemType Directory -Force -Path '{win_root}')"
    if is_dir:
        prep += (f"; if (Test-Path '{win_dst}') "
                 f"{{ Remove-Item -Recurse -Force '{win_dst}' }}")
    rc_prep, out_prep = _win_ssh(prep, timeout=60)
    if rc_prep != 0:
        return rc_prep, "prep mkdir/rm failed: " + out_prep
    args = ["scp", "-q",
            "-o", "StrictHostKeyChecking=no",
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=10",
            "-o", "LogLevel=ERROR",
            "-P", str(WIN_REAL_PORT)]
    if is_dir:
        args.append("-r")
    args += [src, f"{WIN_REAL_USER}@{WIN_REAL_HOST}:{remote_path}"]
    try:
        p = subprocess.run(args, capture_output=True, timeout=timeout)
        out = p.stdout.decode(errors="replace") + p.stderr.decode(errors="replace")
        return p.returncode, out
    except subprocess.TimeoutExpired:
        return -1, "TIMEOUT"
    except Exception as exc:
        return -1, str(exc)


_WIN_OFFSCREEN_DLL = (MXE_PREFIX + "/" + MXE_TARGET
                      + "/qt6/plugins/platforms/qoffscreen.dll")


def _win_upload_bat(name, body):
    """Write a cmd batch `body` (CRLF) to C:\\cc-test\\<name> on the node.
    Returns (rc, remote_win_path, out)."""
    local = os.path.join(tempfile.gettempdir(), name)
    with open(local, "w", newline="\r\n") as f:
        f.write(body)
    rc, out = _win_rsync(local, name, timeout=60)
    win_path = (WIN_REAL_WORK_DIR + "/" + name).replace("/", "\\")
    return rc, win_path, out


def _win_read_remote(win_path):
    """Return the full text of a remote file via SSH ('' if absent). The
    path is literal (no $env), so single quotes are safe.

    Opens with FileShare.ReadWrite so a file still held open by a writing
    process (cmd '>' redirect of a running server) can still be read —
    plain Get-Content -Raw fails with a sharing violation and returns ''."""
    _rc, out = _win_ssh(
        f"if (Test-Path '{win_path}') {{ "
        f"$fs=[System.IO.File]::Open('{win_path}','Open','Read','ReadWrite'); "
        "$sr=New-Object System.IO.StreamReader($fs); "
        "Write-Output $sr.ReadToEnd(); $sr.Close(); $fs.Close() }",
        timeout=30)
    return out


def _win_server_state(port, procname):
    """One round-trip probe of a server launched on the node: returns
    'OPEN' (TCP port accepts a local connection — bound & listening),
    'ALIVE' (process running but port not yet open) or 'DEAD' (process
    gone). Local 127.0.0.1 connect sidesteps the inbound firewall AND the
    cmd '>' log-file write lock that blocks reading the log mid-run."""
    _rc, out = _win_ssh(
        "$open=$false; try { $c=New-Object Net.Sockets.TcpClient; "
        f"$c.Connect('127.0.0.1',{port}); $open=$c.Connected; $c.Close() }} catch {{ }}; "
        "if($open){ 'OPEN' } "
        f"elseif(Get-Process {procname} -ErrorAction SilentlyContinue){{ 'ALIVE' }} "
        "else { 'DEAD' }",
        timeout=20)
    if "OPEN" in out:
        return "OPEN"
    if "DEAD" in out:
        return "DEAD"
    return "ALIVE"


def _win_gui_capture_bat(exe_basename, args, log_basename):
    """Build a cmd batch that runs an installed GUI .exe with output
    redirected to a file.

    Why a batch + redirect (not Start-Process -RedirectStandardOutput or a
    piped Popen): the MXE Qt binaries are GUI-subsystem (Subsystem 2). Over
    a non-interactive SSH session their std::cout/std::cerr is NOT delivered
    to a .NET/PowerShell-created pipe (0 bytes — wine bridged this, native
    Windows does not). cmd's '>' redirection sets the child's std handles to
    the file via STARTUPINFO, which the GUI child DOES inherit and write to.
    QT_QPA_PLATFORM=offscreen + '-platform offscreen' because the SSH session
    has no interactive desktop and the installer strips the offscreen plugin
    (re-added next to the exe by the caller for the test)."""
    arg_str = " ".join(args)
    return (
        "@echo off\r\n"
        "set QT_QPA_PLATFORM=offscreen\r\n"
        "cd /d \"%LOCALAPPDATA%\\CatchChallenger\"\r\n"
        f"\"%LOCALAPPDATA%\\CatchChallenger\\{exe_basename}\" "
        f"-platform offscreen {arg_str} > \"C:\\cc-test\\{log_basename}\" 2>&1\r\n"
        f"echo EXITCODE=%ERRORLEVEL% >> \"C:\\cc-test\\{log_basename}\"\r\n"
    )


def run_on_real_windows(user_installer_exe, win_dp_src):
    """Five sequential steps against a real Windows 10 laptop (non-admin).

    Requires the real-Windows node reachable at WIN_REAL_HOST over SSH.
    Skips all five steps (with SKIP log) when the node is not reachable.

    Steps:
      1. Wipe %LOCALAPPDATA%\\CatchChallenger and %APPDATA%\\CatchChallenger
      2. rsync user-level installer + datapack, install silently (/S)
      3. server-gui.exe --autostart on Windows, Linux client connects
      4. catchchallenger.exe --autosolo (qtopengl) on Windows
      5. catchchallengercpu800x600.exe --autosolo on Windows"""

    skip_reason = None
    if user_installer_exe is None or not os.path.isfile(user_installer_exe):
        skip_reason = "user installer not built"
    elif not _win_real_reachable():
        skip_reason = f"node {WIN_REAL_HOST} not reachable"

    if skip_reason is not None:
        for n in _WIN_REAL_CASE_NAMES:
            log_info(f"[SKIP] {n}: {skip_reason}")
        return

    # ── 1. uninstall / pre-clean ─────────────────────────────────────────────
    name1 = _WIN_REAL_CASE_NAMES[0]
    rc1, out1 = _win_ssh(
        "Remove-Item -Recurse -Force "
        "$env:LOCALAPPDATA\\CatchChallenger,"
        "$env:APPDATA\\CatchChallenger "
        "-ErrorAction SilentlyContinue; "
        "if (Test-Path $env:LOCALAPPDATA\\CatchChallenger) "
        "{ Write-Error 'still present'; exit 1 } else { exit 0 }",
        timeout=30)
    if rc1 == 0:
        log_pass(name1)
    else:
        log_fail(name1, f"rc={rc1} {out1.strip()[-300:]}")

    # ── 2. install from user-level NSIS installer ─────────────────────────────
    name2 = _WIN_REAL_CASE_NAMES[1]
    installer_basename = os.path.basename(user_installer_exe)
    rc_sync, sync_out = _win_rsync(user_installer_exe, installer_basename,
                                   timeout=120)
    if rc_sync != 0:
        log_fail(name2, f"rsync failed rc={rc_sync} {sync_out.strip()[-200:]}")
        for n in _WIN_REAL_CASE_NAMES[2:]:
            log_fail(n, "install failed; step skipped")
        return

    # Stage the datapack to C:\cc-test\datapack (scp), then it's copied
    # NEXT TO THE EXE below — both the server-gui and the --autosolo solo
    # clients require datapack/informations.xml beside the binary
    # (server-gui ignores --datapack-dir; the installer ships no datapack).
    if win_dp_src and os.path.isdir(win_dp_src):
        # The server-gui is configured for ONE maincode ("official"); stage a
        # PRUNED datapack (that maincode only) before transfer. scp -r over SSH
        # pays a per-file round-trip for the datapack's THOUSANDS of small files,
        # so the full multi-maincode datapack overran the timeout and landed
        # WITHOUT map/main/official/, aborting the server-gui with "0 file for
        # datapack loaded main". Pruning to the needed maincode cuts the file
        # count so the scp completes.
        prune_dir = build_paths.build_path("client/build/real-win-datapack")
        prune_dp = os.path.join(prune_dir, "datapack")
        if os.path.exists(prune_dir):
            shutil.rmtree(prune_dir, ignore_errors=True)
        os.makedirs(prune_dir, exist_ok=True)
        shutil.copytree(win_dp_src, prune_dp, ignore=_datapack_ignore)
        _mm = os.path.join(prune_dp, "map", "main")
        if os.path.isdir(_mm):
            for _e in os.listdir(_mm):
                if _e != "official" and os.path.isdir(os.path.join(_mm, _e)):
                    shutil.rmtree(os.path.join(_mm, _e), ignore_errors=True)
        # Transfer the datapack as a SINGLE tar, not scp -r: scp -r onto the stock
        # Windows OpenSSH server lands the root files (so datapack-ok=informations.xml
        # passes) but DROPS the deep map/main/<maincode>/ subtree, so the server
        # aborted with "0 file for datapack loaded main". One archive + tar -x on the
        # box (Windows 10 ships tar.exe/bsdtar) transfers the whole tree reliably.
        import tarfile
        tar_local = os.path.join(prune_dir, "datapack.tar")
        with tarfile.open(tar_local, "w") as _tf:
            _tf.add(prune_dp, arcname="datapack")
        _rc_t, _o_t = _win_rsync(tar_local, "datapack.tar", timeout=300)   # scp ONE file
        if _rc_t == 0:
            _win_ssh("tar -xf 'C:\\cc-test\\datapack.tar' -C 'C:\\cc-test'", timeout=180)
        else:
            _win_rsync(prune_dp + "/", "datapack/", timeout=300)            # fallback
    # Offscreen platform plugin: the installer strips qoffscreen.dll for
    # shipping, but the headless SSH session has no interactive desktop, so
    # re-supply it next to the exe for the test (same plugin the wine path
    # uses). Staged to C:\cc-test, copied into platforms\ during setup.
    if os.path.isfile(_WIN_OFFSCREEN_DLL):
        _win_rsync(_WIN_OFFSCREEN_DLL, "qoffscreen.dll", timeout=60)

    win_work = WIN_REAL_WORK_DIR.replace("/", "\\")
    # Start-Process -Wait so we block until the NSIS installer FINISHES
    # extracting. The old `& installer /S` + poll-for-one-file pattern
    # returned the instant catchchallenger.exe appeared — but NSIS is a
    # GUI-subsystem process that `&` does NOT wait for, so extraction was
    # still in flight and ~50 of the 70 staged DLLs (libgcc/libstdc++/
    # libsqlite3/…) had not landed yet → the server then died at load
    # with STATUS_DLL_NOT_FOUND (0xC0000135).
    rc2, out2 = _win_ssh(
        f"$p = Start-Process -FilePath '{win_work}\\{installer_basename}' "
        "-ArgumentList '/S' -Wait -PassThru; "
        "if (Test-Path $env:LOCALAPPDATA\\CatchChallenger\\" + COMBINED_BIN_GL + ") "
        "{ Write-Output 'installed' } "
        "else { Write-Error 'install incomplete (exe missing)'; exit 1 }",
        timeout=180)
    if rc2 == 0 and "installed" in out2:
        log_pass(name2)
    else:
        log_fail(name2, f"rc={rc2} {out2.strip()[-400:]}")
        for n in _WIN_REAL_CASE_NAMES[2:]:
            log_fail(n, "install failed; step skipped")
        return

    # ── post-install setup: datapack + server-properties.xml + offscreen ──────
    # All three land in the install dir so the server-gui (applicationDirPath
    # lookup) and the solo clients (datapack beside exe) work headlessly.
    srv_props = (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        f'    <server-port value="{WIN_REAL_SERVER_PORT}"/>\n'
        '    <server-ip value=""/>\n'
        '    <httpDatapackMirror value=""/>\n'
        '    <max-players value="10"/>\n'
        '    <pvp value="true"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        '    <content>\n'
        '        <mainDatapackCode value="official"/>\n'
        '        <subDatapackCode value=""/>\n'
        '    </content>\n'
        '</configuration>\n'
    )
    setup_ps = (
        "$d = \"$env:LOCALAPPDATA\\CatchChallenger\"\n"
        # Force a FRESH copy: a stale/locked $d\\datapack left by a prior run (whose
        # server-gui may still hold a handle past the pre-clean) must NOT make this
        # a no-op (the old `if (-not Test-Path)` guard skipped it, leaving an
        # incomplete datapack -> server abort "0 file for datapack loaded main").
        "if (Test-Path \"$d\\datapack\") { Remove-Item -Recurse -Force \"$d\\datapack\" -ErrorAction SilentlyContinue }\n"
        "if (Test-Path 'C:\\cc-test\\datapack') { Copy-Item -Recurse 'C:\\cc-test\\datapack' \"$d\\datapack\" }\n"
        "[void](New-Item -ItemType Directory -Force -Path \"$d\\platforms\")\n"
        "if (Test-Path 'C:\\cc-test\\qoffscreen.dll') "
        "{ Copy-Item 'C:\\cc-test\\qoffscreen.dll' \"$d\\platforms\\qoffscreen.dll\" -Force }\n"
        "Set-Content -LiteralPath \"$d\\server-properties.xml\" -Encoding UTF8 -Value @'\n"
        + srv_props +
        "'@\n"
        "Write-Output (\"datapack-ok=\" + (Test-Path \"$d\\datapack\\informations.xml\"))\n"
        # Diagnose where the 'official' maincode is lost: in the transferred staging
        # dir vs the copied destination.
        "Write-Output (\"cctest-official=\" + (Test-Path 'C:\\cc-test\\datapack\\map\\main\\official'))\n"
        "Write-Output (\"dest-official=\" + (Test-Path \"$d\\datapack\\map\\main\\official\"))\n"
    )
    rc_setup, out_setup = _win_ssh(setup_ps, timeout=120)
    log_info("real-win datapack setup: " + " ".join(
        l.strip() for l in out_setup.splitlines()
        if "official=" in l or "datapack-ok=" in l))
    if "datapack-ok=True" not in out_setup:
        log_fail(_WIN_REAL_CASE_NAMES[2],
                 f"post-install setup failed (datapack not staged): "
                 f"{out_setup.strip()[-200:]}")
        for n in _WIN_REAL_CASE_NAMES[3:]:
            log_fail(n, "setup failed; step skipped")
        return

    # ── 3. server-gui --autostart binds its TCP port ─────────────────────────
    # GUI-subsystem stdout is only capturable through a cmd '>' redirect (see
    # _win_gui_capture_bat); the marker "Waiting connection on port" is the
    # GUIServer DB-OK + listen-ready signal.
    name3 = _WIN_REAL_CASE_NAMES[2]
    srv_log = "srv-real.log"
    _rc_b, srv_bat, _o = _win_upload_bat(
        "run-srv-real.bat",
        _win_gui_capture_bat(COMBINED_BIN_SRV, ["--autostart"], srv_log))
    # Launch via WMI Win32_Process.Create, NOT Start-Process: Windows
    # OpenSSH kills the whole process tree spawned by an SSH session when
    # the channel closes, so a Start-Process'd server dies the instant this
    # _win_ssh returns — before the first cross-session poll. Win32_Process
    # .Create spawns a process owned by the WMI service, detached from the
    # session, so it survives until we explicitly Stop-Process it by name.
    _win_ssh("Get-Process catchchallenger-server-gui -ErrorAction "
             "SilentlyContinue | Stop-Process -Force; "
             f"Remove-Item 'C:\\cc-test\\{srv_log}' -ErrorAction SilentlyContinue; "
             "[void](Invoke-CimMethod -ClassName Win32_Process -MethodName Create "
             f"-Arguments @{{CommandLine='cmd /c {srv_bat}'}})",
             timeout=30)
    # Poll the captured log (read with FileShare.ReadWrite so the live
    # cmd '>' write-lock doesn't block us). "Waiting connection on port" is
    # emitted by GUIServer ONLY after the SQL backend connected AND the
    # server datapack loaded — so it is positive proof that the SQLite
    # plugin (sqldrivers/qsqlite.dll + libsqlite3-0.dll) actually ATTACHED,
    # not merely that the TCP port bound (the bare "Listen *:port" line is
    # printed BEFORE DB init and would pass a naive port probe even when the
    # DB driver is missing).
    srv_ready = False
    srv_fail = False
    srv_dead = False
    srv_out = ""
    deadline = time.time() + SERVER_READY_TIMEOUT
    while time.time() < deadline:
        srv_out = _win_read_remote(f"C:\\cc-test\\{srv_log}")
        low = srv_out.lower()
        if "waiting connection on port" in low:
            srv_ready = True
            break
        if "driver not loaded" in low or "database initialization failed" in low:
            srv_fail = True
            break
        if "exitcode=" in low:   # .bat appends this only after the exe exits
            srv_dead = True
            break
        time.sleep(3)

    def _stop_win_server():
        _win_ssh("Get-Process catchchallenger-server-gui -ErrorAction "
                 "SilentlyContinue | Stop-Process -Force", timeout=30)

    if not srv_ready:
        _stop_win_server()
        if srv_fail:
            sql_line = next((l for l in srv_out.splitlines()
                             if "driver not loaded" in l.lower()
                             or "database initialization failed" in l.lower()), "")
            log_fail(name3, f"SQL plugin NOT attached: {sql_line.strip()}")
        elif srv_dead:
            log_fail(name3, "server-gui exited before it began listening")
        else:
            log_fail(name3, "server-gui never printed 'Waiting connection "
                            "on port' within timeout")
        for l in srv_out.splitlines()[-20:]:
            print(f"  | {l}")
        for n in _WIN_REAL_CASE_NAMES[3:]:
            log_fail(n, "server not running; step skipped")
        return

    # SQL is proven attached by the ready marker; also confirm the TCP port
    # actually accepts a local connection, and surface the concrete SQL line.
    port_ok = (_win_server_state(WIN_REAL_SERVER_PORT,
                                 "catchchallenger-server-gui") == "OPEN")
    sql_evidence = next((l.strip() for l in srv_out.splitlines()
                         if "connected to sqlite" in l.lower()
                         or "loaded the server sql datapack" in l.lower()), "")
    # The server is no longer needed for the solo-client steps (--autosolo
    # runs its own embedded server); stop it so it can't hold the port.
    _stop_win_server()
    if not port_ok:
        log_fail(name3, "SQL plugin attached but TCP port not accepting")
        for n in _WIN_REAL_CASE_NAMES[3:]:
            log_fail(n, "server not listening; step skipped")
        return
    log_pass(name3, f"SQL plugin attached + listening on "
                    f"127.0.0.1:{WIN_REAL_SERVER_PORT}"
                    + (f" — {sql_evidence}" if sql_evidence else ""))

    def _check_jpeg(out, label):
        """Return (jpeg_ok, detail) by parsing 'ImageFormats:...' from output.

        BaseWindow.cpp prints 'ImageFormats:<comma-list>' at startup.
        If the line is absent the plugin loading step ran but the binary
        predates the print — treat as unknown (warn, don't fail)."""
        for line in out.splitlines():
            if line.startswith("ImageFormats:"):
                fmts = line[len("ImageFormats:"):].lower().split(",")
                if "jpeg" in fmts or "jpg" in fmts:
                    return True, "jpeg in " + ",".join(fmts)
                return False, "jpeg MISSING from " + ",".join(fmts)
        return None, "ImageFormats line not found in output (old binary?)"

    # ── 4 & 5. solo clients (--autosolo) on Windows ──────────────────────────
    # Each runs its own embedded server; success = the map-displayed marker
    # reaches the cmd-redirected log within the timeout. Output capture uses
    # the same GUI-subsystem cmd '>' redirect as the server (a piped Popen /
    # PowerShell redirect gets 0 bytes from a GUI-subsystem .exe over SSH).
    def _run_solo_client(case_name, exe_basename, log_basename):
        body = _win_gui_capture_bat(
            exe_basename,
            ["--autosolo", "--closewhenonmap", "--maincode", "test"],
            log_basename)
        _rc, bat_path, _o = _win_upload_bat(
            "run-" + log_basename.replace(".log", "") + ".bat", body)
        timeout_ms = WINE_TIMEOUT * 2 * 1000
        _win_ssh(
            f"Remove-Item 'C:\\cc-test\\{log_basename}' -ErrorAction SilentlyContinue; "
            f"$p = Start-Process -FilePath '{bat_path}' -WindowStyle Hidden -PassThru; "
            f"if (-not $p.WaitForExit({timeout_ms})) {{ "
            "  Get-Process catchchallenger,catchchallengercpu800x600 -ErrorAction "
            "SilentlyContinue | Stop-Process -Force; "
            f"  Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }}",
            timeout=WINE_TIMEOUT * 2 + 60)
        return _win_read_remote(f"C:\\cc-test\\{log_basename}")

    name4 = _WIN_REAL_CASE_NAMES[3]
    out4 = _run_solo_client(name4, COMBINED_BIN_GL, "cli-gl.log")
    on_map4 = "MapVisualiserPlayer::mapDisplayedSlot()" in out4
    if on_map4:
        log_pass(name4)
    else:
        log_fail(name4, f"on_map={on_map4} {out4.strip()[-400:]}")

    name5 = _WIN_REAL_CASE_NAMES[4]
    out5 = _run_solo_client(name5, COMBINED_BIN_CPU, "cli-cpu.log")
    on_map5 = "MapVisualiserPlayer::mapDisplayedSlot()" in out5
    jpeg_ok, jpeg_detail = _check_jpeg(out5, "qtcpu800x600")
    if on_map5 and jpeg_ok is not False:
        detail5 = jpeg_detail if jpeg_ok else f"WARN: {jpeg_detail}"
        log_pass(name5, detail5)
    elif on_map5 and jpeg_ok is False:
        log_fail(name5, f"reached map but JPEG plugin missing: {jpeg_detail}")
    else:
        log_fail(name5, f"on_map={on_map5} jpeg={jpeg_detail} "
                        f"{out5.strip()[-400:]}")


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

    cpu_exe  = None
    gl_exe   = None
    srv_exe  = None
    # Transitive compile: --onlyfailed may queue a downstream test
    # (wine run, screenshot, installer) without queuing the compile
    # row itself. The previous run's post-success sweep wipes
    # /mnt/data/perso/tmpfs/cc-build/, so find_built_exe() then
    # returns None and every dependent test silently no-ops with
    # "0 passed, 0 failed". Force the compile when any downstream
    # case for that binary is queued.
    def _needs_compile(binary, failed_cases):
        if failed_cases is None:
            return True
        dependents = {
            "qtcpu800x600": ["compile qtcpu800x600 (mxe-x86_64)",
                             "wine run qtcpu800x600 --autosolo",
                             "wine run qtcpu800x600",
                             "wine screenshot qtcpu800x600 (start)",
                             "wine screenshot qtcpu800x600 (autosolo)",
                             "sign exe qtcpu800x600",
                             "installer combined", "msi combined",
                             "sign installer combined"]
                            + list(_E2E_CASE_NAMES)
                            + list(_WIN_REAL_CASE_NAMES),
            "qtopengl":     ["compile qtopengl (mxe-x86_64)",
                             "wine run qtopengl --autosolo",
                             "wine run qtopengl",
                             "wine screenshot qtopengl (start)",
                             "wine screenshot qtopengl (autosolo)",
                             "sign exe qtopengl",
                             "installer combined", "msi combined",
                             "sign installer combined"]
                            + list(_E2E_CASE_NAMES)
                            + list(_WIN_REAL_CASE_NAMES),
            # The e2e steps install + run all three binaries, so a
            # queued e2e case must force a server-gui recompile too.
            "server-gui":   ["compile server-gui (mxe-x86_64)",
                             "installer combined", "msi combined",
                             "sign installer combined"]
                            + list(_E2E_CASE_NAMES)
                            + list(_WIN_REAL_CASE_NAMES),
        }
        idx = 0
        names = dependents[binary]
        while idx < len(names):
            if names[idx] in failed_cases:
                return True
            idx += 1
        return False
    if _needs_compile("qtcpu800x600", failed_cases):
        cpu_exe = build_mxe_client(CLIENT_CPU_PRO, CLIENT_CPU_BUILD_WIN,
                                   "qtcpu800x600")
    else:
        cpu_exe = find_built_exe(CLIENT_CPU_BUILD_WIN)
    if _needs_compile("qtopengl", failed_cases):
        gl_exe = build_mxe_client(CLIENT_GL_PRO, CLIENT_GL_BUILD_WIN,
                                  "qtopengl")
    else:
        gl_exe = find_built_exe(CLIENT_GL_BUILD_WIN)
    if _needs_compile("server-gui", failed_cases):
        srv_exe = build_mxe_client(SERVER_GUI_PRO, SERVER_GUI_BUILD_WIN,
                                   "server-gui")
    else:
        srv_exe = find_built_exe(SERVER_GUI_BUILD_WIN,
                                 exe_name="catchchallenger-server-gui.exe")

    win_dp_src = DATAPACKS[0] if DATAPACKS else None
    win_mc = None
    if win_dp_src is not None:
        mcs = detect_maincodes(win_dp_src)
        if mcs:
            win_mc = mcs[0]
    # ── server-gui startup smoke ────────────────────────────────────
    # Stage a fresh datapack next to the freshly-built server-gui.exe
    # and launch it under wine64 for 5 seconds with
    # WINEDEBUG=+loaddll,+module — surfaces missing DLL / Qt-plugin
    # dependency before the installer step packages a broken binary.
    # Distinct from the wine_run_client tests: those exercise a
    # CLIENT against an external Linux server-filedb (testingmulti
    # convention), whereas this directly tests the SERVER-side
    # binary that ships in the installer.
    if srv_exe is not None and should_run(
            "wine server smoke server-gui", failed_cases):
        if win_dp_src and win_mc:
            setup_datapack_client(os.path.dirname(srv_exe), win_dp_src,
                                  win_mc, f"wine server-gui ({win_mc})")
        run_wine_server_smoke(srv_exe, "server-gui", wait_seconds=5)

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

    # ── reference-screenshot regression (both clients) ──────────────
    # Two captures per client (qtopengl + qtcpu800x600), four total:
    #   - start    : 2 s after .exe start, no --autosolo (title screen)
    #   - autosolo : on-map grab inside MapVisualiserPlayer slot
    # qtopengl screenshots pass --fixed so CCBackground's cloud /
    # grass / tree-parallax timers don't drift the PNG. qtcpu800x600
    # has no animated background — its AutoArgs doesn't even accept
    # --fixed — so run_wine_screenshot skips the flag there.
    # Autosolo runs use maincode="test" (the dedicated small-map
    # fixture under datapack/map/main/test/) so the rendered scene
    # matches the blessed reference; without an explicit maincode
    # the in-process server picks the first dir alphabetically
    # ("official") and the screenshot diff fails on every pixel.
    # References at test/screenshot-windows-<client>-{start,autosolo}.png;
    # the diff mask drops beside the produced PNG on failure.
    if gl_exe is not None and should_run(
            "wine screenshot qtopengl (start)", failed_cases):
        ref_start = os.path.join(ROOT, "test",
                                 "screenshot-windows-qtopengl-start.png")
        run_wine_screenshot_and_compare(gl_exe, "qtopengl", "start",
                                        ref_start)
    if gl_exe is not None and should_run(
            "wine screenshot qtopengl (autosolo)", failed_cases):
        if win_dp_src:
            setup_datapack_client(os.path.dirname(gl_exe), win_dp_src,
                                  "test", "wine qtopengl (test)")
        ref_solo = os.path.join(ROOT, "test",
                                "screenshot-windows-qtopengl-autosolo.png")
        run_wine_screenshot_and_compare(gl_exe, "qtopengl", "autosolo",
                                        ref_solo)
    if cpu_exe is not None and should_run(
            "wine screenshot qtcpu800x600 (start)", failed_cases):
        ref_start = os.path.join(ROOT, "test",
                                 "screenshot-windows-qtcpu800x600-start.png")
        run_wine_screenshot_and_compare(cpu_exe, "qtcpu800x600", "start",
                                        ref_start)
    if cpu_exe is not None and should_run(
            "wine screenshot qtcpu800x600 (autosolo)", failed_cases):
        if win_dp_src:
            setup_datapack_client(os.path.dirname(cpu_exe), win_dp_src,
                                  "test", "wine qtcpu800x600 (test)")
        ref_solo = os.path.join(ROOT, "test",
                                "screenshot-windows-qtcpu800x600-autosolo.png")
        run_wine_screenshot_and_compare(cpu_exe, "qtcpu800x600", "autosolo",
                                        ref_solo)

    # Sign the catchchallenger.exe BEFORE bundling — both the NSIS
    # installer and the MSI then ship a signed inner binary.  Done after
    # the autosolo wine run so the unsigned binary is what we exercise
    # for runtime smoke; signing only mutates the embedded Authenticode
    # blob, but doing it post-run keeps build-vs-test steps separable.
    if cpu_exe is not None and should_run("sign exe qtcpu800x600", failed_cases):
        sign_authenticode(cpu_exe, "qtcpu800x600", "exe")
    if gl_exe is not None and should_run("sign exe qtopengl", failed_cases):
        sign_authenticode(gl_exe, "qtopengl", "exe")

    # Combined installer/MSI step — merges qtcpu800x600 + qtopengl +
    # server-gui into ONE installer that lays out three .exe files
    # plus three Start-Menu shortcuts, sharing one copy of Qt6 DLLs /
    # plugins / datapack. Output names (canonical, what
    # publish_binaries.sh + deploy.sh expect):
    #   /mnt/data/perso/tmpfs/catchchallenger-installer.exe
    #   /mnt/data/perso/tmpfs/catchchallenger.msi
    #
    # The per-binary stage dirs are produced by _stage_install_payload
    # (DLL-allowlist filtered, autogen/etc stripped) — call it once
    # per binary, then _stage_combined_payload merges them.
    if cpu_exe is not None and gl_exe is not None and srv_exe is not None:
        # _stage_combined_part rebuilds a binary whose tmpfs build dir was
        # swept by a concurrent run's cleanup-on-exit, and returns None only
        # if it still can't be produced — so a vanished dir no longer aborts
        # the whole script with an unhandled FileNotFoundError.
        parts = [
            _stage_combined_part(gl_exe, CLIENT_GL_PRO, CLIENT_GL_BUILD_WIN,
                                 "qtopengl", WIN_EXE_NAME, COMBINED_BIN_GL),
            _stage_combined_part(cpu_exe, CLIENT_CPU_PRO, CLIENT_CPU_BUILD_WIN,
                                 "qtcpu800x600", WIN_EXE_NAME, COMBINED_BIN_CPU),
            _stage_combined_part(srv_exe, SERVER_GUI_PRO, SERVER_GUI_BUILD_WIN,
                                 "server-gui", os.path.basename(srv_exe),
                                 COMBINED_BIN_SRV),
        ]
        if any(p is None for p in parts):
            detail = ("a Windows binary build dir was missing/incomplete at "
                      "packaging time and could not be rebuilt (concurrent "
                      "cleanup of the shared tmpfs build dir?) — re-run")
            log_fail("installer combined", detail)
            log_fail("msi combined", detail)
            for _n in list(_E2E_CASE_NAMES) + list(_WIN_REAL_CASE_NAMES):
                if should_run(_n, failed_cases):
                    log_fail(_n, "combined payload unavailable; step skipped")
        else:
            # Pre-installer .exe guard — walk every .exe that will land
            # inside the NSIS installer / .msi and fail when ANY of them
            # is >= 80 MiB. A release-mode mingw .exe of the qt clients is
            # ~30-50 MiB; >= 80 MiB almost always means a Debug build
            # leaked -g symbols through. Aborting before the packaging
            # step keeps the bad payload out of the shipped artefacts and
            # surfaces the root cause (wrong CMAKE_BUILD_TYPE).
            oversized = []
            debug_syms = []
            # A Release mingw .exe still carries a small (~100 KiB)
            # .debug_info section originating from the statically-linked
            # MXE runtime libs (libgcc / libstdc++ / libwinpthread shipped
            # by MXE with DWARF retained). A genuine `-g` build of the
            # CatchChallenger sources produces a .debug_info of tens to
            # hundreds of MiB. 1 MiB sits well above the runtime-static
            # floor and well below any plausible `-g`-compiled output, so
            # use it as the discriminator: > 1 MiB ⇒ compiled with -g.
            _DEBUG_INFO_BYTES_MAX = 1 * 1024 * 1024
            for p in parts:
                stage = p["stage"]
                if not stage or not os.path.isdir(stage):
                    continue
                for root_dir, _dirs, files in os.walk(stage):
                    for fn in files:
                        if not fn.lower().endswith(".exe"):
                            continue
                        fp = os.path.join(root_dir, fn)
                        try:
                            sz = os.path.getsize(fp)
                        except OSError:
                            continue
                        if sz >= 80 * 1024 * 1024:
                            oversized.append((fp, sz))
                        if _OBJDUMP:
                            try:
                                res = subprocess.run(
                                    [_OBJDUMP, "-h", fp],
                                    capture_output=True, text=True,
                                    timeout=30)
                            except (OSError, subprocess.SubprocessError):
                                continue
                            debug_info_size = 0
                            # objdump -h header line format:
                            #   "  <idx> <name> <size-hex> <vma> <lma> <fileoff> <align>"
                            for line in res.stdout.splitlines():
                                cols = line.split()
                                if len(cols) < 4:
                                    continue
                                if cols[1] == ".debug_info":
                                    try:
                                        debug_info_size = int(cols[2], 16)
                                    except ValueError:
                                        debug_info_size = 0
                                    break
                            if debug_info_size > _DEBUG_INFO_BYTES_MAX:
                                debug_syms.append((fp, debug_info_size))
            if oversized or debug_syms:
                details = []
                if oversized:
                    details.append("oversized .exe (>=80 MiB): " + "; ".join(
                        f"{os.path.relpath(fp, ROOT)} = {sz/(1024*1024):.1f} MiB"
                        for fp, sz in oversized))
                if debug_syms:
                    details.append("debug symbols (compiled with -g): " + "; ".join(
                        f"{os.path.relpath(fp, ROOT)} .debug_info={dsz/(1024*1024):.1f} MiB"
                        for fp, dsz in debug_syms))
                detail = " | ".join(details) + " — rebuild Release"
                log_fail("installer combined", detail)
                log_fail("msi combined", detail)
            else:
                combined_installer = None
                if should_run("installer combined", failed_cases):
                    combined_installer = build_combined_installer(parts)
                if combined_installer and combined_installer.endswith(".exe") \
                   and should_run("sign installer combined", failed_cases):
                    sign_authenticode(combined_installer, "combined", "installer")
                    cleanup_helpers.promote_artifact(combined_installer,
                                                     "catchchallenger-installer.exe")
                # Publish the freshly-built (and, when signing ran, signed)
                # combined installer to the world-readable drop the website's
                # upload pipeline picks up. Done after sign so the published
                # .exe carries the Authenticode blob. Best-effort: a missing
                # /mnt/data/world must not fail the build.
                if combined_installer and os.path.isfile(combined_installer):
                    world_installer = "/mnt/data/world/catchchallenger-installer.exe"
                    try:
                        os.makedirs(os.path.dirname(world_installer), exist_ok=True)
                        shutil.copy2(combined_installer, world_installer)
                        log_info(f"published installer -> {world_installer}")
                    except OSError as _world_exc:
                        log_info(f"WARNING: could not publish installer to "
                                 f"{world_installer}: {_world_exc}")
                if should_run("msi combined", failed_cases):
                    build_combined_msi(parts)
                # Installed-payload end-to-end. On a normal full run the
                # installer was just built above; on a --onlyfailed re-run
                # that queues only an e2e case (but not "installer
                # combined") combined_installer is still None — rebuild it
                # here so the 5 steps have something to install.
                if any(should_run(n, failed_cases) for n in _E2E_CASE_NAMES):
                    if combined_installer is None:
                        combined_installer = build_combined_installer(parts)
                    run_installed_payload_e2e(combined_installer,
                                              win_dp_src, win_mc)
                # Real Windows laptop (non-admin, user-level install).
                # build_combined_installer_user() reuses the same staged
                # payload so it adds only the NSIS pass (~seconds).
                if any(should_run(n, failed_cases) for n in _WIN_REAL_CASE_NAMES):
                    user_installer = build_combined_installer_user(parts)
                    run_on_real_windows(user_installer, win_dp_src)
    else:
        # Missing one of the three binaries — emit a single FAIL row
        # so failed.json reflects the gap and --onlyfailed can re-run
        # the offending compile.
        missing = []
        if cpu_exe is None:
            missing.append("qtcpu800x600")
        if gl_exe is None:
            missing.append("qtopengl")
        if srv_exe is None:
            missing.append("server-gui")
        if should_run("installer combined", failed_cases):
            log_fail("installer combined",
                     f"missing binaries: {','.join(missing)}")
        if should_run("msi combined", failed_cases):
            log_fail("msi combined",
                     f"missing binaries: {','.join(missing)}")
        # Keep the total test count fixed: the 5 installed-payload
        # steps still report (as FAIL) when a binary is missing rather
        # than vanishing from the run.
        for _n in _E2E_CASE_NAMES:
            if should_run(_n, failed_cases):
                log_fail(_n, f"missing binaries: {','.join(missing)}")
        for _n in _WIN_REAL_CASE_NAMES:
            if should_run(_n, failed_cases):
                log_fail(_n, f"missing binaries: {','.join(missing)}")

    win_need_multi = ((cpu_exe is not None and should_run("wine run qtcpu800x600", failed_cases)) or
                      (gl_exe  is not None and should_run("wine run qtopengl", failed_cases)))
    if win_need_multi:
        srv = None
        bin_path = os.path.join(SERVER_BUILD, SERVER_BIN_NAME)
        # The local server-filedb is normally produced by testingmulti
        # earlier in all.sh — but with --continue (or when running
        # this script in isolation) the build dir may not exist yet,
        # since all.sh wipes tmpfs each invocation. Build it on
        # demand here instead of failing; ccache makes this cheap.
        if not os.path.isfile(bin_path):
            log_info(f"server-filedb missing at {bin_path}; building")
            import cmake_helpers as _ch
            ensure_dir(SERVER_BUILD)
            pro = os.path.join(ROOT,
                               "server/cli/catchchallenger-server-filedb.pro")
            # testingcompilationwindows.py doesn't support sanitizer /
            # valgrind diag modes (cross-build with MXE has no clang/
            # gcc-debug toolchain wired up for them), so pass None for
            # diag — build_project uses its default linux-g++ spec.
            ok = _ch.build_project(
                pro, SERVER_BUILD,
                "server-filedb (testingcompilationwindows)",
                root=ROOT, nproc=str(multiprocessing.cpu_count()),
                log_info=log_info, log_pass=log_pass, log_fail=log_fail,
                ensure_dir=ensure_dir, run_cmd=run_cmd,
            )
            if not ok:
                log_fail("wine run: server start",
                         f"failed to build {SERVER_BIN_NAME}")
        if os.path.isfile(bin_path):
            # Stage datapack + a minimal server-properties.xml next to
            # the binary. testingmulti normally does this earlier in
            # all.sh, but with --continue testingmulti may have been
            # cached-skipped, leaving an empty build dir. Without a
            # datapack the server prints "No datapack found into:
            # ./datapack/" and never binds — the harness then waits
            # the full SERVER_READY_TIMEOUT for nothing.
            try:
                import datapack_stage as _ds
                src_dp = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
                if not os.path.isdir(src_dp):
                    src_dp = DATAPACK_SRC if "DATAPACK_SRC" in globals() else None
                if src_dp and os.path.isdir(src_dp):
                    staged = _ds.staged_local(src_dp)
                    # stage_datapacks.py stages once at all.sh startup, but
                    # when this script runs in isolation (or the tmpfs cache
                    # was wiped) the staged dir is missing and the symlink
                    # would dangle — the server then can't find ./datapack/,
                    # never binds, and burns the full SERVER_READY_TIMEOUT.
                    # Stage on demand (local only); fall back to the source.
                    if not (os.path.isdir(staged) and os.listdir(staged)):
                        log_info(f"staged datapack missing at {staged}; "
                                 "staging on demand")
                        _ds.stage_all([src_dp], exec_nodes=None,
                                      log_info=log_info)
                    target_dp = staged if (os.path.isdir(staged)
                                           and os.listdir(staged)) else src_dp
                    dst_dp = os.path.join(SERVER_BUILD, "datapack")
                    if os.path.islink(dst_dp) or os.path.isfile(dst_dp):
                        os.remove(dst_dp)
                    elif os.path.isdir(dst_dp):
                        shutil.rmtree(dst_dp)
                    os.symlink(target_dp, dst_dp)
                    log_info(f"staged datapack {dst_dp} -> {target_dp}")
            except Exception as e:
                log_info(f"datapack stage non-fatal: {e}")
            # Pin server-port + mainDatapackCode so the wine clients
            # have a stable target. server-port=61917 matches the
            # SERVER_PORT constant used by run_wine_client.
            xml_path = os.path.join(SERVER_BUILD, "server-properties.xml")
            with open(xml_path, "w") as _xf:
                _xf.write(
                    '<?xml version="1.0"?>\n'
                    '<configuration>\n'
                    f'    <server-port value="{SERVER_PORT}"/>\n'
                    '    <server-ip value=""/>\n'
                    '    <httpDatapackMirror value=""/>\n'
                    '    <max-players value="10"/>\n'
                    '    <pvp value="true"/>\n'
                    # `--autologin` makes the client tryLogin with a
                    # synthetic account; without auto-create the
                    # backend rejects with "Bad login" and the wine
                    # run aborts at parseReplyData(packetCode=127,
                    # data=02). Mirror what testingmulti seeds.
                    '    <automatic_account_creation value="true"/>\n'
                    '    <content>\n'
                    '        <mainDatapackCode value="official"/>\n'
                    '        <subDatapackCode value=""/>\n'
                    '    </content>\n'
                    '</configuration>\n'
                )
            srv = start_local_server(SERVER_BUILD)
        elif not any(name == "wine run: server start" for name, *_ in results):
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
