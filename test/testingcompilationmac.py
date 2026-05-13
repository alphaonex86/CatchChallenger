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
import wall_cap
wall_cap.arm()
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


def rsync_to_osx(src, dst, extra_args=None):
    # Anchor every "build" exclude to the project ROOT (leading `/`),
    # otherwise rsync also drops vendored build/ subdirs like
    # general/libzstd/build/{cmake,meson,…} which the project's
    # CMakeLists.txt depends on (add_subdirectory(general/libzstd/
    # build/cmake)). Without the anchor, the osxcross-side cmake
    # configure fails with:
    #   add_subdirectory given source "general/libzstd/build/cmake"
    #   which is not an existing directory.
    args = ["rsync", "-art", "--delete",
            "-e", RSYNC_SSH_E,
            "--exclude=/build/", "--exclude=/build-*/",
            "--exclude=.git/"]
    if extra_args:
        args += list(extra_args)
    # rsync needs IPv6 hosts in [brackets]; otherwise it parses
    # `2803:1920::2:ff08:/path` as user@host:port:host:port…:path,
    # truncates the actual address to `2803`, asks DNS to resolve
    # that, gets a bogus IPv4 (`0.0.10.243` in practice), and times
    # out trying to connect to it. With brackets the whole IPv6
    # literal stays a single token.
    host = f"[{OSX_HOST}]" if ":" in OSX_HOST else OSX_HOST
    args += [src + "/", f"{OSX_USER}@{host}:{dst}/"]
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


# Datapack file-extension whitelist for installer / .dmg / .apk:
# only files the client actually loads at runtime end up shipped.
_DATAPACK_KEEP_EXT = ("tmx", "xml", "tsx", "js",
                      "png", "jpg", "gif",
                      "ogg", "opus")


