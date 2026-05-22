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

# Concurrency marker: pure in-process visibility loop, no port bind / no
# network (the binary name is historical). Safe to run in parallel.
NETWORK_EXCLUSIVE = False

# Build dir lives outside the source tree (root CLAUDE.md).
try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
except Exception:
    BUILD_ROOT = os.path.join("/tmp", "cc-build")
BUILD_DIR  = os.path.join(BUILD_ROOT, "benchmark", "benchmarkmapmanager")

PLAYER_COUNTS = [5, 10, 20, 50, 100, 200, 300]
SEED          = 0x5EED
INSREM_PCT    = 5
RUN_REPEATS   = 3        # warmup + 3 measured wall passes per cell

# Fixed-TIME model (benchmark/CLAUDE.md): the wall-time / throughput
# profilers (rusage, perf-stat) run each player-count for a fixed budget
# and report ticks completed (higher-is-better) -- NOT a fixed tick count
# timed to completion. Per-player-count budget in ms; total binary wall =
# MAP_BENCH_MS * len(PLAYER_COUNTS).
MAP_BENCH_MS  = 2000
# Callgrind's metric is a DETERMINISTIC instruction count, which a wall
# budget would make vary run-to-run, so callgrind alone stays fixed-
# iteration with a small tick count.
MAP_CALLGRIND_TICKS = 200

# Timeout DERIVED from the budget (timeout = budget + margin), not picked
# independently. The binary self-stops at the budget; the timeout is only
# the hard backstop for a hung run, so it must exceed total budget wall.
_BUDGET_WALL_S = (MAP_BENCH_MS / 1000.0) * len(PLAYER_COUNTS)
RUN_TIMEOUT    = int(_BUDGET_WALL_S * 3) + 120   # margin: warmup+startup+repeats
# Callgrind inflates wall ~30x; scale its timeout by the same factor so it
# measures the same WORK (its tick count is fixed, see MAP_CALLGRIND_TICKS).
RUN_TIMEOUT_CALLGRIND = RUN_TIMEOUT * 30


def _color(c, s): return f"{c}{s}{bh.C_RESET}"


