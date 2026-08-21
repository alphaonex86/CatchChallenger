#!/usr/bin/env python3
"""Rank the fleet's hardware from the recorded history of one benchmark.

    python3 benchmark/compare_nodes.py                     # every benchmark
    python3 benchmark/compare_nodes.py benchmarkmapmanager2
    python3 benchmark/compare_nodes.py benchmarkmapmanager2 --run 3

Two questions, two tables, because they are not the same question:

  SAME LOAD    every node that ran the same workload point (the reference
               cell) side by side. This is the honest speed comparison: same
               players, same maps, same vectors, so the only variable left is
               the machine.

  OWN LOAD     each node at ITS largest cell. The populations differ (each
               node's RAM decides its own), so the raw times are not
               comparable and the table ranks on the normalised work rate
               instead -- how much of the benchmark's unit of work the machine
               retires per second, which is what "this box is bigger" means.

Reads only benchmark/history/<benchmark>/<compile-node>/<exec-node>/, so it
needs no fleet, no build and no network: it is a view over what past runs
already measured.
"""

import os
import sys
import json

HERE = os.path.dirname(os.path.abspath(__file__))
HISTORY = os.path.join(HERE, "history")

# The work-rate metrics, in the order they are preferred for ranking the
# OWN-LOAD table. Each is higher-is-better and already divided by the load, so
# two nodes running different sizes still compare.
RATE_METRICS = ("player_ticks_per_s", "ticks_per_s", "ops_per_s", "req_per_s")


def _load(path):
    """Read a JSON file, or return None after saying why -- a node with a
    damaged history must not take the whole comparison down with it."""
    try:
        with open(path, "r") as handle:
            return json.load(handle)
    except (OSError, ValueError) as e:
        print(f"[warn] {path}: {e}", file=sys.stderr)
        return None


def collect(benchmark, run_index):
    """Gather {node_label: {"platform":…, "values":{key: value}, …}} for one
    benchmark, taking each node's run at `run_index` (-1 = its latest)."""
    root = os.path.join(HISTORY, benchmark)
    nodes = {}
    if not os.path.isdir(root):
        return nodes
    for compile_node in sorted(os.listdir(root)):
        compile_dir = os.path.join(root, compile_node)
        if os.path.isdir(compile_dir):
            for exec_node in sorted(os.listdir(compile_dir)):
                node_dir = os.path.join(compile_dir, exec_node)
                series_path = os.path.join(node_dir, "series.json")
                if os.path.isfile(series_path):
                    series = _load(series_path)
                    if series is not None:
                        runs = series.get("runs") or []
                        columns = series.get("series") or {}
                        if runs and columns:
                            index = run_index if run_index >= 0 else len(runs) + run_index
                            if 0 <= index < len(runs):
                                values = {}
                                for key, column in columns.items():
                                    if index < len(column) and column[index] is not None:
                                        values[key] = column[index]
                                platform = _load(os.path.join(node_dir, "platform.json")) or {}
                                nodes[exec_node] = {
                                    "compile": compile_node,
                                    "platform": platform,
                                    "values": values,
                                    "run": runs[index],
                                }
                            else:
                                print(f"[warn] {exec_node}: no run at index "
                                      f"{run_index} (it has {len(runs)})",
                                      file=sys.stderr)
    return nodes


def _cells(values):
    """Player counts this node measured, read back from its `p<N>_` keys."""
    found = set()
    for key in values:
        metric = key.split(".", 1)[-1]
        if metric.startswith("p") and "_" in metric:
            head = metric[1:].split("_", 1)[0]
            if head.isdigit():
                found.add(int(head))
    return found


def reference_cell(nodes):
    """The workload point measured by the most nodes -- the common ground the
    SAME-LOAD table stands on. Ties break towards the larger cell: more load
    means the per-map floor matters less."""
    tally = {}
    for info in nodes.values():
        for cell in _cells(info["values"]):
            tally[cell] = tally.get(cell, 0) + 1
    if not tally:
        return None, 0
    best = sorted(tally.items(), key=lambda kv: (kv[1], kv[0]))[-1]
    return best[0], best[1]


def _raw(values, metric, cell=None):
    """Look a metric up whatever tool recorded it (`rusage.p1000_x`, `x`, …)."""
    want = f"p{cell}_{metric}" if cell is not None else metric
    for key, value in values.items():
        if key.split(".", 1)[-1] == want:
            return value
    return None


def _get(values, metric, cell=None):
    """As _raw, but derives the two normalised metrics when a run predates
    them. They are exact identities, not estimates -- player_ticks_per_s IS
    ticks_per_s x players and ns_per_player IS median_tick_ns / players -- so
    deriving them makes every run already on disk comparable instead of
    waiting for the fleet to be measured again."""
    value = _raw(values, metric, cell)
    if value is None and cell:
        if metric == "player_ticks_per_s":
            rate = _raw(values, "ticks_per_s", cell)
            if rate is not None:
                return float(rate) * cell
        elif metric == "ns_per_player":
            median = _raw(values, "median_tick_ns", cell)
            if median is not None:
                return float(median) / cell
    return value


