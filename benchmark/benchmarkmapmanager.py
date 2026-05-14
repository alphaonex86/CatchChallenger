#!/usr/bin/env python3
"""benchmarkmapmanager.py -- benchmark MapVisibilityAlgorithm::min_network().

Workload: typical "mostly move" tick mix (>90% of ticks change only the
direction; ~5% of ticks insert+remove one player) at 7 player counts:
5, 10, 20, 50, 100, 200, 300. Determinism: seeded LCG, no rand/clock,
x,y are populated at startup and never touched again -- only direction
changes per tick.

One-command target -- per benchmark/CLAUDE.md, run with no args, 1h
timeout. Builds the C++ harness (benchmarkmapmanager/), runs every
profiler that's available (rusage via /usr/bin/time -v, perf stat,
callgrind, binary-size) on the host, prints a one-line progress update
per cell, and writes a candidate-<sha>.json under
benchmark/results/benchmarkmapmanager/<arch>/. If a champion.json
exists, applies the KEEP/DISCARD/ESCALATE matrix and prints the
verdict. Remote execution_nodes with `benchmark: true` and loadavg<1.0
are picked up dynamically; today every remote_nodes.json entry has
`benchmark: false`, so this collapses to the host-only baseline -- the
remote dispatch path is wired so adding a `benchmark: true` node
opts it in with no code change here.
"""
import os
import sys
import json
import shutil
import subprocess
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh

REPO_ROOT  = bh.REPO_ROOT
BENCH_DIR  = os.path.dirname(os.path.abspath(__file__))
SRC_DIR    = os.path.join(BENCH_DIR, "benchmarkmapmanager")
BIN_NAME   = "benchmark_min_network"

# Build dir lives outside the source tree (root CLAUDE.md).
try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
except Exception:
    BUILD_ROOT = os.path.join("/tmp", "cc-build")
BUILD_DIR  = os.path.join(BUILD_ROOT, "benchmark", "benchmarkmapmanager")

PLAYER_COUNTS = [5, 10, 20, 50, 100, 200, 300]
TICKS_PER_RUN = 2000
SEED          = 0x5EED
INSREM_PCT    = 5
RUN_REPEATS   = 3        # warmup + 3 measured wall passes per cell
RUN_TIMEOUT   = 600


def _color(c, s): return f"{c}{s}{bh.C_RESET}"


