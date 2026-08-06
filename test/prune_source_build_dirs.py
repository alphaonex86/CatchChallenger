#!/usr/bin/env python3
"""Keep IN-SOURCE build dirs from growing without bound.

CMake/qmake build dirs belong OUT of the source tree, but Qt Creator defaults to
putting one right next to the .pro/CMakeLists (client/build/Desktop-Debug/), and
nothing ever trims it: it was found holding 1.1 GB, of which 501 MB was .o and
232 MB .a — pure regenerable scratch.

The clear is PROGRESSIVE: stop as soon as the tree is under budget, and always
give up the cheapest thing first, so a developer's next build stays incremental
instead of paying a full reconfigure.

  tier 1  LINKED BINARIES — the cheapest thing in the tree to get back: with the
          .o still present, relinking a 184 MB client took 2 s (measured). Keeping
          a binary while its .o are gone is pure dead weight, which is why these
          go FIRST and the scratch is kept.
  tier 2  static archives (.a) — re-archived from the .o that are still there.
  tier 3  compiler scratch (.o/.d/.rsp/...) — the expensive one: getting these
          back is a FULL recompile, so it is the last thing given up. Shares
          cleanup_helpers' suffix list, the same set all.sh prunes after a fail.
  tier 4  the whole dir — forces a full reconfigure + rebuild, so it NEVER runs
          unless --allow-remove-dir is passed explicitly.

Dry-run first (--dry-run) to see exactly what would go. Nothing outside a
recognised build dir is ever touched: a candidate must both be named like a
build dir AND contain CMake/qmake state.
"""
import argparse
import os
import sys
import time

import cleanup_helpers

#: A directory is a build dir only if its name looks like one AND it holds one
#: of these. Both conditions, so a source dir that happens to be called "build"
#: (or a stale dir holding hand-written files) is never eaten.
_BUILD_DIR_NAMES = ("build", "build-debug", "build-release")
_BUILD_MARKERS = ("CMakeCache.txt", "build.ninja", "Makefile", ".qmake.stash")

_ARCHIVE_SUFFIXES = (".a",)
#: Linked outputs by name, for targets whose magic we do not sniff (Windows).
_LINKED_SUFFIXES = (".exe", ".dll", ".so")


def _is_linked_output(path):
    """True for a LINKED binary (executable / shared object), false for a .o.

    Sniffs the magic instead of trusting the name: on Linux the interesting
    files have NO extension, and a name-based guess would happily eat `Makefile`
    or `build.ninja` and break the build dir. ELF carries its kind in e_type at
    offset 16 — ET_EXEC(2)/ET_DYN(3) is a linked output, ET_REL(1) is a .o, which
    shares the very same \\x7fELF magic. 'MZ' covers the mingw .exe/.dll."""
    low = path.lower()
    si = 0
    while si < len(_LINKED_SUFFIXES):
        if low.endswith(_LINKED_SUFFIXES[si]):
            return True
        si += 1
    try:
        with open(path, "rb") as fh:
            head = fh.read(18)
    except OSError:
        return False
    if len(head) < 18:
        return False
    if head[:2] == b"MZ":
        return True
    if head[:4] != b"\x7fELF":
        return False
    little = head[5] != 2                       # EI_DATA: 1=LSB, 2=MSB
    e_type = int.from_bytes(head[16:18], "little" if little else "big")
    return e_type in (2, 3)                     # ET_EXEC / ET_DYN


def _remove_linked_outputs(build_dir, dry_run):
    """Delete linked binaries/shared objects. Returns (count, bytes)."""
    count = 0
    freed = 0
    for root, _dirs, files in os.walk(build_dir, topdown=False):
        for name in files:
            fp = os.path.join(root, name)
            if os.path.islink(fp) or not _is_linked_output(fp):
                continue
            try:
                sz = os.path.getsize(fp)
            except OSError:
                continue
            if not dry_run:
                try:
                    os.remove(fp)
                except OSError:
                    continue
            count += 1
            freed += sz
    return count, freed


