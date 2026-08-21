#!/usr/bin/env python3
"""benchmarkmapmanager2.py -- min_network() over a WORLD of maps.

Multi-map sibling of benchmarkmapmanager.py. Same production code under
test (MapVisibilityAlgorithm::min_network), different LOAD MODEL. Where
benchmarkmapmanager parks every player on ONE map at a position it never
changes and only rotates directions, this one spreads the population over
many maps of three kinds -- outdoor / city / indoor -- and makes the
players WALK: one cell per tick in their facing direction, refused by the
collision grid (no pathfinding, one array lookup per moving player, so
the run measures the server and not the movement model). One tick =
min_network() over EVERY map, which is what the server's timer does.

THE WORKLOAD IS FIXED. The world shape -- 60/30/10 of the population
outdoor/city/indoor, crowded to 35/200/20 players per map of that kind --
plus the move, turn and migrate rates and the seed are constants inside
benchmarkmapmanager2/main.cpp, not flags. A benchmark whose load can be
changed from the command line is not comparable with its own history;
changing any of those constants is a deliberate re-baseline of
champion.json, never a per-run decision.

Player counts swept: 50, 250, 1000, 2500, 5000. These are TOTAL connected
players -- the strict 254 ceiling is the 8-bit wire slot inside ONE map
(255 is reserved and min_network clamps to 254), so the population is
bounded only by what the fleet can hold: 5000 players is ~120 maps and
~14 MB resident, and the smallest node has 52 MB. Town maps carry the
owner's target crowd of 200 from 1000 players up.

One-command target -- per benchmark/CLAUDE.md, run with no args, 1h
timeout. Builds the C++ harness (benchmarkmapmanager2/), runs every
available profiler (rusage via /usr/bin/time -v, perf stat, callgrind,
binary-size) on the host and on every benchmark-enabled execution node
(built on its compile parent), prints a one-line progress update per
cell, records per-platform history + charts, and applies the
KEEP/DISCARD/ESCALATE matrix against
benchmark/results/benchmarkmapmanager2/champion.json.
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
SRC_DIR    = os.path.join(BENCH_DIR, "benchmarkmapmanager2")
BIN_NAME   = "benchmark_min_network_world"

# Concurrency marker: pure in-process visibility loop, no port bind / no
# network (the binary name is historical). Safe to run in parallel.
NETWORK_EXCLUSIVE = False

# Build dir lives outside the source tree (root CLAUDE.md).
try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
except Exception:
    BUILD_ROOT = os.path.join("/tmp", "cc-build")
BUILD_DIR  = os.path.join(BUILD_ROOT, "benchmark", "benchmarkmapmanager2")

# THE WORKLOAD IS FIXED and lives in the binary (benchmarkmapmanager2/
# main.cpp): world shape, move/turn/migrate rates and seed are compile-time
# constants, not flags -- a benchmark with knobs is not comparable with its own
# history. This harness only chooses WHICH of the fixed player counts to run
# and how each run is bounded (fixed-time vs the fixed-iteration callgrind
# mode). The list below must match PLAYER_COUNTS[] in main.cpp; the binary
# refuses any other count.
#
# These are TOTAL connected players. The strict 254 ceiling is the 8-bit wire
# slot INSIDE one map (255 is reserved, and min_network clamps to 254), so the
# population is bounded only by what the fleet can hold -- 5000 players is
# ~120 maps and ~14 MB resident, and the smallest node has 52 MB. Town maps
# reach the owner's target crowd of 200 from 1000 players up.
PLAYER_COUNTS = [50, 250, 1000, 2500, 5000]
RUN_REPEATS   = 3        # warmup + 3 measured wall passes per cell

# Fixed-TIME model (benchmark/CLAUDE.md): the wall-time / throughput
# profilers (rusage, perf-stat) run each player-count for a fixed budget
# and report ticks completed (higher-is-better) -- NOT a fixed tick count
# timed to completion. Per-player-count budget in ms; total binary wall =
# MAP_BENCH_MS * len(PLAYER_COUNTS).
MAP_BENCH_MS  = 2000

# Callgrind's metric is a DETERMINISTIC instruction count, which a wall budget
# would make vary run-to-run, so callgrind alone stays fixed-iteration with a
# small tick count. One tick here broadcasts the WHOLE world (~120 maps at the
# top of the sweep), so 200 ticks is already a lot of work under a ~30x tool.
MAP_CALLGRIND_TICKS = 200
# callgrind counts only this function (+ callees) via
# --collect-atstart=no --toggle-collect, excluding process startup
# (dynamic linking ~70% of an otherwise-tiny single-core run). Glob
# matched against the binary's symbols at runtime — header-free, works on
# every cross node (benchmark/CLAUDE.md: target the long-lived server,
# not startup). min_network is the measured hot path and is never inlined.
MAP_CALLGRIND_TOGGLE = "*min_network*"

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
    cfg = ["cmake", "-S", SRC_DIR, "-B", BUILD_DIR, "-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    cfg += bh.cmake_accel_defs()
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=300)
    if rc != 0:
        bh.print_local_build_error("build", "cmake configure", sout, serr)
        return None
    # Record which libs (system vs vendored) the local build linked, for
    # the "local" node's history record.
    bh.record_libs("local", sout)
    bld = ["cmake", "--build", BUILD_DIR, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=900)
    if rc != 0:
        bh.print_local_build_error("build", "cmake build", sout, serr)
        return None
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


def _bench_to_cell(bench, cell):
    """Map one parsed BENCH block into the {(player, metric): value} cell.

    Returns an error string when the binary's own oracle failed, else None:
    walk_violations != 0 means the walk model put a player on a collision cell,
    so the workload is not what the benchmark claims to measure -- bad data,
    which is the one thing that IS a FAIL (benchmark/CLAUDE.md)."""
    for p, fields in bench.items():
        viol = fields.get("walk_violations")
        if viol:
            return (f"walk_violations={viol} at players={p}: the collision "
                    f"oracle failed, the measured workload is invalid")
        # Fixed-time headline: throughput (ticks/s, higher-is-better) + the
        # work done (ticks completed). Latency stays per-tick.
        cell[(p, "ticks_per_s")]    = fields.get("ticks_per_s")
        cell[(p, "ticks")]          = fields.get("ticks")
        cell[(p, "median_tick_ns")] = fields.get("median_tick_ns")
        cell[(p, "p95_tick_ns")]    = fields.get("p95_tick_ns")
        cell[(p, "bytes_sent")]     = fields.get("bytes_sent")
        # How many maps this population spread over -- the world shape is
        # derived from the population, so record it with the numbers it
        # explains rather than leaving a reader to recompute it.
        cell[(p, "maps")]           = fields.get("maps")
        # Harness cost per tick. NOT server work and NOT inside the latency
        # window: recorded so a reviewer can check it stayed a fraction of
        # median_tick_ns instead of taking that on trust.
        cell[(p, "median_prep_ns")] = fields.get("median_prep_ns")
    return None


def _mode_args(profiler):
    """Per-profiler workload-size args. Throughput profilers use the
    fixed-TIME budget (--ms); callgrind uses a fixed deterministic tick
    count (--ticks)."""
    if profiler == "callgrind":
        return ["--ticks", str(MAP_CALLGRIND_TICKS)]
    return ["--ms", str(MAP_BENCH_MS)]


def run_one(bin_path, profiler="rusage", players_arg=None):
    cmd = [bin_path, *_mode_args(profiler)]
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
        bad = _bench_to_cell(bench, metrics)
        if bad is not None:
            return None, bad
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
        # toggle_collect: count only min_network (+ callees) so the IR
        # excludes process startup (dynamic linking dominates an
        # otherwise-tiny run).
        ic = bh.measure_callgrind(cmd, timeout=timeout, outdir=BUILD_DIR,
                                  toggle_collect=MAP_CALLGRIND_TOGGLE)
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


def _fit_exponent(points):
    """Least-squares slope of ln(y) against ln(x) over (player_count,
    cost) points -- the empirical scaling exponent of the algorithm.

    ~2.0 means the cost grows with the SQUARE of the player count (every
    recipient paying for every slot); ~1.0 means it grows linearly. This
    is the number that says whether a change actually removed a
    quadratic term or merely shaved a constant off one, which a single
    player-count cell cannot distinguish. Returns None if there are
    fewer than 2 usable points."""
    pts = []
    for x, y in points:
        if x and y and x > 0 and y > 0:
            pts.append((x, y))
    if len(pts) < 2:
        return None
    import math
    lx = [math.log(float(x)) for x, _ in pts]
    ly = [math.log(float(y)) for _, y in pts]
    n = len(pts)
    mx = sum(lx) / n
    my = sum(ly) / n
    num = 0.0
    den = 0.0
    for i in range(n):
        num += (lx[i] - mx) * (ly[i] - my)
        den += (lx[i] - mx) ** 2
    if den == 0:
        return None
    return num / den


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


# Recorded in history and charts, but kept OUT of the champion/candidate
# metric set: they describe the RUN, not the server's performance. `maps` is
# derived from the population (it cannot move unless the workload changes) and
# median_prep_ns is the harness's own cost -- letting either into the decision
# matrix would let harness noise veto a real server improvement.
DIAGNOSTIC_METRICS = ("maps", "median_prep_ns")


def aggregate_metrics(per_cell):
    """Flatten nested {(player,metric): value} into a single
    metric-name -> {median, stddev, unit, better} dict suitable for
    champion/candidate JSON. We only have one sample per cell here
    (perf/callgrind/binary-size are deterministic); stddev=0."""
    out = {}
    for (player, metric_name), value in per_cell.items():
        if value is None: continue
        if metric_name in DIAGNOSTIC_METRICS: continue
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
    parts = [f"./{BIN_NAME}", *_mode_args(profiler)]
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
                 "cflags":   node.get("cflags"),
                 "cxxflags": node.get("cxxflags"),
                 "ldflags":  node.get("ldflags"),
                 # asmflags carries -m32 for the dual-bitness i686 sibling so
                 # the vendored libzstd x86_64 asm is assembled 32-bit too;
                 # arch/bitness let the runtime gate SKIP a box with no 32-bit
                 # loader instead of FAILing it.
                 "asmflags": node.get("asmflags"),
                 "arch":     node.get("arch"),
                 "bitness":  node.get("bitness"),
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
        "cmake_src_subdir":  "benchmark/benchmarkmapmanager2",
        "build_subdir_base": "benchmarkmapmanager2",
        "bin_name":          BIN_NAME,
        # Per-profiler workload mode: fixed-time (--ms) for rusage/perf-stat,
        # fixed-iteration (--ticks) for callgrind.
        "runtime_cmd":       {p: _runtime_cmd_string(p) for p in runnable},
        "profilers":         runnable,
        "cmake_defs":        {"CMAKE_BUILD_TYPE": "Release"},
        # Remote callgrind counts only min_network via --toggle-collect
        # (excludes startup). Resolved against symbols at runtime, so no
        # build define / valgrind header needed on the cross compile node.
        "callgrind_toggle":  MAP_CALLGRIND_TOGGLE,
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
        # A profiler that can't run on this arch (e.g. valgrind lacks the
        # 32-bit-ARM callgrind tool on the aarch64 host) reports skip_reason
        # -> SKIP, not FAIL: unknown metric, not bad data.
        if isinstance(res, dict) and res.get("skip_reason"):
            reason = res["skip_reason"]
            progress.emit(prof, "no", label, status="SKIP", extra=reason[:80])
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
            continue
        # Check for explicit failure with error message
        if isinstance(res, dict) and res.get("rc") not in (None, 0):
            err_msg = res.get("error", f"exit code {res.get('rc')}")
            progress.emit(prof, "no", label, status="FAIL", extra=err_msg)
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err_msg}
            continue
        if res is None:
            # Infra failures (compile/push/bring-up) are tagged "SKIP:" by
            # benchmark_remote; they mean the node has no measurement, not a
            # regression -- record SKIP so the decision matrix treats the
            # metric as unknown rather than a FAIL.  Either way show the full
            # cause + re-run command under a banner (the live line truncates).
            if isinstance(msg, str) and msg.startswith("SKIP:"):
                detail = msg[5:]
                bh.print_node_error("benchmarkmapmanager2", label, "SKIP", detail)
                progress.emit(prof, "no", label, status="SKIP",
                              extra=detail[:80])
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
            else:
                detail = msg if msg != "ok" else "no-data"
                bh.print_node_error("benchmarkmapmanager2", label, "FAIL", detail)
                progress.emit(prof, "no", label, status="FAIL",
                              extra=detail[:80])
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
            bad = _bench_to_cell(parse_bench_lines(res.get("stdout") or ""), cell)
            if bad is not None:
                progress.emit(prof, "no", label, status="FAIL", extra=bad)
                bh.print_node_error("benchmarkmapmanager2", label, "FAIL", bad)
                per_tool[label][prof] = {"status": "FAIL", "metrics": {},
                                         "error": bad}
                continue
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
    bh.set_node_filter(args.node)
    if bh.acquire_singleton_lock("benchmarkmapmanager2") is None:
        return 3
    br.set_benchmark_label("benchmarkmapmanager2")
    if NETWORK_EXCLUSIVE:
        if bh.acquire_network_lock("benchmarkmapmanager2") is None:
            return 3
    comment = args.comment
    maxtime = args.maxtime
    bin_path = build()
    if bin_path is None:
        return 2

    if args.profile:
        tools = bh.profile_tools(args.profile)
        remote_spec = {
            "cmake_src_subdir": "benchmark/benchmarkmapmanager2",
            "build_subdir_base": "benchmarkmapmanager2",
            "bin_name": BIN_NAME,
            "cmake_defs": {"CMAKE_BUILD_TYPE": "Release"},
            "callgrind_toggle": MAP_CALLGRIND_TOGGLE,
            "runtime_cmd": {t: _runtime_cmd_string(t) for t in tools},
        }
        # local: profile each tool with its own workload-mode argv.
        for t in tools:
            scale = (30 if t == "callgrind" else 20 if t == "massif" else 2)
            tmo = RUN_TIMEOUT_CALLGRIND if t == "callgrind" else RUN_TIMEOUT * scale
            bh.profile_once(run_one(bin_path, t), t,
                            cwd=os.path.dirname(bin_path), timeout=tmo,
                            node_label="local", cpu_cores=os.cpu_count(),
                            toggle_collect=MAP_CALLGRIND_TOGGLE)
        # remote: build+push+run+pull per exec node (local_cmd=None: already
        # done the per-tool local runs above).
        br.profile_fleet("benchmarkmapmanager2", tools, None, None, None,
                         remote_spec=remote_spec)
        return 0

    arch = bh.host_arch()
    # --node may exclude the host baseline; only prepend "local" when allowed.
    local_node = [{"label": "local", "arch": arch}] if bh.node_allowed("local", arch) else []
    # Dual 32/64-bit: append an i686 (-m32) sibling after every x86_64 exec
    # node that opted into benchmark_dual_bitness. run_profiler_fleet
    # serialises a node and its sibling on the shared host (per-host lock).
    nodes = local_node + bh.expand_bitness_variants(bh.benchmark_exec_nodes())
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
    progress = bh.Progress(total, "benchmarkmapmanager2")
    deadline = bh.FleetDeadline()

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    # Pre-workload throttle-counter baseline for the local host so the
    # post-run sensor read can attribute throttling to THIS run (delta),
    # not since-boot history. Remote nodes use the single post-run read.
    sensor_pre  = hr.sensor_baseline(hr.local_runner)
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release"]

    all_metrics  = {}     # node_label -> {flat metric dict}
    per_tool     = {}     # node_label -> { tool -> {status, metrics, ...} }
    per_subbench = {}     # node_label -> { tool -> { slice_label -> metric_block } }
    remote_specs = []     # built serially below, run in parallel after
    t_batch_start = time.monotonic()
    truncated = False     # --maxtime stopped the batch before every node ran
    prev_label = None
    for node in nodes:
        label = node["label"]
        # --maxtime: at the node-loop boundary, stop launching more nodes
        # once the overall cap is exceeded; finalise with what completed.
        if bh.maxtime_reached(t_batch_start, maxtime, prev_label):
            truncated = True
            break
        prev_label = label
        per_tool[label] = {}
        per_subbench[label] = {}
        # Per-node budget: restart the clock so a slow earlier node never
        # starves this one (see FleetDeadline).
        deadline.start_node(label)
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
                # Per-player and per-map cost: flat across the sweep means
                # the algorithm scales linearly with the population; rising
                # means a term that will not survive a full server.
                med = cell_metrics.get(f"p{p}_median_tick_ns", {}).get("value")
                maps = cell_metrics.get(f"p{p}_maps", {}).get("value")
                if med:
                    nspp = float(med) / float(p)
                    slice_metrics["ns_per_player"] = {
                        "value": nspp, "median": nspp, "stddev": 0.0,
                        "unit": "ns", "better": "lower", "samples": [nspp]}
                    if maps:
                        nspm = float(med) / float(maps)
                        slice_metrics["ns_per_map"] = {
                            "value": nspm, "median": nspm, "stddev": 0.0,
                            "unit": "ns", "better": "lower", "samples": [nspm]}
                if slice_metrics:
                    slices[f"{p}-players"] = slice_metrics
            # Empirical scaling exponent over the whole sweep: ~1.0 = linear in
            # the population, ~2.0 = every recipient still paying for every
            # slot. This is the number that says whether a change removed a
            # quadratic term or only shaved a constant off one, which no single
            # player count can show. Sub-benchmark only -- never in the
            # candidate metric set, so the champion keeps comparing the same
            # fixed workload.
            fit = []
            for p in PLAYER_COUNTS:
                blk = per_tool[label]["rusage"]["metrics"].get(f"p{p}_median_tick_ns")
                if blk is not None:
                    fit.append((p, blk["value"]))
            exp = _fit_exponent(fit)
            if exp is not None:
                slices["scaling"] = {"scaling_exponent": {
                    "value": exp, "median": exp, "stddev": 0.0,
                    "unit": "count", "better": "lower", "samples": [exp]}}
                print(_color(bh.C_CYAN,
                             f"  scaling exponent = {exp:.2f}  "
                             f"(1.0 = linear in players, 2.0 = quadratic)"))
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

    # Cross-platform candidate record — metrics from every node that
    # produced data, so the decision reflects the whole fleet.
    ended_utc = hr.iso_now()
    rec = {
        "commit": sha,
        "comment": comment,
        "date":   ended_utc,
        "started_utc": started_utc,
        "ended_utc": ended_utc,
        "duration_seconds": hr.duration_seconds(started_utc, ended_utc),
        "batch_id": batch_id,
        "benchmark": "benchmarkmapmanager2",
        "nodes": {},
    }
    for node in nodes:
        label = node["label"]
        if label in all_metrics and all_metrics[label]:
            rec["nodes"][label] = {
                "arch": node.get("arch", "?"),
                "libs": bh.LIBS_BY_NODE.get(label, {}),
                "metrics": all_metrics[label],
            }
    # No candidate-<stamp>.json: nothing reads it (bh.candidate_path had
    # no reader), and every number in it is already in the per-platform
    # history records below + champion.json on promotion.

    # Per-platform history -- one JSON per (benchmark, run, platform).
    # Per benchmark/CLAUDE.md the file is append-only; never overwritten.
    for node in nodes:
        if node["label"] not in per_tool:
            continue
        if node["label"] == "local":
            runner = hr.local_runner
        else:
            runner = hr.make_ssh_runner(node.get("ssh_host"),
                                        node.get("ssh_user"),
                                        node.get("ssh_port", 22))
        # No datapack: this is a self-contained map-manager microbenchmark.
        if node["label"] == "local":
            cc_path = BUILD_DIR
        else:
            cc_path = node.get("work_dir") or "/tmp/cc-bench-run"
        pr = hr.PlatformRecord("benchmarkmapmanager2", batch_id,
                               node["label"], runner=runner,
                               arch_hint=node.get("arch")).collect(
                                   cc_binary_path=cc_path,
                                   sensor_baseline=(sensor_pre
                                       if node["label"] == "local" else None))
        for tool, blk in per_tool[node["label"]].items():
            pr.add_result(tool, blk["metrics"], status=blk["status"])
        for tool, slices in per_subbench.get(node["label"], {}).items():
            for slabel, smetrics in slices.items():
                pr.add_subbenchmark(tool, slabel, smetrics)
        out_p = pr.write(commit=sha, started_utc=started_utc,
                         ended_utc=ended_utc,
                         compile_flags=compile_flags
                             + list(br.exec_node_flag_defs(node).values()),
                         simd_tier="generic",
                         bitness=node.get("bitness"),
                         harness_version=hr.harness_version(),
                         comment=comment)
        if out_p is not None:
            bh.chatter(_color(bh.C_CYAN, f"[history] {out_p}"))

    # Cross-platform champion compare — aggregates every node's metrics,
    # not just the local host. SKIP entirely on a --node run OR a --maxtime
    # truncated run: the decision + champion promotion need the WHOLE fleet,
    # a partial run can't confirm a change helps/regresses everywhere.
    if not bh.node_filter_active() and not truncated:
        champ = bh.load_champion("benchmarkmapmanager2")
        decision, summary = bh.decide_multi_node(champ, rec)
        bh.print_decision("benchmarkmapmanager2", decision, summary)

        if decision == "KEEP":
            ch_p = bh.champion_path("benchmarkmapmanager2")
            bh.write_record(ch_p, rec)
            print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

        hr.attach_decision("benchmarkmapmanager2", batch_id, decision)
    # Charts are a VIEW of the history JSONs, regenerated here so they never
    # lag the numbers (benchmark/CLAUDE.md). `python3 svg.py [benchmark]
    # [node]` still renders any single one on demand, and
    # `python3 chart_generator.py [benchmark]` rebuilds them out of band.
    # Distil the compact tracked form NOW rather than in a separate step, so
    # series.json (one appended number per metric per run) + platform.json
    # (machine description, rewritten only when it changes) are always in
    # sync with the run that just finished.
    import history_series
    history_series.main()
    bh.regenerate_charts("benchmarkmapmanager2")

    return 0


if __name__ == "__main__":
    sys.exit(main())
