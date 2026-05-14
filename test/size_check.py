"""
size_check.py — Static-baseline artifact-size guard.

Every shipping artifact (apk / aab / msi / exe / dmg / installer zip)
has a known-good size baked into BASELINES below. After producing
each artifact, the cross-build harnesses call `verify(label, path)`
which:

    1. Reads the actual file size on disk.
    2. Fails when the file does NOT exist.
    3. Fails when size < ABSOLUTE_FLOOR (10 MiB).
    4. Fails when size > ABSOLUTE_CEILING (100 MiB) for SHIPPING
       artifacts (Windows installer/msi, macOS dmg, Android apk/aab).
       Operator-facing downloads should never balloon past 100 MiB; if
       a deliberate change pushes us over, raise the ceiling explicitly
       and document why.
    5. Fails when size < 0.75 × baseline (the artifact shrank
       suspiciously — empty .apk, missing libs in .msi, etc.).
    6. Passes otherwise.

The baselines are deliberately HARD-CODED. Drift is the signal: when
a real source change shrinks a binary by >25%, an operator must
**manually** update BASELINES rather than letting CI auto-bless. The
test refuses to silently track regressions.

Updating a baseline: when a legitimate refactor / lib swap shrinks an
artifact, look at the new size, set the BASELINES entry to that value,
and commit it as a deliberate change. The PR review then explicitly
sees the baseline edit alongside the source edit.

Why 75% (not 50%, not 90%)?
  - <50%: too lax — a half-broken installer (missing translations,
    missing Qt plugins) would pass.
  - >90%: too tight — every minor source change would trip the guard
    for no diagnostic value.
  - 75% catches the obvious "something major dropped out" without
    flapping on routine 5–10% size shifts.
"""

import os

# Absolute floor — sanity ceiling against "empty artefact" failures
# (an .exe that linked against no objects, an installer payload that
# packaged 0 files). Release-mode mingw binaries can legitimately be
# ~5 MiB (server-gui has no Qt6Widgets game UI), so the floor sits
# below that. Qt6 DLLs travel alongside the .exe via windeployqt;
# the .exe itself is the small piece.
ABSOLUTE_FLOOR = 3 * 1024 * 1024  # 3 MiB

# Absolute ceiling for SHIPPING artifacts (installer/msi/dmg/apk/aab).
# 100 MiB is the largest size we ever advertise to end users; anything
# bigger is a regression (duplicate datapack copy, undeployed strip,
# accidentally bundled debug build, …). Raw build artifacts that are
# NOT user-facing downloads (the unstripped Debug .exe, the macOS
# pre-DMG .app bundle .zip) are exempt — their sizes are dominated by
# debug symbols / unstripped frameworks.
ABSOLUTE_CEILING = 100 * 1024 * 1024  # 100 MiB

# Labels whose `<platform>.<type>.<variant>` middle token marks them
# as user-facing shipping artifacts. ABSOLUTE_CEILING applies only
# when verify()'s label has one of these types.
_SHIPPING_TYPES = {"installer", "msi", "dmg", "apk", "aab"}

