#!/usr/bin/env python3
"""
macdeployqt_linux.py — Linux-native macdeployqt equivalent for osxcross.

Why: Qt's own macdeployqt ships only as a Mach-O binary, so it can't
run on a Linux build host. After the osxcross cmake build produces
catchchallenger.app, we still need to:

  1. Copy every linked Qt6*.framework into <app>/Contents/Frameworks/.
  2. Rewrite each binary's install_name table (otool -L) so the .app
     looks for those frameworks at @executable_path/../Frameworks/
     instead of /root/qt6-macos/6.5.3/macos/lib (the build-time path
     that doesn't exist on a real Mac).
  3. Copy the Qt plugins the binary needs (platforms/, imageformats/,
     iconengines/, sqldrivers/, tls/, multimedia/, networkinformation/,
     styles/) into <app>/Contents/PlugIns/<category>/ + rewrite their
     install names too.
  4. Drop a qt.conf at <app>/Contents/Resources/qt.conf so QtCore at
     startup looks for plugins in the bundled location.

We use osxcross's Linux-native Mach-O tools:
  <triple>-otool             — list dependencies
  <triple>-install_name_tool — rewrite install names

`x86_64-apple-darwin20.4-otool` / `…-install_name_tool` are part of
every osxcross build; no extra install needed.

Usage (intended caller: testingcompilationmac.py):
  python3 macdeployqt_linux.py \
      --app /path/to/catchchallenger.app \
      --qt  /root/qt6-macos/6.5.3/macos \
      --osxcross-bin /root/osxcross/target/bin \
      --triple x86_64-apple-darwin20.4
"""

import argparse
import os
import re
import shutil
import subprocess
import sys


def run(cmd, check=True, capture=True):
    """Run a subprocess; return (rc, stdout)."""
    p = subprocess.run(cmd,
                       stdout=subprocess.PIPE if capture else None,
                       stderr=subprocess.STDOUT,
                       text=True)
    if check and p.returncode != 0:
        sys.stderr.write(f"[macdeployqt_linux] FAIL: {' '.join(cmd)}\n")
        sys.stderr.write(p.stdout or "")
        sys.exit(p.returncode)
    return p.returncode, p.stdout or ""


_LIB_RE = re.compile(r"^\s+(\S+)\s+\(compatibility")


def list_deps(otool, target):
    """Return the install_name strings otool -L reports for `target`."""
    _, out = run([otool, "-L", target])
    deps = []
    li = 0
    lines = out.splitlines()
    while li < len(lines):
        m = _LIB_RE.match(lines[li])
        if m:
            deps.append(m.group(1))
        li += 1
    return deps


# Plugin categories an end-user Qt6 app might dlopen at runtime. We
# copy ONLY the categories that actually exist for the target build
# (e.g. no Qt6Multimedia → no multimedia/ dir to copy). The per-app
# dependency walk picks up the framework binaries; the plugin copy
# is a static set because Qt loads plugins via QPluginLoader at
# runtime, not via the binary's link-time dependency table.
_PLUGIN_CATEGORIES = (
    "platforms",           # qcocoa.dylib — required for any GUI app
    "imageformats",        # qjpeg, qico, qsvg, …
    "iconengines",         # qsvgicon
    "sqldrivers",          # qsqlite (in-app save game)
    "tls",                 # qopensslbackend, qsecuretransportbackend
    "multimedia",          # darwinmediaplugin (Qt6Multimedia)
    "networkinformation",  # qscnetworkreachability
    "styles",              # qmacstyle
    "generic",             # qtuiotouchplugin
)


def copy_frameworks(app, qt_prefix, install_name_tool):
    """Walk the app's binary deps, copy each referenced Qt framework
    into <app>/Contents/Frameworks/, and rewrite every binary's
    install_name table to point at @executable_path/../Frameworks/."""
    fw_dst = os.path.join(app, "Contents", "Frameworks")
    os.makedirs(fw_dst, exist_ok=True)

    # Start with the main exe + every helper binary inside the .app.
    todo = []
    for root, dirs, files in os.walk(app):
        fi = 0
        while fi < len(files):
            n = files[fi]
            fi += 1
            p = os.path.join(root, n)
            # Heuristic: targets that need install_name fixups are
            # files in Contents/MacOS/ or any .dylib / framework
            # binary (no extension is also Mach-O on macOS).
            if (("/Contents/MacOS/" in p
                 or n.endswith(".dylib"))
                and os.path.isfile(p)
                and not os.path.islink(p)):
                todo.append(p)
    seen_fw = set()
    qt_lib_prefix = os.path.join(qt_prefix, "lib") + "/"

    while todo:
        target = todo.pop()
        deps = list_deps(otool_path, target)
        for dep in deps:
            # Only handle Qt-framework deps; system libs (/usr/lib/…)
            # stay as-is.
            if not (dep.startswith(qt_lib_prefix)
                    or dep.startswith("@rpath/Qt")):
                continue
            # dep is e.g. /root/qt6-macos/6.5.3/macos/lib/QtCore.framework/Versions/A/QtCore
            # or          @rpath/QtCore.framework/Versions/A/QtCore
            # Extract the "QtFoo.framework" name.
            mi = dep.find(".framework")
            if mi < 0:
                continue
            slash = dep.rfind("/", 0, mi)
            fw_name = dep[slash + 1:mi] + ".framework"
            src_fw = os.path.join(qt_prefix, "lib", fw_name)
            dst_fw = os.path.join(fw_dst, fw_name)
            if not os.path.isdir(src_fw):
                sys.stderr.write(
                    f"[macdeployqt_linux] WARN: framework not found "
                    f"at {src_fw}; skipping\n")
                continue
            if fw_name not in seen_fw:
                seen_fw.add(fw_name)
                if os.path.exists(dst_fw):
                    shutil.rmtree(dst_fw)
                shutil.copytree(src_fw, dst_fw, symlinks=True)
                # Queue the copied framework's binary for recursion —
                # frameworks depend on other frameworks (Qt6Gui →
                # Qt6Core → …).
                fw_bin = os.path.join(dst_fw, fw_name[:-len(".framework")])
                if os.path.isfile(fw_bin) and not os.path.islink(fw_bin):
                    todo.append(fw_bin)
            # Rewrite this dep in the current target.
            # Compute the new install name:
            #   @executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore
            rel_tail = dep[mi:]  # everything from ".framework" on
            new_name = ("@executable_path/../Frameworks/"
                        + fw_name + rel_tail[len(".framework"):])
            run([install_name_tool, "-change", dep, new_name, target])


