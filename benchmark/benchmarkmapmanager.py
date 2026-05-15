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
import benchmark_remote as br
import history_recorder as hr

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


def _metric_unit_better(metric_name):
    better = "lower"
    unit = "ns" if metric_name.endswith("_ns") else \
           "bytes" if metric_name.endswith("_bytes") or metric_name == "bytes_sent" or metric_name == "binary_size_bytes" else \
           "kb" if metric_name.endswith("_kb") else \
           "s" if metric_name.endswith("_s") else \
           "count"
    return unit, better


def _cell_to_metric_block(per_cell):
    """Convert the {(player, metric): value} cell dict into the per-tool
    `metrics` block expected by history_recorder.PlatformRecord."""
    out = {}
    for (player, metric_name), value in per_cell.items():
        if value is None: continue
        key = metric_name if player == 0 else f"p{player}_{metric_name}"
        unit, better = _metric_unit_better(metric_name)
        out[key] = {"value": value, "unit": unit, "better": better,
                    "samples": [value], "median": value, "stddev": 0.0}
    return out


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


def _runtime_cmd_string():
    """Build the bench binary's argv as a single shell-quoted string for
    use on the exec node. The binary lives in the exec node's work_dir
    after push_binary_to_exec; we invoke it as ./BIN_NAME so cwd is the
    work_dir."""
    parts = [f"./{BIN_NAME}", "--ticks", str(TICKS_PER_RUN),
             "--seed", str(SEED), "--insrem-pct", str(INSREM_PCT)]
    return " ".join(parts)


def run_remote_cells(node, avail_profilers, skips, all_profilers,
                     progress, per_tool, all_metrics):
    """Dispatch the benchmark to a remote exec node: build on the paired
    compile node, rsync binary to exec node, run each profiler. Updates
    `per_tool[label]` and `all_metrics[label]` in place, and emits a
    progress line per profiler.
    """
    label = node["label"]
    compile_node = node.get("compile_node")
    if compile_node is None:
        for prof in all_profilers:
            progress.emit(prof, "no", label, status="SKIP",
                          extra="no-compile-node")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        return

    # exec_node dict matching the remote_nodes.json shape that
    # benchmark_remote expects (uses keys: user, host, port, work_dir).
    exec_node = {"label": label,
                 "user":  node.get("ssh_user"),
                 "host":  node.get("ssh_host"),
                 "port":  node.get("ssh_port", 22),
                 "work_dir": node.get("work_dir") or "/tmp/cc-bench-run"}

    # Only run the profilers that passed the tool check. Pre-emit SKIPs
    # so the progress counter stays in lock-step with all_profilers.
    runnable = [p for p in all_profilers if p in avail_profilers]
    for prof in all_profilers:
        if prof not in runnable:
            reason = skips.get(prof, "tool-missing")
            progress.emit(prof, "no", label, status="SKIP", extra=reason)
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}

    if not runnable:
        return

    out, msg = br.run_benchmark_on_exec(
        compile_node=compile_node,
        exec_node=exec_node,
        cmake_src_subdir="benchmark/benchmarkmapmanager",
        build_subdir=f"benchmarkmapmanager-{label}",
        bin_name=BIN_NAME,
        runtime_cmd=_runtime_cmd_string(),
        profilers=runnable,
        cmake_defs={"CMAKE_BUILD_TYPE": "Release"},
        verbose=True,
    )
    flat = {}
    for prof in runnable:
        res = out.get(prof)
        if res is None or (isinstance(res, dict) and res.get("rc") not in (None, 0)):
            progress.emit(prof, "no", label, status="FAIL", extra=msg if msg != "ok" else "")
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
            continue
        # parse per-profiler structure into metric blocks
        if prof == "rusage":
            cell = {}
            if res.get("max_rss_kb") is not None:
                cell[(0, "max_rss_kb")] = res["max_rss_kb"]
            if res.get("wall_s") is not None:
                cell[(0, "wall_s")] = res["wall_s"]
            if not cell:
                progress.emit(prof, "no", label, status="FAIL", extra="no-data")
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
                continue
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        elif prof == "perf-stat":
            cell = {(0, f"perf_{k}"): v for k, v in res.items()}
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        elif prof == "callgrind":
            cell = {(0, "callgrind_ir"): res}
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        elif prof == "binary-size":
            cell = {(0, "binary_size_bytes"): res}
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        else:
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
        progress.emit(prof, "no", label, status="PASS")
    all_metrics[label] = aggregate_metrics(flat)


