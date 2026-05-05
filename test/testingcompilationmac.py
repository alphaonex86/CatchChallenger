#!/usr/bin/env python3
"""
testingcompilationmac.py — CatchChallenger macOS cross-compile + package
test, driven from a Linux osxcross host.

Switched from the qemu mac VM (192.168.158.34) to an osxcross container at
root@2803:1920::2:ff08 (see operator notes for setup details). The host
ships:
  - osxcross (clang 19.x, target darwin20.4 / macOS 11+)
  - Qt 6.5.3 for macOS at /root/qt6-macos/6.5.3/macos
  - macports deps (libogg, libopus, opusfile, libvorbis, zstd, lz4,
    openssl, zlib)
  - ninja, lld, mold, ccache

Build pipeline (per --target arch, default x86_64):
  1. Probe ssh to the osxcross host.
  2. rsync sources to /root/catchchallenger-test/.
  3. <arch>-apple-darwin20.4-cmake -G Ninja
       -DCMAKE_PREFIX_PATH=/root/qt6-macos/6.5.3/macos
       -DCMAKE_MACOSX_BUNDLE=ON       (so qtcpu800x600 also builds a .app)
       -DCMAKE_BUILD_TYPE=Release
       -DCMAKE_C_COMPILER_LAUNCHER=ccache
       -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
       -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld
     + the per-target -D switches from cmake_helpers.pro_to_cmake_target.
  4. cmake --build (ninja) -j$(nproc).
  5. Stage CatchChallenger-datapack/ next to the .app
     (e.g. /root/.../catchchallenger.app  ->
            /root/.../datapack/internal/).
  6. Run Qt's macdeployqt on the .app to bundle frameworks + plugins.
  7. ad-hoc codesign with osxcross's `<arch>-apple-darwin20.4-codesign`
     when present (osxcross 13+ ships rcodesign-style fakesign tooling
     under that name) — otherwise note that real Apple Developer ID
     codesign + notarize requires a macOS host (xcrun notarytool).
  8. Package: try macdeployqt -dmg first (needs hdiutil — only present
     on macOS, so it usually falls through on Linux), else fall back to
     a portable .zip of the .app + datapack pair, which is the deliverable
     for users who'll mount the artefact on a real Mac.

This is a compile + package test: macOS Mach-O binaries cannot be
executed on the Linux host, so there is no run / multi-mode phase. If
you need to verify that the produced .app actually launches, copy the
.dmg or .zip onto a real macOS box and run it there.

The optimisation knobs (ccache + ninja + lld) are explicit per the
user's request. mold has no Mach-O backend (its mac support project
"sold" is unmaintained), so cross-link uses lld; ccache transparently
caches every .o across runs in the host's default cache dir.
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths
# never lands a __pycache__/ dir in the source tree.  Set before the
# first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, signal, subprocess, json, time, shlex
import build_paths
import diagnostic
from cmd_helpers import (SSH_OPTS_LIST, RSYNC_SSH_E, SSH_TIMEOUT_MARKER,
                         is_ssh_timeout, clamp_ssh, clamp_local)

build_paths.ensure_root()

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT          = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATAPACKS     = _config["paths"]["datapacks"]

CLIENT_CPU_PRO = "client/qtcpu800x600/qtcpu800x600.pro"
CLIENT_GL_PRO  = "client/qtopengl/catchchallenger-qtopengl.pro"

# ── osxcross host (ssh) ────────────────────────────────────────────────────
OSX_HOST          = "2803:1920::2:ff08"
OSX_USER          = "root"
OSX_QT            = "/root/qt6-macos/6.5.3/macos"
OSX_MACDEPLOYQT   = OSX_QT + "/bin/macdeployqt"
OSX_TARGET_TRIPLE = "x86_64-apple-darwin20.4"   # arch wrapper prefix
OSX_CMAKE         = f"/root/osxcross/target/bin/{OSX_TARGET_TRIPLE}-cmake"
OSX_WORK_DIR      = "/root/catchchallenger-test"
OSX_DATAPACK_DIR  = "/root/catchchallenger-datapack"   # rsynced once, reused
OSX_ARTIFACT_DIR  = "/root/catchchallenger-mac-artifacts"
SSH_PROBE_TIMEOUT = 10
RSYNC_TIMEOUT     = 900
COMPILE_TIMEOUT   = 1800
DEPLOY_TIMEOUT    = 600
APP_NAME          = "catchchallenger.app"

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN = "\033[92m"
C_RED   = "\033[91m"
C_CYAN  = "\033[96m"
C_RESET = "\033[0m"

results = []
_last_log_time = [time.monotonic()]
total_expected = [0]

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


# ── ssh wrappers ────────────────────────────────────────────────────────────
def osx_ssh(cmd, timeout=COMPILE_TIMEOUT):
    """Run a remote shell command on the osxcross host. The remote env
    pulls in osxcross + Qt by sourcing /root/setup-macos-build.sh first
    so o64-clang / xcrun-style tools and Qt cmake config are visible."""
    timeout = clamp_ssh(timeout)
    wrapped = "source /root/setup-macos-build.sh >/dev/null 2>&1 ; " + cmd
    args = ["ssh"] + SSH_OPTS_LIST + [f"{OSX_USER}@{OSX_HOST}", wrapped]
    diagnostic.record_cmd(args, None)
    try:
        p = subprocess.run(args, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"{SSH_TIMEOUT_MARKER}: {timeout}s"


def osx_host_reachable():
    rc, out = osx_ssh("echo lVt75gJ4sJXjq2gWxzXd8pV8",
                      timeout=SSH_PROBE_TIMEOUT)
    return rc == 0 and "lVt75gJ4sJXjq2gWxzXd8pV8" in out


def rsync_to_osx(src, dst):
    args = ["rsync", "-art", "--delete",
            "-e", RSYNC_SSH_E,
            "--exclude=build/", "--exclude=build-*/", "--exclude=.git/",
            src + "/", f"{OSX_USER}@{OSX_HOST}:{dst}/"]
    diagnostic.record_cmd(args, None)
    timeout = clamp_local(RSYNC_TIMEOUT)
    try:
        p = subprocess.run(args, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"TIMEOUT after {timeout}s"
    if p.returncode != 0:
        return False, f"rc={p.returncode}\n" + p.stdout.decode(errors="replace")[-1500:]
    return True, ""


def rsync_sources():
    name = "rsync sources to osxcross"
    log_info(f"{name}: {ROOT} -> {OSX_USER}@{OSX_HOST}:{OSX_WORK_DIR}")
    osx_ssh(f"mkdir -p {OSX_WORK_DIR}", timeout=15)
    ok, err = rsync_to_osx(ROOT, OSX_WORK_DIR)
    if ok:
        log_pass(name)
        return True
    log_fail(name, err.splitlines()[0] if err else "")
    print(err[-1500:])
    return False


def rsync_datapack(src):
    name = "rsync datapack to osxcross"
    log_info(f"{name}: {src} -> {OSX_USER}@{OSX_HOST}:{OSX_DATAPACK_DIR}")
    osx_ssh(f"mkdir -p {OSX_DATAPACK_DIR}", timeout=15)
    ok, err = rsync_to_osx(src, OSX_DATAPACK_DIR)
    if ok:
        log_pass(name)
        return True
    log_fail(name, err.splitlines()[0] if err else "")
    print(err[-1500:])
    return False


# ── build ───────────────────────────────────────────────────────────────────
def build_target(pro_rel, label):
    """cmake configure + ninja build via osxcross. Returns the remote
    .app path on success, None on failure."""
    import cmake_helpers as _ch
    name = f"compile {label} (mac, osxcross)"
    try:
        target, configure_flags, source_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return None
    build_dir = f"{OSX_WORK_DIR}/build-mac-{OSX_TARGET_TRIPLE}/{target}"
    # Per "one binary per CMakeLists.txt" refactor: -S points at the
    # binary's specific subdir, not the repo root.
    src_dir   = f"{OSX_WORK_DIR}/{source_subdir}"
    flag_args = " ".join(shlex.quote(f) for f in configure_flags)
    log_info(f"osxcross cmake configure {label} ({OSX_TARGET_TRIPLE})")
    rc, out = osx_ssh(
        f"mkdir -p {build_dir} && "
        f"{OSX_CMAKE} -S {src_dir} -B {build_dir} "
        f"-G Ninja "
        f"-DCMAKE_BUILD_TYPE=Release "
        f"-DCMAKE_CXX_STANDARD=17 "
        f"-DCMAKE_PREFIX_PATH={OSX_QT} "
        f"-DCMAKE_MACOSX_BUNDLE=ON "
        f"-DCMAKE_C_COMPILER_LAUNCHER=ccache "
        f"-DCMAKE_CXX_COMPILER_LAUNCHER=ccache "
        # -fno-lto on test builds (see cmake_helpers.py for the rationale).
        f"-DCMAKE_C_FLAGS_INIT=-fno-lto "
        f"-DCMAKE_CXX_FLAGS_INIT=-fno-lto "
        f"-DCMAKE_EXE_LINKER_FLAGS='-fuse-ld=lld -fno-lto' "
        f"-DCMAKE_SHARED_LINKER_FLAGS='-fuse-ld=lld -fno-lto' "
        f"-DCMAKE_MODULE_LINKER_FLAGS='-fuse-ld=lld -fno-lto' "
        f"-DCATCHCHALLENGER_NO_WEBSOCKET=ON "
        f"-DCATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS=OFF "
        f"-DCATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS=OFF "
        f"{flag_args} 2>&1",
        timeout=COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return None
    log_info(f"ninja build {label} -j$(nproc)")
    rc, out = osx_ssh(
        f"{OSX_CMAKE} --build {build_dir} --target {target} -j$(nproc) 2>&1",
        timeout=COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"ninja build failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return None
    # Self-contained per-binary CMakeLists.txt: the .app lands at
    # <build_dir>/<APP_NAME> directly (or some *.app under build_dir).
    rc, out = osx_ssh(
        f"ls -d {build_dir}/{APP_NAME} 2>/dev/null || "
        f"ls -d {build_dir}/*.app 2>/dev/null | head -1",
        timeout=15)
    app_path = out.strip().splitlines()[0] if out.strip() else ""
    if rc != 0 or not app_path:
        log_fail(name, ".app bundle not found on osxcross")
        if out.strip():
            print(out[-1500:])
        return None
    log_pass(name, f"-> {app_path}")
    return app_path


# ── stage datapack next to the .app ─────────────────────────────────────────
def stage_datapack(app_path):
    """Copy CatchChallenger-datapack/ to <parent-of-.app>/datapack/internal/.
    The parent dir mirrors the runtime layout: an .app at /root/foo.app
    expects its datapack at /root/datapack/internal/."""
    name = f"stage datapack near {os.path.basename(app_path)}"
    parent = os.path.dirname(app_path)
    target = parent + "/datapack/internal"
    log_info(f"{name}: {OSX_DATAPACK_DIR} -> {target}")
    rc, out = osx_ssh(
        f"rm -rf {parent}/datapack && mkdir -p {target} && "
        f"cp -a {OSX_DATAPACK_DIR}/. {target}/ 2>&1",
        timeout=300)
    if rc != 0:
        log_fail(name, f"rc={rc}")
        if out.strip():
            print(out[-1500:])
        return False
    log_pass(name)
    return True


# ── deploy + sign + package ─────────────────────────────────────────────────
def deploy_and_package(app_path, label):
    """Run macdeployqt, ad-hoc codesign (best-effort), then produce a
    distributable archive next to the .app:
      - macdeployqt -dmg if the host has hdiutil (macOS only) OR if
        macdeployqt's built-in dmg path succeeds.
      - else, zip the .app for transport to a real Mac.

    Returns the artifact path on the remote host on success, None on
    deploy failure (packaging failure is non-fatal — the bundled .app
    is still useful)."""
    deploy_name = f"macdeployqt {label}"
    log_info(f"{deploy_name}: {OSX_MACDEPLOYQT} {app_path} -verbose=1")
    # macdeployqt resolves frameworks via OSX_QT/lib so DYLD_FRAMEWORK_PATH
    # is irrelevant here; we just need the binary on the remote.
    rc, out = osx_ssh(
        f"{OSX_MACDEPLOYQT} {shlex.quote(app_path)} -verbose=1 2>&1",
        timeout=DEPLOY_TIMEOUT)
    if rc != 0:
        log_fail(deploy_name, f"rc={rc}")
        if out.strip():
            print(out[-2500:])
        return None
    log_pass(deploy_name)

    # Ad-hoc codesign — osxcross ships per-target codesign_allocate but no
    # actual codesign tool, so on Linux we cannot produce an Apple-valid
    # signature. Try osxcross's `xcrun codesign` shim if it exists,
    # otherwise warn and continue. A real Developer ID signature + notarize
    # has to happen on a Mac (xcrun notarytool submit ... --wait).
    sign_name = f"codesign --sign - {label} (ad-hoc)"
    rc, out = osx_ssh(
        f"if command -v {OSX_TARGET_TRIPLE}-codesign >/dev/null 2>&1; then "
        f"  {OSX_TARGET_TRIPLE}-codesign --force --deep --sign - {shlex.quote(app_path)} 2>&1; "
        f"elif command -v xcrun >/dev/null 2>&1 && xcrun -f codesign >/dev/null 2>&1; then "
        f"  xcrun codesign --force --deep --sign - {shlex.quote(app_path)} 2>&1; "
        f"else "
        f"  echo 'no codesign tool on this Linux host; ad-hoc signature skipped'; "
        f"  echo 'real Apple Developer ID signing must run on macOS:'; "
        f"  echo '  codesign --deep --force --sign \"Developer ID Application: ...\" {os.path.basename(app_path)}'; "
        f"  echo '  xcrun notarytool submit ... --wait'; "
        f"  exit 0; "
        f"fi",
        timeout=120)
    if rc == 0:
        log_pass(sign_name)
    else:
        log_fail(sign_name, f"rc={rc}")
        if out.strip():
            print(out[-1500:])

    # Try .dmg first via macdeployqt -dmg (needs hdiutil, macOS-only).
    # Fallback: portable zip of the .app (and its sibling datapack/) so the
    # user can drop the archive on a Mac, double-click the .app, and run.
    pkg_name = f"package {label} (.dmg or .zip)"
    parent = os.path.dirname(app_path)
    app_base = os.path.basename(app_path)
    artifact_base = f"{label}-{OSX_TARGET_TRIPLE}"
    osx_ssh(f"mkdir -p {OSX_ARTIFACT_DIR}", timeout=15)
    rc, out = osx_ssh(
        f"if command -v hdiutil >/dev/null 2>&1; then "
        f"  cd {shlex.quote(parent)} && "
        f"  {OSX_MACDEPLOYQT} {shlex.quote(app_base)} -dmg -verbose=1 2>&1 && "
        f"  mv {shlex.quote(app_base[:-4] + '.dmg')} "
        f"     {OSX_ARTIFACT_DIR}/{shlex.quote(artifact_base + '.dmg')} && "
        f"  echo PACKAGE_OK={OSX_ARTIFACT_DIR}/{artifact_base}.dmg; "
        f"else "
        f"  cd {shlex.quote(parent)} && "
        f"  zip -qry {OSX_ARTIFACT_DIR}/{shlex.quote(artifact_base + '.zip')} "
        f"      {shlex.quote(app_base)} datapack 2>&1 && "
        f"  echo PACKAGE_OK={OSX_ARTIFACT_DIR}/{artifact_base}.zip; "
        f"fi",
        timeout=DEPLOY_TIMEOUT)
    artifact = ""
    li = 0
    lines = out.splitlines()
    while li < len(lines):
        if lines[li].startswith("PACKAGE_OK="):
            artifact = lines[li].split("=", 1)[1].strip()
        li += 1
    if rc != 0 or not artifact:
        log_fail(pkg_name, f"rc={rc}")
        if out.strip():
            print(out[-2000:])
        return app_path
    log_pass(pkg_name, f"-> {artifact}")
    return artifact


# ── main ────────────────────────────────────────────────────────────────────
def cleanup(*_args):
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — macOS osxcross compile + package")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    if not osx_host_reachable():
        log_info(f"osxcross host {OSX_USER}@{OSX_HOST} unreachable — skipping mac test")
        save_failed_cases()
        summary()
        return

    if should_run("rsync sources to osxcross", failed_cases):
        if not rsync_sources():
            save_failed_cases(); summary(); return

    dp_src = DATAPACKS[0] if DATAPACKS else None
    if dp_src is None:
        log_fail("rsync datapack to osxcross", "no datapack configured")
        save_failed_cases(); summary(); return
    if should_run("rsync datapack to osxcross", failed_cases):
        if not rsync_datapack(dp_src):
            save_failed_cases(); summary(); return

    targets = [
        (CLIENT_CPU_PRO, "qtcpu800x600"),
        (CLIENT_GL_PRO,  "qtopengl"),
    ]
    ti = 0
    while ti < len(targets):
        pro_rel, label = targets[ti]
        if should_run(f"compile {label} (mac, osxcross)", failed_cases):
            app_path = build_target(pro_rel, label)
            if app_path is not None:
                if should_run(f"stage datapack near {APP_NAME}", failed_cases):
                    stage_datapack(app_path)
                if should_run(f"macdeployqt {label}", failed_cases):
                    deploy_and_package(app_path, label)
        ti += 1

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