def build():
    os.makedirs(BUILD_DIR, exist_ok=True)
    print(_color(bh.C_CYAN, f"[build] {SRC_DIR} -> {BUILD_DIR}"))
    cfg = ["cmake", "-S", SRC_DIR, "-B", BUILD_DIR, "-DCMAKE_BUILD_TYPE=Release",
           # Enable the CALLGRIND_TOGGLE_COLLECT markers so callgrind (cell +
           # --profile) measures the work loop, not startup. CMake auto-probes
           # the valgrind header, so this is a no-op build-wise where the
           # header is absent. Negligible at runtime off-callgrind (2 marker
           # nops per run).
           "-DCATCHCHALLENGER_BENCH_CALLGRIND=ON"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    cfg += bh.cmake_accel_defs()
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


def _mode_args(profiler):
    """Per-profiler workload-size args. Throughput profilers use the
    fixed-TIME budget (--ms); callgrind uses a fixed deterministic tick
    count (--ticks)."""
    if profiler == "callgrind":
        return ["--ticks", str(MAP_CALLGRIND_TICKS)]
    return ["--ms", str(MAP_BENCH_MS)]


def run_one(bin_path, profiler="rusage", players_arg=None):
    cmd = [bin_path, *_mode_args(profiler),
           "--seed", str(SEED), "--insrem-pct", str(INSREM_PCT)]
    if players_arg is not None:
        for p in players_arg:
            cmd += ["--players", str(p)]
    return cmd


def cell_run(bin_path, profiler, label_node):
    """Run a single (profiler, players=*) cell on the local host.
    Returns (metrics_dict, error_msg) where metrics_dict is None on failure
    and error_msg contains the failure reason."""
    cmd = run_one(bin_path, profiler)
    run_cwd = os.path.dirname(bin_path)
    timeout = RUN_TIMEOUT_CALLGRIND if profiler == "callgrind" else RUN_TIMEOUT
    metrics = {}
    if profiler == "rusage":
        for _ in range(RUN_REPEATS):
            t = bh.measure_time_v(cmd, timeout=timeout, cwd=run_cwd)
            if t["rc"] != 0:
                return None, t.get("error") or f"time -v exited with code {t['rc']}"
        rc, sout, serr, dt = bh.run_capture(cmd, timeout=timeout,
                                            cwd=run_cwd,
                                            preexec_fn=bh._drop_core_rlimit)
        if rc != 0:
            return None, f"benchmark binary exited with code {rc}"
        bench = parse_bench_lines(sout)
        t = bh.measure_time_v(cmd, timeout=timeout, cwd=run_cwd)
        for p, fields in bench.items():
            # Fixed-time headline: throughput (ticks/s, higher-is-better)
            # + the work done (ticks completed). Latency stays per-tick.
            metrics[(p, "ticks_per_s")]    = fields.get("ticks_per_s")
            metrics[(p, "ticks")]          = fields.get("ticks")
            metrics[(p, "median_tick_ns")] = fields.get("median_tick_ns")
            metrics[(p, "p95_tick_ns")]    = fields.get("p95_tick_ns")
            metrics[(p, "bytes_sent")]     = fields.get("bytes_sent")
        if t["max_rss_kb"] is not None:
            metrics[(0, "max_rss_kb")] = t["max_rss_kb"]
        if t["wall_s"] is not None:
            metrics[(0, "wall_s")] = t["wall_s"]
        if not metrics:
            return None, "no metrics captured"
        return metrics, None
    if profiler == "perf-stat":
        out = bh.measure_perf_stat(cmd, timeout=timeout, cwd=run_cwd)
        if not out:
            return None, "SKIP:" + bh.perf_no_hw_skip("local")
        for evt, val in out.items():
            metrics[(0, f"perf_{evt}")] = val
        return metrics, None
    if profiler == "callgrind":
        # collect_atstart=False: the binary's CALLGRIND_TOGGLE_COLLECT
        # markers bound the timed loop, so the IR count excludes process
        # startup (dynamic linking dominates an otherwise-tiny run).
        ic = bh.measure_callgrind(cmd, timeout=timeout, outdir=BUILD_DIR,
                                  collect_atstart=False)
        if ic is None:
            # measure_callgrind already printed the diagnostic banner
            # (AVX-512/unhandled-insn case) and may have auto-disabled
            # the tool. The terse FAIL summary still goes into the
            # per-cell history so the operator can grep for it.
            return None, ("callgrind captured no data "
                          "(see stderr for valgrind diagnostics; common "
                          "cause: host ld-linux uses AVX-512 EVEX "
                          "valgrind cannot decode)")
        metrics[(0, "callgrind_ir")] = ic
        return metrics, None
    if profiler == "binary-size":
        sz = bh.binary_size(bin_path)
        if sz is None:
            return None, "binary-size failed"
        metrics[(0, "binary_size_bytes")] = sz
        return metrics, None
    return None, f"unknown profiler: {profiler}"


def probe_cpu_percent_per_player(bin_path):
    """Run the bench binary once per PLAYER_COUNTS entry (single value
    via --players N) and derive cpu_percent = (user+sys)/wall*100 from
    /usr/bin/time -v. Returns {N -> cpu_percent}.

    The main sweep runs all player counts in a single process so we
    can't tease apart per-count CPU there. The probe is MAP_BENCH_MS per
    count (fixed-time), cheap relative to the sweep + callgrind cells."""
    out = {}
    for p in PLAYER_COUNTS:
        cmd = run_one(bin_path, players_arg=[p])
        t = bh.measure_time_v(cmd, timeout=RUN_TIMEOUT)
        if t.get("rc") != 0:
            continue
        wall = t.get("wall_s") or 0.0
        u    = t.get("user_s")
        sv   = t.get("sys_s")
        if wall <= 0 or u is None or sv is None:
            continue
        pct = (u + sv) / wall * 100.0
        # Single-threaded binary -> bounded at 100. >100 implies the
        # binary briefly spawned a worker (or scheduler jitter); clamp.
        out[p] = min(100.0, pct)
    return out


def _metric_unit_better(metric_name):
    # Throughput is higher-is-better; everything else lower-is-better.
    better = "higher" if metric_name in ("ticks_per_s", "ticks") else "lower"
    unit = "ticks/s" if metric_name == "ticks_per_s" else \
           "ns" if metric_name.endswith("_ns") else \
           "bytes" if metric_name.endswith("_bytes") or metric_name == "bytes_sent" or metric_name == "binary_size_bytes" else \
           "kb" if metric_name.endswith("_kb") else \
           "s" if metric_name.endswith("_s") else \
           "%" if metric_name == "cpu_percent" or metric_name.endswith("_cpu_percent") else \
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
        unit, better = _metric_unit_better(metric_name)
        out[key] = {"median": value, "stddev": 0.0, "unit": unit, "better": better}
    return out


def _runtime_cmd_string(profiler="rusage"):
    """Build the bench binary's argv as a single shell-quoted string for
    use on the exec node. The binary lives in the exec node's work_dir
    after push_binary_to_exec; we invoke it as ./BIN_NAME so cwd is the
    work_dir. Per-profiler workload mode: fixed-time (--ms) for throughput
    profilers, fixed-iteration (--ticks) for callgrind."""
    parts = [f"./{BIN_NAME}", *_mode_args(profiler),
             "--seed", str(SEED), "--insrem-pct", str(INSREM_PCT)]
    return " ".join(parts)


def _remote_spec(node, avail_profilers, skips, all_profilers,
                 progress, per_tool):
    """Build the run_profiler_fleet spec for one remote exec node. Emits
    the SKIP progress lines (no-compile-node / tool-missing) up front so
    the counter stays in lock-step, then returns the spec dict -- or None
    when there's nothing to run (no compile node / no runnable profiler)."""
    label = node["label"]
    compile_node = node.get("compile_node")
    if compile_node is None:
        for prof in all_profilers:
            progress.emit(prof, "no", label, status="SKIP",
                          extra="no-compile-node")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        return None
    exec_node = {"label": label,
                 "user":  node.get("ssh_user"),
                 "host":  node.get("ssh_host"),
                 "port":  node.get("ssh_port", 22),
                 "work_dir": node.get("work_dir") or "/tmp/cc-bench-run",
                 "lxc_nfs": node.get("lxc_nfs"),
                 "ninja":  node.get("ninja")}
    runnable = [p for p in all_profilers if p in avail_profilers]
    for prof in all_profilers:
        if prof not in runnable:
            reason = skips.get(prof, "tool-missing")
            progress.emit(prof, "no", label, status="SKIP", extra=reason)
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
    if not runnable:
        return None
    return {
        "exec_node":         exec_node,
        "compile_node":      compile_node,
        "cmake_src_subdir":  "benchmark/benchmarkmapmanager",
        "build_subdir_base": "benchmarkmapmanager",
        "bin_name":          BIN_NAME,
        # Per-profiler workload mode: fixed-time (--ms) for rusage/perf-stat,
        # fixed-iteration (--ticks) for callgrind.
        "runtime_cmd":       {p: _runtime_cmd_string(p) for p in runnable},
        "profilers":         runnable,
        "cmake_defs":        {"CMAKE_BUILD_TYPE": "Release",
                              "CATCHCHALLENGER_BENCH_CALLGRIND": "ON"},
        # The remote binary carries the CALLGRIND_TOGGLE_COLLECT markers,
        # so remote callgrind must run with --collect-atstart=no or the
        # toggle inverts (collects everything-but-the-loop).
        "callgrind_collect_atstart": False,
        "run_timeout":       RUN_TIMEOUT,
    }


def _record_remote_result(label, runnable, out, msg,
                          progress, per_tool, all_metrics):
    """Parse one exec node's run_profiler_fleet result (out, msg) into
    per_tool/all_metrics + emit per-profiler progress. Run serially after
    the parallel fleet returns, so Progress needs no lock."""
    flat = {}
    for prof in runnable:
        res = out.get(prof)
        # Check for explicit failure with error message
        if isinstance(res, dict) and res.get("rc") not in (None, 0):
            err_msg = res.get("error", f"exit code {res.get('rc')}")
            progress.emit(prof, "no", label, status="FAIL", extra=err_msg)
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err_msg}
            continue
        if res is None:
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
            # Per-player throughput/latency from the binary's BENCH stdout
            # (captured by remote_time_v as res["stdout"]).
            for p, fields in parse_bench_lines(res.get("stdout") or "").items():
                cell[(p, "ticks_per_s")]    = fields.get("ticks_per_s")
                cell[(p, "ticks")]          = fields.get("ticks")
                cell[(p, "median_tick_ns")] = fields.get("median_tick_ns")
                cell[(p, "p95_tick_ns")]    = fields.get("p95_tick_ns")
                cell[(p, "bytes_sent")]     = fields.get("bytes_sent")
            if not cell:
                err_msg = res.get("error", "no-data")
                progress.emit(prof, "no", label, status="FAIL", extra=err_msg)
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err_msg}
                continue
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        elif prof == "perf-stat":
            if not res:
                reason = bh.perf_no_hw_skip(label)
                progress.emit(prof, "no", label, status="SKIP", extra=reason)
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
                continue
            cell = {(0, f"perf_{k}"): v for k, v in res.items()}
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        elif prof == "callgrind":
            if isinstance(res, int):
                cell = {(0, "callgrind_ir"): res}
                flat.update(cell)
                per_tool[label][prof] = {"status": "PASS",
                                         "metrics": _cell_to_metric_block(cell)}
            else:
                err_msg = res.get("error", "callgrind failed") if isinstance(res, dict) else "callgrind failed"
                progress.emit(prof, "no", label, status="FAIL", extra=err_msg)
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err_msg}
        elif prof == "binary-size":
            if res is None:
                progress.emit(prof, "no", label, status="FAIL", extra="no-data")
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
                continue
            cell = {(0, "binary_size_bytes"): res}
            flat.update(cell)
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        else:
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
        progress.emit(prof, "no", label, status="PASS")
    all_metrics[label] = aggregate_metrics(flat)