def main():
    bin_path = build()
    if bin_path is None:
        return 2

    arch = bh.host_arch()
    nodes = [{"label": "local", "arch": arch}] + bh.benchmark_exec_nodes()
    all_profilers = ["rusage", "binary-size", "perf-stat", "callgrind"]

    # Pre-resolve per-node profiler availability. profilers_runnable_on()
    # honours benchmark_disabled_tools (persisted skips) and probes the
    # node interactively for tools we haven't classified yet; the answer
    # is persisted so we never re-prompt for the same (node,tool).
    node_profilers = {}
    node_skips     = {}
    for node in nodes:
        avail, skips = bh.profilers_runnable_on(node, all_profilers)
        node_profilers[node["label"]] = avail
        node_skips[node["label"]]     = skips

    total = sum(len(all_profilers) for _ in nodes)
    progress = bh.Progress(total)

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release"]

    all_metrics = {}     # node_label -> {flat metric dict}
    per_tool    = {}     # node_label -> { tool -> {status, metrics, ...} }
    for node in nodes:
        label = node["label"]
        per_tool[label] = {}
        if label != "local":
            run_remote_cells(node, node_profilers[label], node_skips[label],
                             all_profilers, progress, per_tool, all_metrics)
            continue
        flat = {}
        for prof in all_profilers:
            if prof not in node_profilers[label]:
                reason = node_skips[label].get(prof, "tool-missing")
                progress.emit(prof, "no", label, status="SKIP", extra=reason)
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
                continue
            cell = cell_run(bin_path, prof, label)
            if cell is None:
                progress.emit(prof, "no", label, status="FAIL")
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
                continue
            flat.update(cell)
            progress.emit(prof, "no", label, status="PASS")
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        all_metrics[label] = aggregate_metrics(flat)

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

    # Per-platform history -- one JSON per (benchmark, run, platform).
    # Per benchmark/CLAUDE.md the file is append-only; never overwritten.
    ended_utc = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    for node in nodes:
        if node["label"] not in per_tool:
            continue
        if node["label"] == "local":
            runner = hr.local_runner
        else:
            runner = hr.make_ssh_runner(node.get("ssh_host"),
                                        node.get("ssh_user"),
                                        node.get("ssh_port", 22))
        pr = hr.PlatformRecord("benchmarkmapmanager", batch_id,
                               node["label"], runner=runner,
                               arch_hint=node.get("arch")).collect()
        for tool, blk in per_tool[node["label"]].items():
            pr.add_result(tool, blk["metrics"], status=blk["status"])
        out_p = pr.write(commit=sha, started_utc=started_utc,
                         ended_utc=ended_utc,
                         compile_flags=compile_flags,
                         simd_tier="generic",
                         harness_version=hr.harness_version())
        print(_color(bh.C_CYAN, f"[history] {out_p}"))

    champ = bh.load_champion("benchmarkmapmanager", arch)
    decision, summary = bh.decide(champ, rec)
    bh.print_decision("benchmarkmapmanager", arch, decision, summary)

    if decision == "KEEP":
        ch_p = bh.champion_path("benchmarkmapmanager", arch)
        bh.write_record(ch_p, rec)
        print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

    hr.attach_decision("benchmarkmapmanager", batch_id, decision)
    import chart_generator
    for cp in chart_generator.regenerate("benchmarkmapmanager"):
        print(_color(bh.C_CYAN, f"[chart] {cp}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
