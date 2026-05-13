#!/bin/bash
# Publish freshly-built client installers to the web VPS files dir:
#   /home/first-world.info/catchchallenger/files/<version>/
# then bump /home/first-world.info/catchchallenger/updater.txt.
#
# Usage: publish_binaries.sh <target> [<target> ...]
#   target ∈ { windows | mac | android }
#
# Each requested target MUST exist and be within
# [MIN_BYTES, MAX_BYTES] (default 10 MiB .. 100 MiB), otherwise this
# script aborts and updater.txt is NOT touched. Refusing to publish
# (and refusing to bump updater.txt) is the whole point — updater.txt
# drives download.php's per-version URLs, so bumping it without
# artifacts on disk produces 404s for every client trying to
# self-update. The upper bound catches a broken build that bundled
# debug symbols / wrong Qt / forgotten data dir.

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GIT_ROOT="$(realpath "${HERE}/..")"
WEB_VPS="${WEB_VPS:-root@763.vps.confiared.com}"
MIN_BYTES="${MIN_BYTES:-$((10 * 1024 * 1024))}"
MAX_BYTES="${MAX_BYTES:-$((100 * 1024 * 1024))}"

[ $# -gt 0 ] || { echo "usage: $0 <windows|mac|android> [...]" >&2; exit 2; }

VERSION="$(grep -F 'CATCHCHALLENGER_VERSION_PRIVATE' \
    "${GIT_ROOT}/general/base/VersionPrivate.hpp" \
    | sed -nE 's/.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*/\1/p' | head -1)"
[ -n "$VERSION" ] || { echo "[publish] cannot parse version from VersionPrivate.hpp" >&2; exit 1; }

TMPFS_BUILD_ROOT="$(python3 -c "import sys; sys.path.insert(0,'${HERE}'); import test_config as c; print(c.TMPFS_BUILD_ROOT)")"
ANDROID_WS="$(python3 -c "import sys; sys.path.insert(0,'${HERE}'); import test_config as c; print(c.ANDROID_WORKSPACE)")"
MAC_DIR="${TMPFS_BUILD_ROOT}/client/qtopengl/build/testing-mac"

TARGET_DIR="/home/first-world.info/catchchallenger/files/${VERSION}"

say() { printf '[publish] %s\n' "$*"; }
die() { printf '[publish][FAIL] %s\n' "$*" >&2; exit 1; }

# Resolve all artifacts and validate sizes BEFORE any rsync, so a
# failure leaves nothing half-published.
declare -A SRC=()
declare -A DST=()

resolve() {
    local target="$1" src="$2" dst="$3"
    [ -n "$src" ] && [ -f "$src" ] || die "${target}: artifact not found (looked at: ${src:-<no match>})"
    local sz
    sz=$(stat -c%s "$src")
    [ "$sz" -ge "$MIN_BYTES" ] || die "${target}: ${src} is ${sz} bytes (< ${MIN_BYTES})"
    [ "$sz" -le "$MAX_BYTES" ] || die "${target}: ${src} is ${sz} bytes (> ${MAX_BYTES})"
    SRC[$target]="$src"
    DST[$target]="$dst"
    say "${target}: ${src} (${sz} bytes) → ${TARGET_DIR}/${dst}"
}

for t in "$@"; do
    case "$t" in
        windows)
            win_exe="$(find "${TMPFS_BUILD_ROOT}/client/qtopengl/build/testing-wine64" \
                -maxdepth 3 -name '*-installer.exe' 2>/dev/null | head -1 || true)"
            win_zip="$(find "${TMPFS_BUILD_ROOT}/client/qtopengl/build/testing-wine64" \
                -maxdepth 3 -name '*-installer.zip' 2>/dev/null | head -1 || true)"
            resolve windows-exe "$win_exe" "catchchallenger-qtcpu800x600-windows-x86-${VERSION}-setup.exe"
            resolve windows-zip "$win_zip" "catchchallenger-qtcpu800x600-windows-x86-${VERSION}.zip"
            ;;
        mac)
            dmg=""
            for d in /root/catchchallenger-mac-artifacts "$MAC_DIR"; do
                dmg="$(find "$d" -maxdepth 2 -name '*.dmg' 2>/dev/null | head -1 || true)"
                [ -n "$dmg" ] && break
            done
            resolve mac "$dmg" "catchchallenger-mac-os-x-${VERSION}.dmg"
            ;;
        android)
            resolve android "${ANDROID_WS}/apk/qtopengl.apk" "catchchallenger-android-${VERSION}.apk"
            ;;
        *)
            die "unknown target: $t (expected: windows|mac|android)"
            ;;
    esac
done

say "uploading to ${WEB_VPS}:${TARGET_DIR}"
ssh "$WEB_VPS" "mkdir -p ${TARGET_DIR}"
for t in "${!SRC[@]}"; do
    rsync -avrt --progress "${SRC[$t]}" "${WEB_VPS}:${TARGET_DIR}/${DST[$t]}"
done

ssh "$WEB_VPS" "chown -R www-data:www-data ${TARGET_DIR} \
    && printf '%s' '${VERSION}' > /home/first-world.info/catchchallenger/updater.txt \
    && chown www-data:www-data /home/first-world.info/catchchallenger/updater.txt"
say "updater.txt → ${VERSION}"
say "done"
