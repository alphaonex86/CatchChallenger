#!/usr/bin/env python3
"""history_series.py -- one compact, machine-readable time series per node.

benchmark/history/ holds one verbose JSON per run and stays gitignored (1300+
files, 25 MB, growing). This distils each
history/<benchmark>/<platform>/<node>/ into two tracked files:

  series.json    the whole measurement history of that node, compacted
  platform.json  the machine description, rewritten only when it changes

FORMAT (designed to be parsed, not read):

    {
      "benchmark": "benchmarkmapmanager",
      "node": "fitxlan",
      "runs":   [[ended_utc, commit_short, decision, comment], ...],
      "meta":   {"<metric>": [unit, better]},
      "series": {"<metric>": [v0, v1, ...]}
    }

series[m][i] is the value of metric m at runs[i]; null where that run did not
report it. Everything constant is factored out of the inner loop: the unit and
better-direction live once per metric in `meta`, and the timestamp / commit /
decision live once per run in `runs` -- so a new run appends ONE number per
metric instead of a fresh {value, median, stddev, unit, better} block.

Metric keys are flattened the way chart_generator labels them:
    "<tool>.<metric>"            aggregate metrics
    "<tool>.<slice>.<metric>"    sub-benchmark slices

Reading it back is a two-liner:

    d = json.load(open("series.json"))
    points = list(zip((r[0] for r in d["runs"]), d["series"]["rusage.wall_s"]))

Run standalone, or let benchmark/all.sh call it after a suite.
"""
import json
import math
import os
import sys

HISTORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "history")

# Files this script writes -- never mistake them for run records.
GENERATED = frozenset(("series.json", "platform.json", "last.json"))

SIG_DIGITS = 4

# Machine description keys are everything NOT in here; they go to platform.json.
RUN_KEYS = frozenset((
    "benchmark", "node", "commit", "commit_short", "ended_utc", "decision",
    "comment", "harness_version", "compile_flags", "simd_tier", "results",
))

# Dropped outright: run-local noise and bulk that says nothing about
# performance or hardware.
DROP_KEYS = frozenset((
    "kernel_config_gz", "sensors", "noise_services", "loadavg_1min_at_start",
    "batch_id", "started_utc", "cpu_flags",
))


def round_sig(value, digits=SIG_DIGITS):
    """Round a measurement to `digits` significant figures.

    A raw score like 5646546513248 carries ~9 digits of run-to-run noise that
    nothing reads and that guarantees a fresh git blob every run. 4 figures
    still resolve ~0.02%, far below the noise floor of these benchmarks."""
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return value
    if value == 0 or value != value or value in (float("inf"), float("-inf")):
        return value
    exp = math.floor(math.log10(abs(value)))
    quant = digits - 1 - exp
    rounded = round(value, quant)
    if quant <= 0:
        return int(rounded)
    return rounded


def load_runs(node_dir):
    """Every run record in a node dir, chronological."""
    runs = []
    for name in sorted(os.listdir(node_dir)):
        if not name.endswith(".json") or name in GENERATED:
            continue
        try:
            with open(os.path.join(node_dir, name)) as handle:
                runs.append(json.load(handle))
        except (OSError, ValueError):
            continue
    runs.sort(key=lambda r: r.get("ended_utc") or "")
    return runs


def flatten_metrics(record):
    """{metric_key: (value, unit, better)} for one run.

    Prefers `median` over `value` -- the same order chart_generator plots."""
    out = {}
    for tool, blk in (record.get("results") or {}).items():
        if not isinstance(blk, dict):
            continue
        for name, m in (blk.get("metrics") or {}).items():
            if not isinstance(m, dict):
                continue
            val = m.get("median") if m.get("median") is not None else m.get("value")
            if val is None:
                continue
            out[f"{tool}.{name}"] = (round_sig(val), m.get("unit"), m.get("better"))
        for slice_label, smetrics in (blk.get("subbenchmarks") or {}).items():
            if not isinstance(smetrics, dict):
                continue
            for name, m in smetrics.items():
                if not isinstance(m, dict):
                    continue
                val = m.get("median") if m.get("median") is not None else m.get("value")
                if val is None:
                    continue
                out[f"{tool}.{slice_label}.{name}"] = (round_sig(val),
                                                       m.get("unit"),
                                                       m.get("better"))
    return out


def build_series(runs):
    """(runs_axis, meta, series) from the chronological run records."""
    runs_axis = []
    per_run = []
    for record in runs:
        runs_axis.append([
            record.get("ended_utc"),
            record.get("commit_short") or (record.get("commit") or "")[:7] or None,
            record.get("decision"),
            record.get("comment") or None,
        ])
        per_run.append(flatten_metrics(record))

    meta = {}
    for metrics in per_run:
        for key, (_v, unit, better) in metrics.items():
            if key not in meta:
                meta[key] = [unit, better]

    series = {}
    for key in meta:
        column = []
        seen = False
        for metrics in per_run:
            hit = metrics.get(key)
            if hit is None:
                column.append(None)
            else:
                column.append(hit[0])
                seen = True
        if seen:
            series[key] = column
    meta = {k: v for k, v in meta.items() if k in series}
    return runs_axis, meta, series