def dir_size(path):
    """Bytes used by everything under `path` (symlinks not followed)."""
    total = 0
    for root, _dirs, files in os.walk(path):
        for name in files:
            fp = os.path.join(root, name)
            try:
                st = os.lstat(fp)
            except OSError:
                continue
            if not os.path.islink(fp):
                total += st.st_size
    return total


def _is_build_dir(path):
    if os.path.basename(path).lower() not in _BUILD_DIR_NAMES:
        return False
    mi = 0
    while mi < len(_BUILD_MARKERS):
        if os.path.exists(os.path.join(path, _BUILD_MARKERS[mi])):
            return True
        mi += 1
    # Qt Creator nests the real build under <build>/<Kit-Name>/; accept the
    # parent when any immediate child carries a marker.
    try:
        entries = os.listdir(path)
    except OSError:
        return False
    for child in entries:
        cp = os.path.join(path, child)
        if os.path.isdir(cp):
            mi = 0
            while mi < len(_BUILD_MARKERS):
                if os.path.exists(os.path.join(cp, _BUILD_MARKERS[mi])):
                    return True
                mi += 1
    return False


def find_build_dirs(root):
    """In-source build dirs under `root`, outermost first (never nested)."""
    found = []
    for dirpath, dirnames, _files in os.walk(root):
        if ".git" in dirnames:
            dirnames.remove(".git")
        keep = []
        for d in dirnames:
            full = os.path.join(dirpath, d)
            if _is_build_dir(full):
                found.append(full)      # do not descend: it is handled whole
            else:
                keep.append(d)
        dirnames[:] = keep
    return found


def _remove_by_suffix(build_dir, suffixes, dry_run):
    """Delete files under build_dir whose name ends with one of `suffixes`.
    Files only — the CMakeFiles/<target>.dir/ tree itself carries the
    dependency graph an incremental rebuild needs. Returns (count, bytes)."""
    count = 0
    freed = 0
    for root, _dirs, files in os.walk(build_dir, topdown=False):
        for name in files:
            low = name.lower()
            hit = False
            si = 0
            while si < len(suffixes):
                if low.endswith(suffixes[si]):
                    hit = True
                    break
                si += 1
            if not hit:
                continue
            fp = os.path.join(root, name)
            try:
                sz = os.path.getsize(fp)
            except OSError:
                continue
            if not dry_run:
                try:
                    os.remove(fp)
                except OSError:
                    continue
            count += 1
            freed += sz
    return count, freed


def newest_mtime(path):
    """Most recent mtime under `path`, 0 when empty/unreadable."""
    newest = 0.0
    for root, _dirs, files in os.walk(path):
        for name in files:
            try:
                st = os.lstat(os.path.join(root, name))
            except OSError:
                continue
            if st.st_mtime > newest:
                newest = st.st_mtime
    return newest


