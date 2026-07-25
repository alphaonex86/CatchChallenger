#!/usr/bin/env python3
"""prune_charts.py -- keep only the newest candidate chart per folder.

Every benchmark run writes a candidate-<stamp>.svg beside the champion chart,
so results/ accumulates them forever: 1557 files / 652 MB of the 700 MB tree at
the time this was written, all of it reconstructible from the candidate/history
JSONs that stay on disk.

This keeps, per directory, the NEWEST candidate-*.svg and removes the older
ones. It never touches:
  * champion*.svg      -- the current chart, and the one tracked in git
  * *.json             -- candidate/champion records are the source of truth
                          a chart can be regenerated from (chart_generator.py)
  * anything outside benchmark/results/

DRY RUN BY DEFAULT. Pass --apply to actually delete. Deletions are done with
os.remove() on explicit, individually-listed paths -- never a shell wildcard
(root CLAUDE.md: look before you delete, no wildcards in a delete command).

  python3 prune_charts.py            # list what would go
  python3 prune_charts.py --apply    # remove them
  python3 prune_charts.py --keep 3   # keep the newest 3 per folder
"""
import argparse
import os
import sys

RESULTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")
PREFIX = "candidate-"
SUFFIX = ".svg"


def candidates_by_dir(root):
    """{dir: [filenames]} for candidate-*.svg only."""
    found = {}
    for dirpath, _dirnames, filenames in os.walk(root):
        names = [n for n in filenames
                 if n.startswith(PREFIX) and n.endswith(SUFFIX)]
        if names:
            found[dirpath] = names
    return found


def newest_first(dirpath, names):
    """Sort newest-first. The stamp in candidate-<ISO>.svg sorts
    lexicographically in chronological order; mtime is the tie-break and the
    fallback for any hand-renamed file."""
    def key(name):
        try:
            mtime = os.path.getmtime(os.path.join(dirpath, name))
        except OSError:
            mtime = 0.0
        return (name, mtime)
    return sorted(names, key=key, reverse=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--apply", action="store_true",
                    help="actually delete (default: dry run)")
    ap.add_argument("--keep", type=int, default=1,
                    help="how many newest candidate charts to keep per folder")
    args = ap.parse_args()

    if args.keep < 0:
        print("--keep must be >= 0", file=sys.stderr)
        return 2
    if not os.path.isdir(RESULTS):
        print(f"[prune-charts] no results dir at {RESULTS}")
        return 0

    doomed = []
    freed = 0
    for dirpath, names in sorted(candidates_by_dir(RESULTS).items()):
        for name in newest_first(dirpath, names)[args.keep:]:
            path = os.path.join(dirpath, name)
            try:
                freed += os.path.getsize(path)
            except OSError:
                pass
            doomed.append(path)

    if not doomed:
        print(f"[prune-charts] nothing to prune (keep={args.keep})")
        return 0

    print(f"[prune-charts] {len(doomed)} obsolete candidate chart(s), "
          f"{freed / 1048576:.1f} MB, keeping the newest {args.keep} per folder")
    if not args.apply:
        for path in doomed[:5]:
            print("  would remove " + os.path.relpath(path, RESULTS))
        if len(doomed) > 5:
            print(f"  ... and {len(doomed) - 5} more")
        print("[prune-charts] DRY RUN — pass --apply to delete")
        return 0

    removed = 0
    for path in doomed:
        try:
            os.remove(path)
            removed += 1
        except OSError as ex:
            print(f"  skip {path}: {ex}", file=sys.stderr)
    print(f"[prune-charts] removed {removed} file(s), {freed / 1048576:.1f} MB freed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