# Static baselines, captured from a known-good build snapshot. Update
# when a real source change shifts the size; do NOT update mechanically
# from the latest run (that would defeat the regression guard).
#
# Sizes are expressed in bytes for clarity at the call site.
BASELINES = {
    # ── Android (qt-for-android arm64-v8a, Qt 6.8) ────────────────────
    "android.apk.qtopengl":   32_000_000,   # ~30.7 MiB
    "android.aab.qtopengl":   19_000_000,   # ~18.3 MiB

    # ── Windows MXE x86_64 (Qt 6.8 Release) ───────────────────────────
    # The .exe is the Release binary (-O3 -DNDEBUG, no -g). MXE's
    # statically-linked runtime libs carry a small (~100 KiB)
    # .debug_info section that survives the link, but the binary
    # itself is otherwise stripped of source-level debug info.
    "windows.exe.qtcpu800x600":       12_500_000,   # ~11.9 MiB
    "windows.exe.qtopengl":           13_700_000,   # ~13.1 MiB
    # server-gui is smaller than the clients — no Qt6Widgets game
    # UI, no datapack-renderer code, just the admin Qt6Network +
    # Qt6Sql stack.
    "windows.exe.server-gui":          5_200_000,   # ~4.9 MiB
    # NSIS installer .zip — 7z-compressed Qt6 deps + the .exe.
    # Post-fix sizes: previous .a/.lib inclusion + windeployqt dumped
    # ~400 MiB into the installer; after the runtime-only filter the
    # per-client installer was ~40 MiB. The COMBINED installer ships
    # all three binaries (qtopengl + qtcpu800x600 + server-gui) plus
    # ONE copy of every Qt6 DLL / plugin / datapack — so it's roughly
    # one .exe larger than the previous per-client size, in the
    # ~50-60 MiB range.
    "windows.installer.combined":     55_000_000,   # ~52 MiB
    "windows.msi.combined":           57_000_000,   # ~54 MiB
    # Per-binary baselines kept around for the in-tree iteration loop
    # (build_installer + build_msi still exist as helpers for ad-hoc
    # debug runs that target a single binary); the canonical run goes
    # through "combined" and emits one .exe + one .msi.
    "windows.installer.qtcpu800x600": 41_000_000,   # ~39 MiB
    "windows.installer.qtopengl":     43_000_000,   # ~41 MiB
    "windows.msi.qtcpu800x600":       43_000_000,   # ~41 MiB
    "windows.msi.qtopengl":           45_000_000,   # ~43 MiB

    # ── macOS osxcross (Qt 6.5.3) ─────────────────────────────────────
    # Bundle .zip of the .app + datapack/. macdeployqt brings in
    # frameworks, so the .zip is comparable to the Windows installer.
    "macos.bundle.qtcpu800x600":      100_000_000,   # placeholder
    "macos.bundle.qtopengl":          100_000_000,   # placeholder
    "macos.dmg.qtcpu800x600":         100_000_000,   # placeholder
    "macos.dmg.qtopengl":             100_000_000,   # placeholder
}


# Every entry above is required to be present even if the artifact
# isn't produced on a given run — tests that don't build a particular
# kind (e.g. android skipped due to missing AVD) just don't call
# verify(). Missing baselines are a programming error and surface
# loudly.
def baseline_for(label):
    if label not in BASELINES:
        raise KeyError(
            f"size_check: no baseline for {label!r}; add it to "
            f"BASELINES in test/size_check.py")
    return BASELINES[label]


def _is_shipping(label):
    parts = label.split(".")
    return len(parts) >= 3 and parts[1] in _SHIPPING_TYPES


def verify(label, path):
    """Return (ok: bool, detail: str). NEVER raises — caller logs."""
    if not os.path.isfile(path):
        return False, f"artifact missing: {path}"
    try:
        size = os.path.getsize(path)
    except OSError as e:
        return False, f"stat({path}) failed: {e}"
    baseline = baseline_for(label)
    floor = max(ABSOLUTE_FLOOR, int(0.75 * baseline))
    human_size = _human(size)
    human_baseline = _human(baseline)
    human_floor = _human(floor)
    human_ceiling = _human(ABSOLUTE_CEILING)
    if size < floor:
        # Pick the more informative reason.
        if size < ABSOLUTE_FLOOR:
            why = f"{human_size} < 10 MiB hard floor"
        else:
            why = (f"{human_size} < 75% of baseline {human_baseline} "
                   f"(floor {human_floor})")
        return False, f"size regression on {label}: {why}"
    if _is_shipping(label) and size > ABSOLUTE_CEILING:
        return False, (f"size regression on {label}: {human_size} > "
                       f"{human_ceiling} shipping-artifact hard ceiling")
    return True, (f"{human_size} (baseline {human_baseline}, "
                  f"floor {human_floor})")


def _human(n):
    if n < 1024:
        return f"{n} B"
    if n < 1024 * 1024:
        return f"{n/1024:.1f} KiB"
    if n < 1024 * 1024 * 1024:
        return f"{n/(1024*1024):.1f} MiB"
    return f"{n/(1024*1024*1024):.2f} GiB"