def _fmt(value, width, digits=0):
    if value is None:
        return "-".rjust(width)
    if isinstance(value, float) and digits == 0 and abs(value) < 1000:
        digits = 1
    return f"{value:>{width},.{digits}f}"


def print_same_load(nodes, cell, count):
    metrics = [("median_tick_ns", "median_ns", 12, 0),
               ("p95_tick_ns",    "p95_ns",    11, 0),
               ("cpu_percent",    "cpu%",       6, 1),
               ("ns_per_player",  "ns/plyr",    9, 0)]
    rows = []
    for label, info in nodes.items():
        values = info["values"]
        primary = _get(values, "median_tick_ns", cell)
        if primary is not None:
            plat = info["platform"]
            rows.append((primary, label, plat,
                         [_get(values, m[0], cell) for m in metrics]))
    if not rows:
        print(f"  no node recorded the {cell}-player cell")
        return
    rows.sort()
    print(f"\n  SAME LOAD -- {cell} players, the cell {count} of "
          f"{len(nodes)} nodes ran (fastest first)")
    header = f"  {'node':<16}{'arch':<9}{'MHz':>7}  {'cpu':<22}"
    for _, title, width, _d in metrics:
        header += f"{title:>{width+2}}"
    print(header)
    for _, label, plat, cells in rows:
        line = (f"  {label:<16}{str(plat.get('arch') or '-'):<9}"
                f"{_fmt(plat.get('cpu_mhz'), 7, 0)}  "
                f"{str(plat.get('cpu_model') or '-')[:22]:<22}")
        for value, (_m, _t, width, digits) in zip(cells, metrics):
            line += "  " + _fmt(value, width, digits)
        print(line)


def print_own_load(nodes):
    rows = []
    for label, info in nodes.items():
        values = info["values"]
        cells = _cells(values)
        if cells:
            big = max(cells)
            rate = None
            rate_name = None
            for candidate in RATE_METRICS:
                rate = _get(values, candidate, big)
                if rate is not None:
                    rate_name = candidate
                    break
            if rate_name in ("ticks_per_s", "ops_per_s", "req_per_s"):
                # A raw rate counts BATCHES, and a node whose RAM only holds a
                # twelfth of the population retires a batch far more often
                # while doing far less work. Ranking on it would put the
                # 133 MHz Pentium above a Raspberry Pi 3.
                rate, rate_name = None, None
            rows.append((rate if rate is not None else -1.0, label,
                         info["platform"], big, rate, rate_name,
                         _get(values, "median_tick_ns", big),
                         _get(values, "ns_per_player", big),
                         _get(values, "max_rss_kb")))
    if not rows:
        return
    rows.sort(reverse=True)
    name = next((r[5] for r in rows if r[5]), "rate")
    print(f"\n  OWN LOAD -- each node at its own largest cell "
          f"(ranked on {name}, which is per-player so the sizes still compare)")
    print(f"  {'node':<16}{'players':>9}{'median_ns':>13}{'ns/plyr':>10}"
          f"{name:>19}{'peak RSS MB':>13}")
    for _sort, label, _plat, big, rate, _rn, median, per_player, rss in rows:
        print(f"  {label:<16}{big:>9,}{_fmt(median, 13, 0)}"
              f"{_fmt(per_player, 10, 0)}{_fmt(rate, 19, 0)}"
              f"{_fmt(rss / 1024.0 if rss else None, 13, 1)}")


def compare(benchmark, run_index):
    nodes = collect(benchmark, run_index)
    if not nodes:
        print(f"[{benchmark}] no history recorded yet")
        return False
    print(f"\n[{benchmark}] {len(nodes)} nodes")
    for label in sorted(nodes):
        run = nodes[label]["run"]
        stamp = run[0] if run else "?"
        commit = run[1] if run and len(run) > 1 else "?"
        print(f"    {label:<16} {stamp}  {commit}")
    cell, count = reference_cell(nodes)
    if cell is not None:
        print_same_load(nodes, cell, count)
    else:
        print("  no per-player cells in this benchmark's history: nothing to "
              "line up on a common load")
    print_own_load(nodes)
    return True


def main(argv):
    run_index = -1
    names = []
    index = 1
    while index < len(argv):
        if argv[index] == "--run" and index + 1 < len(argv):
            try:
                run_index = int(argv[index + 1])
            except ValueError:
                print(f"--run wants an integer, got {argv[index + 1]!r}",
                      file=sys.stderr)
                return 2
            index += 2
        else:
            names.append(argv[index])
            index += 1
    if not names:
        if os.path.isdir(HISTORY):
            names = sorted(n for n in os.listdir(HISTORY)
                           if os.path.isdir(os.path.join(HISTORY, n)))
        if not names:
            print(f"no history under {HISTORY}", file=sys.stderr)
            return 1
    any_output = False
    for name in names:
        if compare(name, run_index):
            any_output = True
    print()
    return 0 if any_output else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
