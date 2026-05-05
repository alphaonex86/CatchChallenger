#!/usr/bin/env python3
"""
testingcompilationandroid.py — CatchChallenger Android cross-compile + run test.

Steps:
  1. Probe local Android tooling under CC_ANDROID_HOME (sdk, emulator, AVD,
     Qt-for-Android with companion gcc_64 host qmake).
  2. qmake-android + make + make install + androiddeployqt for qtopengl
     (qtcpu800x600 is the fixed-800x600 widget UI; not suitable for phones).
  3. Boot the first AVD found, run --autosolo (no server) and the multi-mode
     run against the local server-filedb (which testingclient.py /
     testingserver.py already built).

Self-skips when CC_ANDROID_HOME isn't ready. All Android local tooling lives
under <paths.android_workspace> (see test_config.py / config.json) — this
script never touches anything outside that workspace and never installs
packages on the host.

Sibling of testingcompilationmac.py — both used to live inside testingclient.py.
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re, time, glob
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

CLIENT_GL_PRO = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")

#local server-filedb that the multi-mode emulator client connects back to.
SERVER_BUILD    = build_paths.build_path("server/epoll/build/testing-filedb")
SERVER_BIN_NAME = "catchchallenger-server-cli-epoll"

# Android emulator's NAT maps 10.0.2.2 to the HOST's loopback
# (https://developer.android.com/studio/run/emulator-networking).
# 192.168.158.10 is the host's LAN IP and is NOT reachable from the
# emulator over the default user-mode networking — use 10.0.2.2 instead
# so the multi-mode client can talk to the local server-filedb that
# testingcompilationandroid.py spins up on 0.0.0.0:port.
SERVER_HOST_REMOTE = "10.0.2.2"
SERVER_PORT        = str(_config["server_port"])

# ── Android workspace ──────────────────────────────────────────────────────
import test_config as _tc
CC_ANDROID_HOME       = os.environ.get("CC_ANDROID_HOME",
                                       _tc.ANDROID_WORKSPACE)
ANDROID_SDK           = os.path.join(CC_ANDROID_HOME, "sdk")
ANDROID_AVD_HOME      = os.path.join(CC_ANDROID_HOME, "avd")
ANDROID_USER_HOME_DIR = os.path.join(CC_ANDROID_HOME, "user")
ANDROID_APK_DIR       = os.path.join(CC_ANDROID_HOME, "apk")
# Android App Bundle (Google Play distribution format) — built alongside
# the APK in build_android_apk(). Same install_root produces both.
ANDROID_AAB_DIR       = os.path.join(CC_ANDROID_HOME, "aab")
ANDROID_BUILD_DIR     = os.path.join(CC_ANDROID_HOME, "build")
ANDROID_QT_DIR        = os.path.join(CC_ANDROID_HOME, "qt")
ANDROID_ADB           = os.path.join(ANDROID_SDK, "platform-tools", "adb")
ANDROID_EMULATOR_BIN  = os.path.join(ANDROID_SDK, "emulator", "emulator")
ANDROID_QT_ABI        = "android_arm64_v8a"
ANDROID_NATIVE_API    = "34"
ANDROID_NDK_HOST      = "linux-x86_64"
ANDROID_BUILD_TOOLS   = "34.0.0"
ANDROID_BOOT_TIMEOUT  = 240
ANDROID_RUN_TIMEOUT   = 120
# Package id from the Qt6 androiddeployqt build. The default Qt-for-Android
# template names the package `org.qtproject.example.<target>` based on the
# CMake target name (`catchchallenger` here). Verified via
# `aapt dump badging qtopengl.apk` — using `org.catchchallenger.android`
# previously made `am start` silently launch the WRONG activity (a
# non-existent package on a freshly-flashed AVD), so logcat showed only
# system noise and the success_marker never landed.
ANDROID_PACKAGE       = "org.qtproject.example.catchchallenger"
ANDROID_ACTIVITY      = "org.qtproject.qt.android.bindings.QtActivity"
COMPILE_TIMEOUT       = 600
SERVER_READY_TIMEOUT  = 60

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


def find_android_qmake():
    """Locate the Qt-for-Android qmake6 inside CC_ANDROID_HOME.

    The android qmake is a thin shell wrapper that re-execs the host
    (gcc_64) qmake under the same Qt version; if that host qmake is
    missing the wrapper fails with rc=127 at first call. Require the
    host qmake here so the phase self-skips cleanly."""
    qt_root = ANDROID_QT_DIR
    if not os.path.isdir(qt_root):
        return None
    versions = []
    try:
        entries = os.listdir(qt_root)
    except OSError:
        return None
    idx = 0
    while idx < len(entries):
        if os.path.isdir(os.path.join(qt_root, entries[idx])):
            versions.append(entries[idx])
        idx += 1
    versions.sort(reverse=True)
    idx = 0
    while idx < len(versions):
        ver_dir = os.path.join(qt_root, versions[idx])
        host_qmake_6 = os.path.join(ver_dir, "gcc_64", "bin", "qmake6")
        host_qmake = os.path.join(ver_dir, "gcc_64", "bin", "qmake")
        if not os.path.isfile(host_qmake_6) and not os.path.isfile(host_qmake):
            idx += 1
            continue
        cand = os.path.join(ver_dir, ANDROID_QT_ABI, "bin", "qmake6")
        if os.path.isfile(cand):
            return cand
        cand = os.path.join(ver_dir, ANDROID_QT_ABI, "bin", "qmake")
        if os.path.isfile(cand):
            return cand
        idx += 1
    return None


def find_android_avd():
    """Return the first AVD name under ANDROID_AVD_HOME, or None."""
    if not os.path.isdir(ANDROID_AVD_HOME):
        return None
    try:
        entries = os.listdir(ANDROID_AVD_HOME)
    except OSError:
        return None
    idx = 0
    while idx < len(entries):
        e = entries[idx]
        idx += 1
        if e.endswith(".ini"):
            return e[:-4]
    return None


def android_local_ready():
    """True iff all local Android tooling is present: adb, emulator, an AVD,
    and a Qt-for-Android qmake.  When False, the phase self-skips."""
    if not os.path.isdir(CC_ANDROID_HOME):
        return False
    missing = []
    if not os.path.isfile(ANDROID_ADB):
        missing.append(f"adb ({ANDROID_ADB})")
    if not os.path.isfile(ANDROID_EMULATOR_BIN):
        missing.append(f"emulator ({ANDROID_EMULATOR_BIN})")
    if find_android_qmake() is None:
        missing.append(f"Qt-for-Android qmake under {CC_ANDROID_HOME}/qt/<version>/{ANDROID_QT_ABI}/bin/")
    if find_android_avd() is None:
        missing.append(f"at least one AVD under {ANDROID_AVD_HOME}")
    if missing:
        log_info("Android local tooling missing — phase will skip:")
        idx = 0
        while idx < len(missing):
            log_info(f"  - {missing[idx]}")
            idx += 1
        return False
    return True


def android_env():
    """Build a fresh, self-contained env for adb / emulator / qmake-android /
    androiddeployqt. We deliberately do NOT inherit ANDROID_*, QT_*,
    JAVA_HOME, LD_LIBRARY_PATH, PYTHONPATH from the parent — those may carry
    conflicting paths from the user's shell. Only minimal locale/identity
    vars are forwarded."""
    env = {}
    forward = ("HOME", "USER", "LOGNAME", "LANG", "LC_ALL", "LC_CTYPE",
               "TERM", "TMPDIR", "TZ", "DISPLAY", "XAUTHORITY",
               "SHELL", "MAIL", "PWD")
    fi = 0
    while fi < len(forward):
        v = os.environ.get(forward[fi])
        if v is not None:
            env[forward[fi]] = v
        fi += 1
    env["CC_ANDROID_HOME"]          = CC_ANDROID_HOME
    env["ANDROID_HOME"]             = ANDROID_SDK
    env["ANDROID_SDK_ROOT"]         = ANDROID_SDK
    env["ANDROID_AVD_HOME"]         = ANDROID_AVD_HOME
    env["ANDROID_USER_HOME"]        = ANDROID_USER_HOME_DIR
    env["ANDROID_NATIVE_API_LEVEL"] = ANDROID_NATIVE_API
    env["ANDROID_NDK_HOST"]         = ANDROID_NDK_HOST
    env["ANDROID_NDK_PLATFORM"]     = "android-" + ANDROID_NATIVE_API
    env["ANDROID_SDK_BUILD_TOOLS"]  = ANDROID_BUILD_TOOLS
    env["QT_ANDROID_BUILD_ALL_ABIS"] = "FALSE"
    env["CMAKE_BUILD_TYPE"]         = "Debug"

    # Gradle needs a JDK (with javac), not a JRE. The host's default
    # java often points at a JRE-only install (gentoo's
    # /opt/openjdk-jre-bin-21 ships no javac, gradle aborts with
    # "Toolchain installation does not provide JAVA_COMPILER"). Probe
    # for a JDK with javac and pin JAVA_HOME / ORG_GRADLE_JAVA_HOME so
    # gradle's auto-toolchain detection picks the right one.
    jdk_candidates = []
    import glob as _glob
    for pat in ("/opt/openjdk-bin-*", "/usr/lib/jvm/java-*-openjdk*",
                "/usr/lib/jvm/openjdk-*", "/usr/lib/jvm/jdk-*",
                "/opt/jdk-*"):
        jdk_candidates.extend(sorted(_glob.glob(pat), reverse=True))
    java_home = None
    ji = 0
    while ji < len(jdk_candidates):
        cand = jdk_candidates[ji]
        ji += 1
        if os.path.isfile(os.path.join(cand, "bin", "javac")):
            java_home = cand
            break
    if java_home is not None:
        env["JAVA_HOME"]            = java_home
        env["GRADLE_JAVA_HOME"]     = java_home
        env["ORG_GRADLE_JAVA_HOME"] = java_home

    qt_root = ANDROID_QT_DIR
    qt_host_bin = None
    if os.path.isdir(qt_root):
        try:
            versions = sorted([d for d in os.listdir(qt_root)
                               if os.path.isdir(os.path.join(qt_root, d))],
                              reverse=True)
        except OSError:
            versions = []
        vi = 0
        while vi < len(versions):
            cand = os.path.join(qt_root, versions[vi], ANDROID_QT_ABI)
            if os.path.isdir(cand):
                env["QT_ANDROID"] = cand
                host_cand = os.path.join(qt_root, versions[vi], "gcc_64")
                if os.path.isdir(host_cand):
                    env["QT_HOST_PATH"] = host_cand
                    qt_host_bin = os.path.join(host_cand, "bin")
                break
            vi += 1

    ndk_root = os.path.join(ANDROID_SDK, "ndk")
    if os.path.isdir(ndk_root):
        try:
            ndks = sorted(os.listdir(ndk_root), reverse=True)
        except OSError:
            ndks = []
        ni = 0
        while ni < len(ndks):
            cand = os.path.join(ndk_root, ndks[ni])
            if os.path.isdir(cand):
                env["ANDROID_NDK_ROOT"] = cand
                env["ANDROID_NDK_HOME"] = cand
                break
            ni += 1

    path_parts = []
    # JDK first so `java -version` resolves to the JDK gradle expects,
    # not the host's default JRE-only install.
    if "JAVA_HOME" in env:
        path_parts.append(os.path.join(env["JAVA_HOME"], "bin"))
    if "QT_ANDROID" in env:
        path_parts.append(os.path.join(env["QT_ANDROID"], "bin"))
    if qt_host_bin is not None:
        path_parts.append(qt_host_bin)
    path_parts.append(os.path.join(ANDROID_SDK, "platform-tools"))
    path_parts.append(os.path.join(ANDROID_SDK, "emulator"))
    path_parts.append(os.path.join(ANDROID_SDK, "cmdline-tools", "latest", "bin"))
    bt_dir = os.path.join(ANDROID_SDK, "build-tools", ANDROID_BUILD_TOOLS)
    if os.path.isdir(bt_dir):
        path_parts.append(bt_dir)
    path_parts += ["/usr/local/sbin", "/usr/local/bin",
                   "/usr/sbin", "/usr/bin", "/sbin", "/bin"]
    env["PATH"] = ":".join(path_parts)
    return env


def build_android_apk(pro_file, build_dir, label):
    """Phase 5 (qmake -> CMake) Android build path:
       qt-cmake (Android toolchain) configure
        + cmake --build
        + cmake --install
        + androiddeployqt
    Returns the local path to the produced .apk, or None on failure."""
    import cmake_helpers as _ch
    name = f"compile {label} (android-{ANDROID_QT_ABI})"
    qmake = find_android_qmake()  # still useful as a "Qt-for-Android present" probe
    qt_bin_dir = os.path.dirname(qmake)
    qt_cmake = os.path.join(qt_bin_dir, "qt-cmake")
    host_bin = os.path.join(os.path.dirname(os.path.dirname(qt_bin_dir)),
                            "gcc_64", "bin")
    androiddeployqt = None
    cands = [os.path.join(host_bin, "androiddeployqt"),
             os.path.join(qt_bin_dir, "androiddeployqt")]
    ci = 0
    while ci < len(cands):
        if os.path.isfile(cands[ci]):
            androiddeployqt = cands[ci]
            break
        ci += 1
    if androiddeployqt is None:
        log_fail(name, f"androiddeployqt not found (looked in {host_bin} and {qt_bin_dir})")
        return None
    if not os.path.isfile(qt_cmake):
        log_fail(name, f"qt-cmake not found at {qt_cmake}")
        return None
    pro_rel = os.path.relpath(pro_file, ROOT).replace(os.sep, "/")
    try:
        target, configure_flags, source_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return None
    cmake_source = os.path.join(ROOT, source_subdir)
    # The source dir flipped during the qmake → CMake refactor (was the
    # repo root, now points at the binary's own subdir CMakeLists.txt).
    # CMake refuses to reuse a build dir whose CMakeCache.txt records a
    # different CMAKE_HOME_DIRECTORY, so detect that case and wipe the
    # stale cache rather than fail with the cryptic
    # "source ... does not match the source used to generate cache".
    cache_path = os.path.join(build_dir, "CMakeCache.txt")
    if os.path.isfile(cache_path):
        try:
            with open(cache_path, "r") as _cf:
                _cache_txt = _cf.read()
        except (OSError, UnicodeDecodeError):
            _cache_txt = ""
        if (f"CMAKE_HOME_DIRECTORY:INTERNAL={cmake_source}" not in _cache_txt
                and "CMAKE_HOME_DIRECTORY:INTERNAL=" in _cache_txt):
            shutil.rmtree(build_dir, ignore_errors=True)
    ensure_dir(build_dir)
    env = android_env()

    log_info(f"android qt-cmake configure {label}")
    args = [qt_cmake, "-S", cmake_source, "-B", build_dir,
            "-DCMAKE_BUILD_TYPE=Debug",
            # -fno-lto everywhere on test builds: keeps the link path
            # predictable and avoids lto-wrapper fanout (see cmake_helpers.py).
            "-DCMAKE_C_FLAGS_INIT=-fno-lto",
            "-DCMAKE_CXX_FLAGS_INIT=-fno-lto",
            "-DCMAKE_EXE_LINKER_FLAGS=-fno-lto",
            "-DCMAKE_SHARED_LINKER_FLAGS=-fno-lto",
            "-DCMAKE_MODULE_LINKER_FLAGS=-fno-lto",
            "-DCATCHCHALLENGER_NOAUDIO=ON",
            "-DCATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS=OFF",
            "-DCATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS=OFF",
            # Embed the in-process server so --autosolo can launch
            # without an external server binary. Same rationale as the
            # MXE pass; the O_CLOEXEC #ifndef shim in ClientHeavyLoadMirror.cpp
            # plus the <algorithm> includes for std::sort are what let
            # this compile cleanly for Android Qt.
            "-DCATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON"]
    args.extend(configure_flags)
    rc, out = run_cmd(args, build_dir, env=env)
    if rc != 0:
        log_fail(name, f"qt-cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_info(f"android cmake --build -j{NPROC} {label}")
    rc, out = run_cmd(["cmake", "--build", build_dir,
                       "--target", target, "-j", NPROC],
                      build_dir, env=env)
    if rc != 0:
        log_fail(name, f"cmake build failed (rc={rc})")
        if out.strip():
            print(out[-8000:])
        return None
    install_root = os.path.join(build_dir, "android-build")
    log_info(f"android cmake --install --prefix={install_root}")
    rc, out = run_cmd(["cmake", "--install", build_dir,
                       "--prefix", install_root],
                      build_dir, env=env)
    if rc != 0:
        log_fail(name, f"cmake install failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    deploy_json = os.path.join(build_dir, "android-" + target + "-deployment-settings.json")
    if not os.path.isfile(deploy_json):
        matches = glob.glob(os.path.join(build_dir, "**",
                                         "android-*-deployment-settings.json"),
                            recursive=True)
        if matches:
            deploy_json = matches[0]
    if not os.path.isfile(deploy_json):
        log_fail(name, f"deployment-settings.json not produced under {build_dir}")
        return None
    # androiddeployqt is mode-flagged: `--apk` invokes gradle's
    # assembleDebug task, `--aab` invokes bundleDebug. The flags don't
    # combine cleanly across all Qt 6.x point releases (some versions
    # silently drop the bundle when both are passed), so run the tool
    # twice against the same install_root — gradle reuses the staged
    # artefacts so the second pass is fast.
    # APK goes to apk/ for sideloading + emulator testing; AAB goes to
    # aab/ for Google Play distribution. Both outputs are mandatory —
    # failing to produce either surfaces real packaging regressions
    # instead of half-passing.
    log_info(f"androiddeployqt --apk --output {install_root}")
    rc, out = run_cmd([androiddeployqt, "--input", deploy_json,
                       "--output", install_root, "--apk", "--gradle"],
                      build_dir, env=env)
    if rc != 0:
        log_fail(name, f"androiddeployqt --apk failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    apk_candidates = glob.glob(os.path.join(install_root, "build", "outputs",
                                            "apk", "**", "*.apk"),
                               recursive=True)
    if not apk_candidates:
        log_fail(name, f".apk not produced under {install_root}")
        return None
    apk_src = apk_candidates[0]
    ensure_dir(ANDROID_APK_DIR)
    apk_dst_path = os.path.join(ANDROID_APK_DIR, label + ".apk")
    shutil.copy2(apk_src, apk_dst_path)
    log_info(f"androiddeployqt --aab --output {install_root}")
    rc, out = run_cmd([androiddeployqt, "--input", deploy_json,
                       "--output", install_root, "--aab", "--gradle"],
                      build_dir, env=env)
    if rc != 0:
        log_fail(name, f"androiddeployqt --aab failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    aab_candidates = glob.glob(os.path.join(install_root, "build", "outputs",
                                            "bundle", "**", "*.aab"),
                               recursive=True)
    if not aab_candidates:
        log_fail(name, f".aab not produced under {install_root}")
        return None
    aab_src = aab_candidates[0]
    ensure_dir(ANDROID_AAB_DIR)
    aab_dst_path = os.path.join(ANDROID_AAB_DIR, label + ".aab")
    shutil.copy2(aab_src, aab_dst_path)
    log_pass(name, f"-> {os.path.relpath(apk_dst_path, ROOT)} + "
                   f"{os.path.relpath(aab_dst_path, ROOT)}")
    return apk_dst_path


def adb_cmd(args, timeout=60):
    timeout = clamp_local(timeout)
    full = [ANDROID_ADB] + list(args)
    try:
        p = subprocess.run(full, timeout=timeout, env=android_env(),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def start_android_emulator():
    avd_name = find_android_avd()
    if avd_name is None:
        log_fail("android emulator start", "no AVD configured")
        return None
    log_info(f"starting emulator: {ANDROID_EMULATOR_BIN} -avd {avd_name}")
    env = android_env()
    emu_args = [ANDROID_EMULATOR_BIN, "-avd", avd_name,
                "-no-window", "-no-audio", "-no-snapshot",
                "-gpu", "swiftshader_indirect"]
    diagnostic.record_cmd(emu_args, None)
    proc = subprocess.Popen(
        emu_args,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env,
        preexec_fn=os.setsid)
    rc, _ = adb_cmd(["wait-for-device"], timeout=ANDROID_BOOT_TIMEOUT)
    if rc != 0:
        log_fail("android emulator start", "adb wait-for-device failed")
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        return None
    deadline = time.monotonic() + ANDROID_BOOT_TIMEOUT
    while time.monotonic() < deadline:
        rc, out = adb_cmd(["shell", "getprop", "sys.boot_completed"], timeout=10)
        if rc == 0 and out.strip() == "1":
            log_pass("android emulator start", f"booted AVD={avd_name}")
            return proc
        time.sleep(2.0)
    log_fail("android emulator start", f"timeout {ANDROID_BOOT_TIMEOUT}s waiting for sys.boot_completed")
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    return None


def stop_android_emulator(proc):
    if proc is None:
        return
    log_info("stopping android emulator")
    adb_cmd(["emu", "kill"], timeout=10)
    try:
        proc.wait(timeout=15)
    except subprocess.TimeoutExpired:
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


def run_android_apk(apk_path, label, args=None, timeout=ANDROID_RUN_TIMEOUT,
                    success_marker="MapVisualiserPlayer::mapDisplayedSlot()"):
    name = f"android run {label}"
    log_info(f"adb install -r {apk_path}")
    rc, out = adb_cmd(["install", "-r", apk_path], timeout=120)
    if rc != 0:
        log_fail(name, f"adb install failed (rc={rc})")
        if out.strip():
            print(out[-1500:])
        return False
    adb_cmd(["logcat", "-c"], timeout=10)
    am_args = ["shell", "am", "start", "-W", "-n",
               f"{ANDROID_PACKAGE}/{ANDROID_ACTIVITY}"]
    if args:
        # `adb shell` re-joins argv with spaces and re-tokenises on the
        # device-side /system/bin/sh, so multi-word values to `am --es
        # KEY VALUE` get split unless we shell-quote on the device.
        # Wrap the joined VALUE in single quotes so the device shell
        # sees one token. Without this, args like
        # "--autosolo --closewhenonmap" arrive at `am start` as two
        # extra positional flags and `am` rejects "--closewhenonmap" as
        # "Unknown option".
        joined = " ".join(args).replace("'", "'\\''")
        am_args += ["--es", "applicationArguments", f"'{joined}'"]
    log_info(f"am start {ANDROID_PACKAGE}/{ANDROID_ACTIVITY} {' '.join(args) if args else ''}")
    rc, out = adb_cmd(am_args, timeout=30)
    if rc != 0:
        log_fail(name, f"am start failed (rc={rc})")
        if out.strip():
            print(out[-1500:])
        return False
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        rc, out = adb_cmd(["logcat", "-d"], timeout=10)
        if rc == 0 and success_marker in out:
            log_pass(name, f"reached map ({success_marker})")
            adb_cmd(["shell", "am", "force-stop", ANDROID_PACKAGE], timeout=10)
            return True
        time.sleep(2.0)
    log_fail(name, f"timeout {timeout}s without success_marker")
    rc, out = adb_cmd(["logcat", "-d", "-t", "200"], timeout=10)
    if rc == 0:
        lines = out.splitlines()
        li = max(0, len(lines) - 30)
        while li < len(lines):
            print(f"  | {lines[li]}")
            li += 1
    adb_cmd(["shell", "am", "force-stop", ANDROID_PACKAGE], timeout=10)
    return False


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
    print("  CatchChallenger — Android cross-compile testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    if not android_local_ready():
        log_info(f"android local tooling under {CC_ANDROID_HOME} not ready — skipping android test")
        save_failed_cases()
        summary()
        return

    gl_apk = None
    if should_run("compile qtopengl (android-" + ANDROID_QT_ABI + ")", failed_cases):
        gl_apk = build_android_apk(CLIENT_GL_PRO,
                                   os.path.join(ANDROID_BUILD_DIR, "qtopengl"),
                                   "qtopengl")

    android_need_solo  = (gl_apk is not None and should_run("android run qtopengl --autosolo", failed_cases))
    android_need_multi = (gl_apk is not None and should_run("android run qtopengl", failed_cases))
    if android_need_solo or android_need_multi:
        emu = start_android_emulator()
        if emu is not None:
            if android_need_solo:
                run_android_apk(gl_apk, "qtopengl --autosolo",
                                args=["--autosolo", "--closewhenonmap"])
            if android_need_multi:
                srv = None
                if os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN_NAME)):
                    set_http_datapack_mirror(SERVER_BUILD, "")
                    srv = start_local_server(SERVER_BUILD)
                else:
                    log_fail("android run: server start",
                             f"{SERVER_BIN_NAME} not found in {SERVER_BUILD}")
                if srv is not None:
                    run_android_apk(gl_apk, "qtopengl",
                                    args=["--host", SERVER_HOST_REMOTE,
                                          "--port", SERVER_PORT,
                                          "--autologin", "--character", "AndroidGL",
                                          "--closewhenonmap"])
                    stop_local_server()
            stop_android_emulator(emu)

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