def rsync_datapack(src):
    """rsync the datapack to osxcross — filtered to keep only runtime-
    relevant extensions (.tmx / .xml / .tsx / .js / .png / .jpg / .gif
    / .ogg / .opus). README.md / .po / .ts / build leftovers are
    excluded so the eventual .dmg stays under the shipping ceiling."""
    name = "rsync datapack to osxcross"
    log_info(f"{name}: {src} -> {OSX_USER}@{OSX_HOST}:{OSX_DATAPACK_DIR} "
             f"(ext whitelist: {','.join(_DATAPACK_KEEP_EXT)})")
    osx_ssh(f"mkdir -p {OSX_DATAPACK_DIR}", timeout=15)
    # rsync filter order matters — rules are evaluated top-down for
    # each path. We want:
    #   1. EXCLUDE every dot-prefixed entry first (.git, .qt,
    #      .DS_Store, .gitignore, …) regardless of file vs dir.
    #   2. INCLUDE every surviving directory so the recurse keeps
    #      going.
    #   3. INCLUDE files with one of the allow-listed extensions.
    #   4. EXCLUDE everything else.
    filters = ["--exclude=.*", "--include=*/"]
    ei = 0
    while ei < len(_DATAPACK_KEEP_EXT):
        filters.append(f"--include=*.{_DATAPACK_KEEP_EXT[ei]}")
        ei += 1
    filters.append("--exclude=*")
    ok, err = rsync_to_osx(src, OSX_DATAPACK_DIR, extra_args=filters)
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
        # The osxcross toolchain sets
        # CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY which restricts every
        # find_package() to look only inside CMAKE_FIND_ROOT_PATH
        # (toolchain default: the macOS SDK + macports). The Qt6
        # install at /root/qt6-macos/... isn't inside either, so
        # find_package(Qt6) fails even when -DCMAKE_PREFIX_PATH points
        # at it. Two fixes layered for robustness:
        #   1. Switch the find-root policy to BOTH so CMAKE_PREFIX_PATH
        #      is consulted alongside CMAKE_FIND_ROOT_PATH.
        #   2. ADD the Qt install root to CMAKE_FIND_ROOT_PATH itself.
        #      This makes the Qt6 sub-component find_package() calls
        #      (Qt6Core, Qt6Gui, Qt6Network, ...) resolve via the
        #      same path-search as the parent Qt6 find_package().
        # Pin Qt6_DIR explicitly so the top-level find_package(Qt6)
        # skips the search entirely.
        f"-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH "
        # The `;` in CMAKE_FIND_ROOT_PATH / CMAKE_PREFIX_PATH lists
        # is cmake's list separator but ALSO bash's statement
        # terminator — single-quote the value so the inner ssh shell
        # passes the whole string to cmake intact.
        f"'-DCMAKE_FIND_ROOT_PATH={OSX_QT};/usr/lib/x86_64-linux-gnu/cmake' "
        f"'-DCMAKE_PREFIX_PATH={OSX_QT};/usr' "
        f"-DQt6_DIR={OSX_QT}/lib/cmake/Qt6 "
        # Cross-compile needs HOST-native moc/uic/rcc; the target Qt
        # at OSX_QT ships Mach-O binaries that don't run on Linux.
        # Debian's qt6-base-dev-tools installs:
        #   /usr/lib/qt6/libexec/{moc,uic,rcc,…}                (Linux ELF)
        #   /usr/lib/x86_64-linux-gnu/cmake/Qt6CoreTools/       (cmake config)
        # Pin each `*Tools` cmake-config dir explicitly so Qt's
        # cross-compile find logic (Qt6Config.cmake → for module
        # matching `Tools$`) picks up the host tools from /usr instead
        # of trying to run the Mach-O binaries.
        f"-DQt6CoreTools_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6CoreTools "
        f"-DQt6GuiTools_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6GuiTools "
        f"-DQt6WidgetsTools_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6WidgetsTools "
        f"-DQt6DBusTools_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6DBusTools "
        f"-DCMAKE_MACOSX_BUNDLE=ON "
        f"-DCMAKE_C_COMPILER_LAUNCHER=ccache "
        f"-DCMAKE_CXX_COMPILER_LAUNCHER=ccache "
        # -fno-lto on test builds (see cmake_helpers.py for the rationale).
        f"-DCMAKE_C_FLAGS_INIT=-fno-lto "
        f"-DCMAKE_CXX_FLAGS_INIT=-fno-lto "
        f"-DCMAKE_EXE_LINKER_FLAGS='-fuse-ld=lld -fno-lto' "
        f"-DCMAKE_SHARED_LINKER_FLAGS='-fuse-ld=lld -fno-lto' "
        f"-DCMAKE_MODULE_LINKER_FLAGS='-fuse-ld=lld -fno-lto' "
        # Audio + websockets are re-enabled on the osxcross Qt6 install
        # (Qt6Multimedia + Qt6WebSockets are present alongside Qt6Core).
        # If a fresh osxcross host is missing either module, install
        # them with:
        #   aqt install-qt mac desktop 6.5.3 clang_64 -O /root/qt6-macos \
        #       -m qtmultimedia qtwebsockets qtcharts
        # (or rsync them over from a host with IPv4 reachable to
        # download.qt.io if the osxcross host is IPv6-only).
        f"{flag_args} 2>&1",
        timeout=COMPILE_TIMEOUT)
    if rc != 0:
        _fc.set_extras(name, host=OSX_HOST, compile_output=(out or ""))
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return None
    log_info(f"ninja build {label} -j$(nproc)")
    build_cmd = f"{OSX_CMAKE} --build {build_dir} --target {target} -j$(nproc) 2>&1"
    rc, out = osx_ssh(build_cmd, timeout=COMPILE_TIMEOUT)
    if rc != 0:
        log_info(f"build failed; flushing remote ccache and retrying once ({label})")
        osx_ssh("ccache -C 2>/dev/null || true", timeout=120)
        rc, out = osx_ssh(build_cmd, timeout=COMPILE_TIMEOUT)
    if rc != 0:
        _fc.set_extras(name,
                       host=OSX_HOST,
                       cmd=f"ssh {OSX_USER}@{OSX_HOST} {build_cmd!r}",
                       compile_output=(out or ""))
        log_fail(name, f"ninja build failed (rc={rc}) [retried after remote ccache -C]")
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
    # Qt's macdeployqt is a Mach-O binary that won't run on the Linux
    # osxcross host. Use the Linux-native equivalent we ship at
    # test/macdeployqt_linux.py — it walks the .app's binary deps via
    # the osxcross otool/install_name_tool wrappers (Linux ELF), copies
    # the needed Qt6 frameworks + plugins into the bundle, and rewrites
    # install_names so the .app finds them via
    # @executable_path/../Frameworks/. Drop the script onto the
    # osxcross host on each run so the local edits land immediately.
    script_local  = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                 "macdeployqt_linux.py")
    script_remote = f"{OSX_WORK_DIR}/macdeployqt_linux.py"
    # Push the script with a small rsync — same RSYNC_SSH_E used
    # everywhere so ConnectTimeout / BatchMode policy applies.
    push = subprocess.run(
        ["rsync", "-art",
         "-e", RSYNC_SSH_E,
         script_local,
         f"{OSX_USER}@[{OSX_HOST}]:{script_remote}"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        timeout=clamp_local(30))
    if push.returncode != 0:
        log_fail(deploy_name, f"rsync of macdeployqt_linux.py failed "
                              f"(rc={push.returncode})")
        sys.stdout.write(push.stdout.decode(errors='replace')[-1500:])
        return None
    log_info(f"{deploy_name}: macdeployqt_linux.py {app_path}")
    rc, out = osx_ssh(
        f"python3 {script_remote} "
        f"--app {shlex.quote(app_path)} "
        f"--qt {shlex.quote(OSX_QT)} "
        f"--osxcross-bin /root/osxcross/target/bin "
        f"--triple {OSX_TARGET_TRIPLE} 2>&1",
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

    # Build a real .dmg via libdmg-hfsplus (Linux-native HFS+ DMG
    # creator we built at /root/libdmg-hfsplus/build/dmg/dmg on the
    # osxcross host — see commit for the install + build steps).
    # The tool wants a STAGING DIRECTORY containing what to place on
    # the DMG (typically the .app + a Symlink/Alias to /Applications,
    # but we just ship the .app + sibling datapack/), produces an
    # uncompressed HFS+ image, then converts to a UDZO-compressed .dmg.
    # Fallback: portable .zip when /root/libdmg-hfsplus/build/dmg/dmg
    # isn't installed (skip the dmg step gracefully).
    pkg_name = f"package {label} (.dmg or .zip)"
    parent = os.path.dirname(app_path)
    app_base = os.path.basename(app_path)
    artifact_base = f"{label}-{OSX_TARGET_TRIPLE}"
    osx_ssh(f"mkdir -p {OSX_ARTIFACT_DIR}", timeout=15)
    rc, out = osx_ssh(
        f"set -e; "
        f"DMG_BIN=/root/libdmg-hfsplus/build/dmg/dmg; "
        f"if [ -x \"$DMG_BIN\" ]; then "
        # Stage the .app + datapack under a clean temp dir so the
        # DMG's root has predictable contents.
        f"  STAGE=$(mktemp -d /tmp/cc-dmg-stage.XXXXXX); "
        f"  cp -a {shlex.quote(parent)}/{shlex.quote(app_base)} \"$STAGE/\"; "
        f"  [ -d {shlex.quote(parent)}/datapack ] && "
        f"      cp -a {shlex.quote(parent)}/datapack \"$STAGE/\"; "
        f"  OUT={OSX_ARTIFACT_DIR}/{shlex.quote(artifact_base + '.dmg')}; "
        # libdmg-hfsplus's `dmg create` wants a SOURCE.iso first (HFS+
        # ISO image), then converts to compressed .dmg. genisoimage's
        # -hfs produces the right HFS+ overlay; xorrisofs is the
        # modern equivalent on Debian/recent.
        f"  ISO=$(mktemp /tmp/cc-dmg.XXXXXX.iso); "
        f"  genisoimage -V CatchChallenger -no-pad -r -hfs -hide-hfs '*.DS_Store' "
        f"      -o \"$ISO\" \"$STAGE\" 2>&1 | tail -3; "
        f"  \"$DMG_BIN\" build \"$ISO\" \"$OUT\" 2>&1 | tail -3; "
        f"  rm -rf \"$STAGE\" \"$ISO\"; "
        f"  echo PACKAGE_OK=$OUT; "
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