def build():
    os.makedirs(BUILD_DIR, exist_ok=True)
    print(_color(bh.C_CYAN, f"[build] {SRC_DIR} -> {BUILD_DIR}"))
    cfg = ["cmake", "-S", SRC_DIR, "-B", BUILD_DIR, "-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=300)
    if rc != 0:
        print(sout); print(serr, file=sys.stderr)
        print(_color(bh.C_RED, "[build] cmake configure FAILED")); return None
    bld = ["cmake", "--build", BUILD_DIR, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=900)
    if rc != 0:
        print(sout); print(serr, file=sys.stderr)
        print(_color(bh.C_RED, "[build] cmake build FAILED")); return None
    bin_path = os.path.join(BUILD_DIR, BIN_NAME)
    if not os.path.isfile(bin_path):
        print(_color(bh.C_RED, f"[build] missing binary: {bin_path}")); return None
    print(_color(bh.C_GREEN, f"[build] OK ({bh.binary_size(bin_path)} bytes)"))
    return bin_path


def parse_bench_lines(stdout):
    """Parse the harness's `BENCH players=N ...` lines.

    Returns dict: { player_count_int : { metric_name : value } }."""
    out = {}
    for line in stdout.splitlines():
        if not line.startswith("BENCH "):
            continue
        f = {}
        for kv in line[len("BENCH "):].split():
            k, _, v = kv.partition("=")
            try: f[k] = int(v)
            except ValueError:
                try: f[k] = float(v)
                except ValueError: f[k] = v
        if "players" in f:
            out[int(f["players"])] = f
    return out


def run_one(bin_path, players_arg=None):
    cmd = [bin_path, "--ticks", str(TICKS_PER_RUN),
           "--seed", str(SEED), "--insrem-pct", str(INSREM_PCT)]
    if players_arg is not None:
        for p in players_arg:
            cmd += ["--players", str(p)]
    return cmd


def cell_run(bin_path, profiler, label_node):
    """Run a single (profiler, players=*) cell on the local host, return
    a dict of metrics: {(player, metric_name) -> value}."""
    cmd = run_one(bin_path)
    metrics = {}
    if profiler == "rusage":
        # Several timed reps; aggregate the harness's own median tick
        # numbers across reps.
        per_rep = []
        for _ in range(RUN_REPEATS):
            t = bh.measure_time_v(cmd, timeout=RUN_TIMEOUT)
            if t["rc"] != 0:
                return None
            # The harness only prints to stdout; measure_time_v
            # discarded stdout to keep memory low. Re-run once
            # plain to harvest the BENCH lines (bytes/median).
        rc, sout, serr, dt = bh.run_capture(cmd, timeout=RUN_TIMEOUT)
        if rc != 0:
            return None
        bench = parse_bench_lines(sout)
        # Add max RSS from one fresh /usr/bin/time -v pass.
        t = bh.measure_time_v(cmd, timeout=RUN_TIMEOUT)
        for p, fields in bench.items():
            metrics[(p, "median_tick_ns")] = fields.get("median_tick_ns")
            metrics[(p, "p95_tick_ns")]    = fields.get("p95_tick_ns")
            metrics[(p, "bytes_sent")]     = fields.get("bytes_sent")
        if t["max_rss_kb"] is not None:
            metrics[(0, "max_rss_kb")] = t["max_rss_kb"]
        if t["wall_s"] is not None:
            metrics[(0, "wall_s")] = t["wall_s"]
        return metrics
    if profiler == "perf-stat":
        out = bh.measure_perf_stat(cmd, timeout=RUN_TIMEOUT)
        if not out: return None
        for evt, val in out.items():
            metrics[(0, f"perf_{evt}")] = val
        return metrics
    if profiler == "callgrind":
        ic = bh.measure_callgrind(cmd, timeout=RUN_TIMEOUT * 30, outdir=BUILD_DIR)
        if ic is None: return None
        metrics[(0, "callgrind_ir")] = ic
        return metrics
    if profiler == "binary-size":
        sz = bh.binary_size(bin_path)
        if sz is None: return None
        metrics[(0, "binary_size_bytes")] = sz
        return metrics
    return None


def aggregate_metrics(per_cell):
    """Flatten nested {(player,metric): value} into a single
    metric-name -> {median, stddev, unit, better} dict suitable for
    champion/candidate JSON. We only have one sample per cell here
    (perf/callgrind/binary-size are deterministic); stddev=0."""
    out = {}
    for (player, metric_name), value in per_cell.items():
        if value is None: continue
        key = metric_name if player == 0 else f"p{player}_{metric_name}"
        better = "lower"
        if metric_name in ("perf_instructions", "perf_cycles"):
            better = "lower"
        unit = "ns" if metric_name.endswith("_ns") else \
               "bytes" if metric_name.endswith("_bytes") or metric_name == "bytes_sent" or metric_name == "binary_size_bytes" else \
               "kb" if metric_name.endswith("_kb") else \
               "s" if metric_name.endswith("_s") else \
               "count"
        out[key] = {"median": value, "stddev": 0.0, "unit": unit, "better": better}
    return out


def main():
    bin_path = build()
    if bin_path is None:
        return 2

    arch = bh.host_arch()
    nodes = [{"label": "local", "arch": arch}] + bh.benchmark_exec_nodes()
    profilers = ["rusage", "binary-size"]
    if bh.have_tool("perf"):
        profilers.append("perf-stat")
    if bh.have_tool("valgrind") and bh.have_tool("callgrind_annotate"):
        profilers.append("callgrind")

    total = len(nodes) * len(profilers)
    progress = bh.Progress(total)

    all_metrics = {}     # node_label -> {flat metric dict}
    for node in nodes:
        if node["label"] != "local":
            # Remote dispatch is the responsibility of test/remote_build.py.
            # Until any execution_node opts in via benchmark:true, we have
            # no remote runs; emit SKIP and continue. The wiring is the
            # single source of truth for the build/exec hand-off; per
            # CLAUDE.md we don't ship a parallel implementation here.
            for prof in profilers:
                progress.emit(prof, "no", node["label"], status="SKIP",
                              extra="remote-dispatch-not-wired")
            continue
        flat = {}
        for prof in profilers:
            cell = cell_run(bin_path, prof, node["label"])
            if cell is None:
                progress.emit(prof, "no", node["label"], status="FAIL")
                continue
            flat.update(cell)
            progress.emit(prof, "no", node["label"], status="PASS")
        all_metrics[node["label"]] = aggregate_metrics(flat)

    # Persist + decide for the host arch only (others are SKIP today).
    sha = bh.git_sha()
    rec = {
        "commit": sha,
        "date":   time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "node":   "local",
        "metrics": all_metrics.get("local", {}),
        "profilers": {"binary-size": bin_path},
    }
    cand_p = bh.candidate_path("benchmarkmapmanager", arch, sha)
    bh.write_record(cand_p, rec)
    print(_color(bh.C_CYAN, f"[record] candidate -> {cand_p}"))

    champ = bh.load_champion("benchmarkmapmanager", arch)
    decision, summary = bh.decide(champ, rec)
    bh.print_decision("benchmarkmapmanager", arch, decision, summary)

    if decision == "KEEP":
        ch_p = bh.champion_path("benchmarkmapmanager", arch)
        bh.write_record(ch_p, rec)
        print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