def main():
    args = bh.parse_bench_args()
    if bh.acquire_singleton_lock("benchmarkmapmanager") is None:
        return 3
    br.set_benchmark_label("benchmarkmapmanager")
    if NETWORK_EXCLUSIVE:
        if bh.acquire_network_lock("benchmarkmapmanager") is None:
            return 3
    comment = args.comment
    bin_path = build()
    if bin_path is None:
        return 2

    if args.profile:
        tools = bh.profile_tools(args.profile)
        remote_spec = {
            "cmake_src_subdir": "benchmark/benchmarkmapmanager",
            "build_subdir_base": "benchmarkmapmanager",
            "bin_name": BIN_NAME,
            "cmake_defs": {"CMAKE_BUILD_TYPE": "Release",
                           "CATCHCHALLENGER_BENCH_CALLGRIND": "ON"},
            "callgrind_collect_atstart": False,
            "runtime_cmd": {t: _runtime_cmd_string(t) for t in tools},
        }
        # local: profile each tool with its own workload-mode argv.
        for t in tools:
            scale = (30 if t == "callgrind" else 20 if t == "massif" else 2)
            tmo = RUN_TIMEOUT_CALLGRIND if t == "callgrind" else RUN_TIMEOUT * scale
            bh.profile_once(run_one(bin_path, t), t,
                            cwd=os.path.dirname(bin_path), timeout=tmo,
                            node_label="local", cpu_cores=os.cpu_count(),
                            collect_atstart=False)
        # remote: build+push+run+pull per exec node (local_cmd=None: already
        # done the per-tool local runs above).
        br.profile_fleet("benchmarkmapmanager", tools, None, None, None,
                         remote_spec=remote_spec)
        return 0

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

    total = sum(len(node_profilers[node["label"]]) for node in nodes)
    progress = bh.Progress(total, "benchmarkmapmanager")
    deadline = bh.FleetDeadline()

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release"]

    all_metrics  = {}     # node_label -> {flat metric dict}
    per_tool     = {}     # node_label -> { tool -> {status, metrics, ...} }
    per_subbench = {}     # node_label -> { tool -> { slice_label -> metric_block } }
    remote_specs = []     # built serially below, run in parallel after
    for node in nodes:
        label = node["label"]
        per_tool[label] = {}
        per_subbench[label] = {}
        if deadline.reached():
            per_tool[label] = deadline.skip_node(label, all_profilers,
                                                 progress)
            all_metrics[label] = aggregate_metrics({})
            continue
        if label != "local":
            # Collect the spec now (emits SKIPs); the actual build+run is
            # done in parallel by run_profiler_fleet after this loop.
            spec = _remote_spec(node, node_profilers[label],
                                node_skips[label], all_profilers,
                                progress, per_tool)
            if spec is not None:
                remote_specs.append(spec)
            else:
                all_metrics[label] = aggregate_metrics({})
            continue
        flat = {}
        for prof in all_profilers:
            if prof not in node_profilers[label]:
                reason = node_skips[label].get(prof, "tool-missing")
                progress.emit(prof, "no", label, status="SKIP", extra=reason)
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
                continue
            deadline.note(label, prof)
            cell, err = cell_run(bin_path, prof, label)
            if cell is None:
                if err and err.startswith("SKIP:"):
                    reason = err[5:]
                    progress.emit(prof, "no", label, status="SKIP", extra=reason)
                    per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
                else:
                    progress.emit(prof, "no", label, status="FAIL", extra=err or "profiler failed")
                    per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err or "profiler failed"}
                continue
            flat.update(cell)
            progress.emit(prof, "no", label, status="PASS")
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _cell_to_metric_block(cell)}
        all_metrics[label] = aggregate_metrics(flat)

        # Per-CLAUDE.md "CPU% per sub-benchmark": for the rusage profile
        # only (perf/callgrind/binary-size are deterministic single
        # values and have no workload axis), probe each PLAYER_COUNTS
        # value individually and emit one sub-bench slice per count
        # carrying cpu_percent + the parsed BENCH tick metrics.
        if per_tool[label].get("rusage", {}).get("status") == "PASS":
            cpu_per_p = probe_cpu_percent_per_player(bin_path)
            slices = {}
            cell_metrics = per_tool[label]["rusage"]["metrics"]
            for p in PLAYER_COUNTS:
                slice_metrics = {}
                # pull parsed tick/byte values out of the existing flat
                # metric dict (keys are p<count>_<metric_name>).
                prefix = f"p{p}_"
                for key, blk in cell_metrics.items():
                    if key.startswith(prefix):
                        slice_metrics[key[len(prefix):]] = blk
                if p in cpu_per_p:
                    slice_metrics["cpu_percent"] = {
                        "value": cpu_per_p[p], "median": cpu_per_p[p],
                        "stddev": 0.0, "unit": "%", "better": "lower",
                        "samples": [cpu_per_p[p]],
                    }
                if slice_metrics:
                    slices[f"{p}-players"] = slice_metrics
            per_subbench[label]["rusage"] = slices

    # Remote fleet, in PARALLEL: phase 1 builds every unique compile node
    # at once; phase 2 pushes the binary + runs the profilers on each exec
    # node (corresponding to its compile node) concurrently. Results are
    # recorded serially here afterwards so Progress stays single-threaded.
    if remote_specs:
        print(_color(bh.C_CYAN,
              f"[bench] remote fleet: building "
              f"{len({s['compile_node']['label'] for s in remote_specs})} "
              f"compile node(s) in parallel, then running "
              f"{len(remote_specs)} exec node(s)"))
        fleet = br.run_profiler_fleet(remote_specs, verbose=True)
        for spec in remote_specs:
            label = spec["exec_node"]["label"]
            out, msg = fleet.get(label, ({}, "no fleet result"))
            _record_remote_result(label, spec["profilers"], out, msg,
                                  progress, per_tool, all_metrics)

    sha = bh.git_sha()
    cand_stamp = started_utc.replace(":", "-")

    # Cross-platform candidate record — metrics from every node that
    # produced data, so the decision reflects the whole fleet.
    rec = {
        "commit": sha,
        "comment": comment,
        "date":   time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "batch_id": batch_id,
        "benchmark": "benchmarkmapmanager",
        "nodes": {},
    }
    for node in nodes:
        label = node["label"]
        if label in all_metrics and all_metrics[label]:
            rec["nodes"][label] = {
                "arch": node.get("arch", "?"),
                "metrics": all_metrics[label],
            }
    cand_p = bh.candidate_path("benchmarkmapmanager", cand_stamp)
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
        for tool, slices in per_subbench.get(node["label"], {}).items():
            for slabel, smetrics in slices.items():
                pr.add_subbenchmark(tool, slabel, smetrics)
        out_p = pr.write(commit=sha, started_utc=started_utc,
                         ended_utc=ended_utc,
                         compile_flags=compile_flags,
                         simd_tier="generic",
                         harness_version=hr.harness_version(),
                         comment=comment)
        if out_p is not None:
            print(_color(bh.C_CYAN, f"[history] {out_p}"))

    # Cross-platform champion compare — aggregates every node's metrics,
    # not just the local host.
    champ = bh.load_champion("benchmarkmapmanager")
    decision, summary = bh.decide_multi_node(champ, rec)
    bh.print_decision("benchmarkmapmanager", decision, summary)

    if decision == "KEEP":
        ch_p = bh.champion_path("benchmarkmapmanager")
        bh.write_record(ch_p, rec)
        print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

    hr.attach_decision("benchmarkmapmanager", batch_id, decision)
    import chart_generator
    for cp in chart_generator.regenerate("benchmarkmapmanager", cand_stamp):
        print(_color(bh.C_CYAN, f"[chart] {cp}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