def prune_to_budget(build_dir, budget_bytes, dry_run=False,
                    allow_remove_dir=False, stale_days=14, now=None,
                    log=print):
    """Apply tiers to `build_dir` until it fits in `budget_bytes`.

    A dir nobody has built in `stale_days` gets its scratch dropped even when it
    is UNDER budget: the only reason to keep .o files is a fast incremental
    rebuild, and that value has expired. Measured here, dirs idle 46-61 days were
    sitting on 380 MB of .o purely because each was individually small enough to
    pass the budget test.

    Returns the size left (predicted size when dry_run)."""
    size = dir_size(build_dir)
    if now is None:
        now = time.time()
    newest = newest_mtime(build_dir)
    idle_days = (now - newest) / 86400.0 if newest else 0.0
    stale = stale_days > 0 and newest and idle_days >= stale_days
    log(f"  {build_dir}: {size / 1048576:.0f} MB (idle {idle_days:.0f}d)")
    if size <= budget_bytes and not stale:
        log(f"    already within budget ({budget_bytes / 1048576:.0f} MB) — untouched")
        return size
    if size <= budget_bytes and stale:
        # Nothing here will ever be rebuilt incrementally, so every regenerable
        # tier goes; only the CMake state stays, sparing a reconfigure.
        nb, fb = _remove_linked_outputs(build_dir, dry_run)
        na, fa = _remove_by_suffix(build_dir, _ARCHIVE_SUFFIXES, dry_run)
        ns, fs = _remove_by_suffix(build_dir,
                                   cleanup_helpers._PRUNE_FILE_SUFFIXES, dry_run)
        freed = fb + fa + fs
        size -= freed
        if nb + na + ns:
            log(f"    stale >{stale_days}d: dropped {nb} binar(y|ies), {na} "
                f"archive(s), {ns} scratch file(s) — {freed / 1048576:.0f} MB "
                f"-> {size / 1048576:.0f} MB (CMake state kept)")
        else:
            log(f"    within budget, nothing regenerable left — untouched")
        return size

    n, freed = _remove_linked_outputs(build_dir, dry_run)
    size -= freed
    log(f"    tier 1 linked binaries: {n} file(s), {freed / 1048576:.0f} MB "
        f"-> {size / 1048576:.0f} MB (.o kept: regenerating these is ONE relink)")
    if size <= budget_bytes:
        return size

    n, freed = _remove_by_suffix(build_dir, _ARCHIVE_SUFFIXES, dry_run)
    size -= freed
    log(f"    tier 2 static archives: {n} file(s), {freed / 1048576:.0f} MB "
        f"-> {size / 1048576:.0f} MB (re-archived from the kept .o)")
    if size <= budget_bytes:
        return size

    n, freed = _remove_by_suffix(build_dir, cleanup_helpers._PRUNE_FILE_SUFFIXES,
                                 dry_run)
    size -= freed
    log(f"    tier 3 compiler scratch: {n} file(s), {freed / 1048576:.0f} MB "
        f"-> {size / 1048576:.0f} MB (costs a full recompile to get back)")
    if size <= budget_bytes:
        return size

    if not allow_remove_dir:
        log(f"    still {size / 1048576:.0f} MB > budget: the rest is binaries "
            f"and CMake state. Pass --allow-remove-dir to drop the whole dir "
            f"(costs a full reconfigure + rebuild).")
        return size
    log(f"    tier 4 removing the whole dir ({size / 1048576:.0f} MB)")
    if not dry_run:
        cleanup_helpers.remove_build_dir(build_dir)
    return 0


def main():
    ap = argparse.ArgumentParser(
        description="Progressively trim in-source build dirs to a size budget.")
    default_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ap.add_argument("--root", default=default_root,
                    help=f"tree to scan (default: {default_root})")
    ap.add_argument("--budget-mb", type=int, default=300,
                    help="per-build-dir budget in MB (default: 300)")
    ap.add_argument("--dry-run", action="store_true",
                    help="report what would be freed, delete nothing")
    ap.add_argument("--allow-remove-dir", action="store_true",
                    help="allow tier 3 (drop the whole build dir)")
    ap.add_argument("--stale-days", type=int, default=14,
                    help="a build dir untouched this long gives up its compiler "
                         "scratch even when under budget (0 disables)")
    args = ap.parse_args()

    budget = args.budget_mb * 1048576
    dirs = find_build_dirs(args.root)
    if not dirs:
        print(f"[prune] no in-source build dir under {args.root}")
        return 0
    mode = "DRY-RUN, nothing deleted" if args.dry_run else "deleting"
    print(f"[prune] {len(dirs)} in-source build dir(s) under {args.root} "
          f"({mode}, budget {args.budget_mb} MB each)")
    now = time.time()
    total_before = 0
    total_after = 0
    for d in dirs:
        before = dir_size(d)
        total_before += before
        total_after += prune_to_budget(d, budget, dry_run=args.dry_run,
                                       allow_remove_dir=args.allow_remove_dir,
                                       stale_days=args.stale_days, now=now)
    print(f"[prune] {total_before / 1048576:.0f} MB -> "
          f"{total_after / 1048576:.0f} MB "
          f"({(total_before - total_after) / 1048576:.0f} MB freed)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