def copy_plugins(app, qt_prefix, install_name_tool):
    """Copy the standard Qt6 plugin categories into the .app and
    rewrite each plugin's install_name table the same way."""
    src_plugins = os.path.join(qt_prefix, "plugins")
    dst_plugins = os.path.join(app, "Contents", "PlugIns")
    if not os.path.isdir(src_plugins):
        return
    os.makedirs(dst_plugins, exist_ok=True)
    qt_lib_prefix = os.path.join(qt_prefix, "lib") + "/"
    fw_dst = os.path.join(app, "Contents", "Frameworks")
    ci = 0
    while ci < len(_PLUGIN_CATEGORIES):
        cat = _PLUGIN_CATEGORIES[ci]
        ci += 1
        src_cat = os.path.join(src_plugins, cat)
        if not os.path.isdir(src_cat):
            continue
        dst_cat = os.path.join(dst_plugins, cat)
        if os.path.exists(dst_cat):
            shutil.rmtree(dst_cat)
        shutil.copytree(src_cat, dst_cat, symlinks=True)
        # Fix install names of every .dylib in this category.
        for entry in os.listdir(dst_cat):
            if not entry.endswith(".dylib"):
                continue
            plugin = os.path.join(dst_cat, entry)
            deps = list_deps(otool_path, plugin)
            for dep in deps:
                if not (dep.startswith(qt_lib_prefix)
                        or dep.startswith("@rpath/Qt")):
                    continue
                mi = dep.find(".framework")
                if mi < 0:
                    continue
                slash = dep.rfind("/", 0, mi)
                fw_name = dep[slash + 1:mi] + ".framework"
                rel_tail = dep[mi:]
                new_name = ("@executable_path/../../Frameworks/"
                            + fw_name + rel_tail[len(".framework"):])
                run([install_name_tool, "-change", dep, new_name, plugin])
    # Make sure every framework we just copied has its OWN id set to
    # @executable_path/.../<framework name>/Versions/A/<framework
    # name> too, otherwise dlopen() at startup hits the original
    # /root/qt6-macos/... path embedded in LC_ID_DYLIB and the loader
    # rejects it on a real Mac.
    if os.path.isdir(fw_dst):
        for fw in os.listdir(fw_dst):
            if not fw.endswith(".framework"):
                continue
            short = fw[:-len(".framework")]
            # Standard Qt framework layout: <fw>/Versions/A/<short>
            fw_bin = os.path.join(fw_dst, fw, "Versions", "A", short)
            if not os.path.isfile(fw_bin):
                continue
            id_str = ("@executable_path/../Frameworks/"
                      + fw + "/Versions/A/" + short)
            run([install_name_tool, "-id", id_str, fw_bin])


def write_qt_conf(app):
    """Drop a Contents/Resources/qt.conf so QtCore knows to load
    plugins from the bundled PlugIns/ tree rather than from the
    build-host path baked into Qt6Core at compile time."""
    res = os.path.join(app, "Contents", "Resources")
    os.makedirs(res, exist_ok=True)
    qt_conf = os.path.join(res, "qt.conf")
    with open(qt_conf, "w") as f:
        f.write("[Paths]\n"
                "Plugins = PlugIns\n"
                "Imports = Resources/qml\n"
                "Qml2Imports = Resources/qml\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--app", required=True,
                    help="Path to the .app bundle to deploy.")
    ap.add_argument("--qt", required=True,
                    help="Qt install prefix on the build host "
                         "(e.g. /root/qt6-macos/6.5.3/macos).")
    ap.add_argument("--osxcross-bin", required=True,
                    help="osxcross target/bin dir holding the "
                         "<triple>-otool / install_name_tool wrappers.")
    ap.add_argument("--triple", required=True,
                    help="Target triple prefix on the osxcross tool "
                         "names (e.g. x86_64-apple-darwin20.4).")
    args = ap.parse_args()
    global otool_path
    otool_path = os.path.join(args.osxcross_bin,
                              f"{args.triple}-otool")
    install_name_tool = os.path.join(args.osxcross_bin,
                                     f"{args.triple}-install_name_tool")
    for p in (otool_path, install_name_tool):
        if not os.path.isfile(p):
            sys.stderr.write(f"[macdeployqt_linux] missing tool: {p}\n")
            return 1
    if not os.path.isdir(args.app):
        sys.stderr.write(f"[macdeployqt_linux] not a directory: {args.app}\n")
        return 1
    if not os.path.isdir(args.qt):
        sys.stderr.write(f"[macdeployqt_linux] Qt prefix missing: {args.qt}\n")
        return 1
    copy_frameworks(args.app, args.qt, install_name_tool)
    copy_plugins(args.app, args.qt, install_name_tool)
    write_qt_conf(args.app)
    print(f"[macdeployqt_linux] OK: {args.app}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