def derive_normalised(series, meta):
    """Add load-normalised, cell-INDEPENDENT columns to a series read back from
    disk, so every node lands in the same chart panel.

    Each node sizes its own population from its RAM, so its metrics are named
    after a count nobody else ran: `p65530_median_tick_ns` and
    `p5461_median_tick_ns` are two panels holding one machine each, and a chart
    that drops panels covering less than half the fleet drops both. The way out
    is a name with no count in it, carrying the node's largest cell divided by
    its own load:

        own_load_players           N, the largest cell this node could run
        own_load_ns_per_player     p<N>_median_tick_ns / N
        own_load_player_ticks_per_s  p<N>_ticks_per_s * N

    Both ratios are identities, not estimates, which is why deriving them on
    read is right: every run already on disk becomes comparable without
    rewriting a single recorded number.

    Caveat worth keeping in mind while reading such a panel: the per-player cost
    falls as the population grows, because the fixed cost of ticking the whole
    world spreads over more players. So a node that could only afford a small
    cell is charged for that -- these columns compare MACHINES AS CONFIGURED
    (silicon plus the RAM it has), which is the honest question for a fleet
    whose nodes cannot all hold the same load. For silicon alone, read the
    cell every node ran instead.

    Mutates `series`/`meta` in place and returns how many columns it added."""
    added = 0
    by_tool = {}
    for key in series:
        tool, _, metric = key.rpartition(".")
        if metric.startswith("p") and "_" in metric:
            head, _, tail = metric[1:].partition("_")
            if head.isdigit() and int(head) > 0:
                if tail in ("median_tick_ns", "ticks_per_s"):
                    cells = by_tool.setdefault(tool, {})
                    cells.setdefault(int(head), {})[tail] = key
    for tool, cells in by_tool.items():
        players = max(cells)
        prefix = f"{tool}." if tool else ""
        source = cells[players]
        columns = [("own_load_ns_per_player", "median_tick_ns", "ns",
                    1.0 / players),
                   ("own_load_player_ticks_per_s", "ticks_per_s",
                    "player-ticks/s", float(players)),
                   # The two raw figures at that same cell, under a name with
                   # no count in it so they get a panel holding every node.
                   # Read as a CAPABILITY, always paired with
                   # own_load_players: "this box holds M players at T ticks/s"
                   # is what decides whether hardware can serve a world, and it
                   # is the question a raw tick rate answers well. It is only
                   # as a hardware RANKING that it misleads (it counts batches,
                   # so a node holding a twelfth of the population looks fast)
                   # -- that is what own_load_player_ticks_per_s is for.
                   ("own_load_ticks_per_s", "ticks_per_s", "ticks/s", 1.0),
                   ("own_load_median_tick_ns", "median_tick_ns", "ns", 1.0)]
        for name, from_metric, unit, scale in columns:
            key = source.get(from_metric)
            if key is not None and prefix + name not in series:
                series[prefix + name] = [None if v is None else round(v * scale, 2)
                                         for v in series[key]]
                meta[prefix + name] = unit
                added += 1
        # What that population COSTS in memory. max_rss_kb is the peak of the
        # whole run, and the largest cell is where the peak happens, so it is
        # already "RAM at max players" -- divided by the count it becomes the
        # per-player cost, which is the number that says whether a smaller
        # board could hold the same world. It is NOT a constant: it climbs as
        # maps crowd, which is exactly why the sizing model has to assume the
        # worst slope.
        rss = f"{prefix}max_rss_kb"
        name = prefix + "own_load_kb_per_player"
        if rss in series and name not in series:
            series[name] = [None if v is None else round(float(v) / players, 3)
                            for v in series[rss]]
            meta[name] = "kb"
            added += 1
        # Which N the two above were taken at -- without it a reader cannot
        # tell a fast machine from one that simply ran a smaller load.
        if prefix + "own_load_players" not in series:
            any_column = series[next(iter(source.values()))]
            series[prefix + "own_load_players"] = [None if v is None else players
                                                   for v in any_column]
            meta[prefix + "own_load_players"] = "count"
            added += 1
    return added


def platform_of(record):
    """Machine description: every non-empty field that is not run-specific."""
    out = {}
    for key, value in (record or {}).items():
        if key in DROP_KEYS or key in RUN_KEYS:
            continue
        if value is None or value == {} or value == [] or value == "":
            continue
        out[key] = value
    return out


