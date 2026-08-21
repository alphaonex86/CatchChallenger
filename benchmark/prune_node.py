#!/usr/bin/env python3
"""prune_node.py -- drop every recorded result of one execution node.

    python3 benchmark/prune_node.py <node-label> [--apply] [--why "..."]

For when a node stops being the machine that produced its numbers: a CPU
clocked down, RAM added, a board replaced, a governor changed. Its history is
then not a timeline any more, it is two machines plotted as one -- and its
champion entry is worse than useless, because the next run is compared against
hardware that no longer exists and reports a regression that never happened.

Removes, for that label:
  * benchmark/history/<benchmark>/<compile-node>/<label>/     (the timeline)
  * benchmark/results/<benchmark>/<compile-node>/<label>/     (its charts)
  * the node's entry inside every benchmark/results/*/champion.json

Other nodes are untouched: a champion keeps its remaining nodes, so their
comparisons keep working. The pruned node simply has no baseline until a
future run promotes a new champion.

Dry-run by default -- it prints exactly what it would remove. Pass --apply to
do it.
"""
import json
import os
import shutil
import sys

BENCH_DIR = os.path.dirname(os.path.abspath(__file__))


def node_dirs(root, label):
    """<root>/<benchmark>/<compile-node>/<label> directories that exist."""
    found = []
    if not os.path.isdir(root):
        return found
    for bench in sorted(os.listdir(root)):
        bdir = os.path.join(root, bench)
        if not os.path.isdir(bdir):
            continue
        for compile_node in sorted(os.listdir(bdir)):
            cand = os.path.join(bdir, compile_node, label)
            if os.path.isdir(cand):
                found.append(cand)
    return found


def champions_with(label):
    """champion.json files carrying an entry for that node."""
    found = []
    root = os.path.join(BENCH_DIR, "results")
    if not os.path.isdir(root):
        return found
    for bench in sorted(os.listdir(root)):
        path = os.path.join(root, bench, "champion.json")
        if not os.path.isfile(path):
            continue
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, ValueError) as e:
            print(f"  ! cannot read {path}: {e}", file=sys.stderr)
            continue
        if label in (data.get("nodes") or {}):
            found.append((path, data))
    return found


def main():
    args = [a for a in sys.argv[1:]]
    apply_it = "--apply" in args
    args = [a for a in args if a != "--apply"]
    why = ""
    if "--why" in args:
        i = args.index("--why")
        if i + 1 < len(args):
            why = args[i + 1]
        args = args[:i] + args[i + 2:]
    if len(args) != 1:
        print(__doc__)
        return 2
    label = args[0]

    hist = node_dirs(os.path.join(BENCH_DIR, "history"), label)
    res  = node_dirs(os.path.join(BENCH_DIR, "results"), label)
    champs = champions_with(label)
    if not hist and not res and not champs:
        print(f"nothing recorded for {label!r}")
        return 0

    print(f"{'REMOVING' if apply_it else 'would remove'} every result of "
          f"{label!r}{(' -- ' + why) if why else ''}")
    for d in hist:
        n = sum(len(f) for _, _, f in os.walk(d))
        print(f"  history  {os.path.relpath(d, BENCH_DIR)}  ({n} file(s))")
    for d in res:
        n = sum(len(f) for _, _, f in os.walk(d))
        print(f"  results  {os.path.relpath(d, BENCH_DIR)}  ({n} file(s))")
    for path, data in champs:
        metrics = (data["nodes"][label].get("metrics") or {})
        others = [n for n in data.get("nodes", {}) if n != label]
        print(f"  champion {os.path.relpath(path, BENCH_DIR)}  "
              f"({len(metrics)} metric(s); {len(others)} other node(s) kept)")
    if not apply_it:
        print("\ndry run -- pass --apply to remove")
        return 0

    for d in hist + res:
        shutil.rmtree(d)
    for path, data in champs:
        del data["nodes"][label]
        if why:
            note = data.get("pruned_nodes") or {}
            note[label] = why
            data["pruned_nodes"] = note
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, sort_keys=True)
            f.write("\n")
    print(f"\nremoved {len(hist)} history dir(s), {len(res)} results dir(s), "
          f"and {len(champs)} champion entr(y/ies)")
    print("the node has no baseline now: it re-enters the champion when a "
          "future run promotes one")
    return 0


if __name__ == "__main__":
    sys.exit(main())