def append_run(record):
    """Append ONE finished run into its node's series.json, and refresh
    platform.json when the machine description changed. Returns the series path.

    This IS the storage now - there is no per-run <stamp>.json any more. A run
    costs one entry in `runs` (ended_utc, commit, decision, comment) plus ONE
    number per metric in `series`; everything constant (unit, machine, libs)
    lives once, outside the per-run loop. A metric seen for the first time is
    back-filled with nulls so every column keeps the length of `runs`.
    """
    import benchmark_helpers as bh
    benchmark = record.get("benchmark")
    node = record.get("node")
    if not benchmark or not node:
        return None
    comp, exe = bh.node_path_parts(node)
    ndir = os.path.join(HISTORY, benchmark, comp, exe)
    os.makedirs(ndir, exist_ok=True)
    spath = os.path.join(ndir, "series.json")

    payload = {"benchmark": benchmark, "node": node,
               "runs": [], "meta": {}, "series": {}}
    if os.path.isfile(spath):
        try:
            with open(spath) as handle:
                loaded = json.load(handle)
            if isinstance(loaded, dict) and isinstance(loaded.get("runs"), list):
                payload = loaded
        except (OSError, ValueError):
            pass   # unreadable/corrupt: start a fresh series rather than lose the run

    runs = payload.setdefault("runs", [])
    meta = payload.setdefault("meta", {})
    series = payload.setdefault("series", {})
    n_before = len(runs)

    runs.append([
        record.get("ended_utc"),
        record.get("commit_short") or (record.get("commit") or "")[:7] or None,
        record.get("decision"),
        record.get("comment") or None,
        record.get("batch_id"),   # which session this run belongs to (cross-node chart)
    ])

    flat = flatten_metrics(record)
    for key, (value, unit, _better) in flat.items():
        # `better` is NOT stored: it is a property of the metric kind, resolved
        # by the viewer (chart_generator.better_for). Keeping it here repeated
        # the same word in every file for no reader.
        if key not in series:
            series[key] = [None] * n_before      # back-fill the runs before it existed
            meta[key] = unit
        elif key not in meta:
            meta[key] = unit
        series[key].append(value)
    for key, column in series.items():
        if key not in flat:
            column.append(None)                  # not reported by this run

    write_if_changed(spath, payload)
    platform = platform_of(record)
    if platform:
        write_if_changed(os.path.join(ndir, "platform.json"), platform)
    return spath


def set_last_decision(benchmark, decision):
    """Stamp `decision` on the newest run of every node of `benchmark` that has
    none yet. The decision is only known after the cross-node champion compare,
    i.e. after append_run() already stored the run."""
    n = 0
    for ndir in node_dirs():
        spath = os.path.join(ndir, "series.json")
        if not os.path.isfile(spath):
            continue
        try:
            with open(spath) as handle:
                payload = json.load(handle)
        except (OSError, ValueError):
            continue
        if payload.get("benchmark") != benchmark:
            continue
        runs = payload.get("runs") or []
        if not runs or runs[-1][2] is not None:
            continue
        runs[-1][2] = decision
        write_if_changed(spath, payload)
        n += 1
    return n


def node_dirs():
    """history/<benchmark>/<platform>/<node>/ -- exactly 3 levels down."""
    found = []
    if not os.path.isdir(HISTORY):
        return found
    for benchmark in sorted(os.listdir(HISTORY)):
        bdir = os.path.join(HISTORY, benchmark)
        if not os.path.isdir(bdir):
            continue
        for platform in sorted(os.listdir(bdir)):
            pdir = os.path.join(bdir, platform)
            if not os.path.isdir(pdir):
                continue
            for node in sorted(os.listdir(pdir)):
                ndir = os.path.join(pdir, node)
                if os.path.isdir(ndir):
                    found.append(ndir)
    return found


def write_if_changed(path, payload):
    """Write only when the bytes differ, so a rerun costs git nothing."""
    text = json.dumps(payload, sort_keys=True, separators=(",", ":")) + "\n"
    try:
        with open(path) as handle:
            if handle.read() == text:
                return False, len(text)
    except (OSError, ValueError):
        pass
    with open(path, "w") as handle:
        handle.write(text)
    return True, len(text)


def main():
    dirs = node_dirs()
    wrote = 0
    total = 0
    for ndir in dirs:
        runs = load_runs(ndir)
        if not runs:
            continue
        runs_axis, meta, series = build_series(runs)
        if not series:
            continue
        latest = runs[-1]
        payload = {
            "benchmark": latest.get("benchmark"),
            "node": latest.get("node"),
            "runs": runs_axis,
            "meta": meta,
            "series": series,
        }
        changed, size = write_if_changed(os.path.join(ndir, "series.json"), payload)
        wrote += 1 if changed else 0
        total += size
        platform = platform_of(latest)
        if platform:
            changed, size = write_if_changed(
                os.path.join(ndir, "platform.json"), platform)
            wrote += 1 if changed else 0
            total += size
        # last.json is superseded by series.json (its final column)
        stale = os.path.join(ndir, "last.json")
        if os.path.isfile(stale):
            os.remove(stale)
    print(f"[history-series] {len(dirs)} node dirs, {wrote} file(s) updated, "
          f"{total} bytes total")
    return 0


if __name__ == "__main__":
    sys.exit(main())
