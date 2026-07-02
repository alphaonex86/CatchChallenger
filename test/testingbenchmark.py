#!/usr/bin/env python3
"""testingbenchmark.py — performance regression gate.

Unlike benchmark/*.py (which optimise and may promote a new champion),
this is a hard PASS/FAIL test: it runs the chosen benchmarks LOCALLY
(no remote/exec nodes, no ssh) and asserts every measured metric stays
within a fixed band around a frozen baseline value.

The baselines and the band live in THIS file — no JSON sidecar to drift.
A silent 2% per-commit regression cannot accumulate: the floor is the
same value every run, anchored to the source. Bumping a baseline or the
margin is a code change, reviewable in `git diff test/testingbenchmark.py`.

The margin (MARGIN) is uniform 10%. It is set well above the observed
local noise floor (CV 0.1%–2.2% for binary-size, instruction count,
RSS, wall_s; CV 0.4%–1.3% for the saturated-path sub-benchmark metrics
— see the table committed alongside this script's first commit, derived
from `benchmark/history/<bench>/local/local/*.json`). 10% gives zero
false positives on the noise band while catching any single ≥10%
regression on the first commit that introduces it.

Metrics deliberately NOT in the table (perf cycles, perf/cache-misses,
all *_tick_ns, bytes_sent, short-run cpu_percent) carry CV 10%–60% on
the local box and cannot support a hard gate — they would either false-
positive constantly or force a margin so wide a real regression hides.
"""
import faulthandler
import glob
import json
import os
import subprocess
import sys
import time

faulthandler.enable()
WALL_LIMIT_SEC = 14 * 60
faulthandler.dump_traceback_later(WALL_LIMIT_SEC + 10, exit=False)

REPO       = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BENCH_DIR  = os.path.join(REPO, "benchmark")
HISTORY    = os.path.join(BENCH_DIR, "history")

# Uniform band around the frozen baseline. current must satisfy:
#   direction == "higher" (lower is better):  cur <= baseline * (1 + MARGIN)
#   direction == "lower"  (higher is better): cur >= baseline * (1 - MARGIN)
MARGIN = 0.10

# Frozen baselines — measured on the local reference box.
# (benchmark, profiler, metric) -> {"baseline": <value>, "direction": "higher"|"lower"}
# profiler may be "<tool>#<subbench>" to dig into results.<tool>.subbenchmarks.
BASELINES = {
    # ---- benchmarkserversave (datapack → datapack-cache.bin save) ----
    ("benchmarkserversave", "binary-size", "binary_size_bytes"):  {"baseline": 2_875_000, "direction": "higher"},
    ("benchmarkserversave", "perf-stat",   "perf_instructions:u"): {"baseline": 49_500_000, "direction": "higher"},
    ("benchmarkserversave", "rusage",      "cache_bytes"):         {"baseline": 51_270,    "direction": "higher"},
    ("benchmarkserversave", "rusage",      "max_rss_kb"):          {"baseline": 8_250,     "direction": "higher"},
    # ---- benchmarkmapmanager (per-tick server under bot load) ----
    ("benchmarkmapmanager", "binary-size", "binary_size_bytes"):  {"baseline": 1_283_000, "direction": "higher"},
    ("benchmarkmapmanager", "rusage",      "max_rss_kb"):          {"baseline": 8_200,     "direction": "higher"},
    ("benchmarkmapmanager", "rusage",      "wall_s"):              {"baseline": 14.3,      "direction": "higher"},
    # Saturated-path throughput: regression = LOWER ticks/s (less work/s at ~100% CPU).
    ("benchmarkmapmanager", "rusage#50-players",  "ticks_per_s"):  {"baseline": 46_000,    "direction": "lower"},
    ("benchmarkmapmanager", "rusage#100-players", "ticks_per_s"):  {"baseline": 28_500,    "direction": "lower"},
    ("benchmarkmapmanager", "rusage#200-players", "ticks_per_s"):  {"baseline": 10_500,    "direction": "lower"},
}

BENCHMARKS = ["benchmarkserversave", "benchmarkmapmanager"]


def log(s):
    print(s, flush=True)


def run_benchmark(name):
    """Invoke benchmark/<name>.py restricted to local-only execution.

    `--node local` skips every remote/exec node (no ssh, no rsync, no
    loadavg gate). The benchmark writes a fresh per-platform history
    JSON we then read back — no JSON is committed to the repo."""
    log(f"[testingbenchmark] running {name} --node local …")
    t0 = time.time()
    proc = subprocess.run(
        [sys.executable, os.path.join(BENCH_DIR, f"{name}.py"),
         "--node", "local", "--comment", "testingbenchmark"],
        cwd=REPO,
        env=os.environ.copy(),
    )
    dt = time.time() - t0
    log(f"[testingbenchmark] {name} exit={proc.returncode} dt={dt:.1f}s")
    return proc.returncode


def latest_history(bench):
    """Newest history JSON the benchmark just wrote for the local node."""
    patt = os.path.join(HISTORY, bench, "local", "local", "*.json")
    files = glob.glob(patt)
    if not files:
        return None
    return max(files, key=os.path.getmtime)


def extract_metric(record, profiler, metric):
    """Pull median value for (profiler, metric) out of a history record.
    Handles profiler="<tool>#<subbench>" by digging into subbenchmarks."""
    results = record.get("results", {})
    if "#" in profiler:
        tool, sub = profiler.split("#", 1)
        tool_data = results.get(tool, {})
        sb = tool_data.get("subbenchmarks") or {}
        cell = sb.get(sub, {})
        if metric not in cell:
            return None
        return cell[metric].get("median")
    tool_data = results.get(profiler, {})
    metrics = tool_data.get("metrics") or {}
    if metric not in metrics:
        return None
    return metrics[metric].get("median")


def check_benchmark(bench):
    """Run the benchmark, then verify every baseline entry that targets it.
    Returns (failures_list, ok_count)."""
    failures = []
    ok_count = 0

    rc = run_benchmark(bench)
    if rc != 0:
        failures.append(f"{bench}: benchmark script exited {rc}")
        return failures, 0

    hist = latest_history(bench)
    if hist is None:
        failures.append(f"{bench}: no history JSON produced under {HISTORY}")
        return failures, 0

    try:
        with open(hist) as fh:
            record = json.load(fh)
    except Exception as exc:
        failures.append(f"{bench}: cannot parse {hist}: {exc}")
        return failures, 0

    loadavg = record.get("loadavg_1min_at_start")
    governor = record.get("cpu_governor")
    if governor and governor != "performance":
        log(f"  [warn] cpu_governor={governor!r} (expected 'performance') — "
            f"timing metrics may be skewed; treat any FAIL below with caution.")

    for (b, prof, metric), spec in BASELINES.items():
        if b != bench:
            continue
        cur = extract_metric(record, prof, metric)
        if cur is None:
            failures.append(f"{bench}/{prof}/{metric}: MISSING in history JSON")
            continue
        base = spec["baseline"]
        direction = spec["direction"]
        if direction == "higher":
            threshold = base * (1.0 + MARGIN)
            ok = cur <= threshold
        else:
            threshold = base * (1.0 - MARGIN)
            ok = cur >= threshold
        rel = (cur - base) / base * 100.0 if base else 0.0
        tag = "OK  " if ok else "FAIL"
        log(f"  [{tag}] {prof}/{metric}: cur={cur:g} base={base:g} "
            f"rel={rel:+.1f}% thr={threshold:g}")
        if ok:
            ok_count += 1
        else:
            failures.append(
                f"{bench}/{prof}/{metric}: cur={cur:g} base={base:g} "
                f"rel={rel:+.1f}% (beyond ±{int(MARGIN*100)}% band)"
            )
    return failures, ok_count


def main():
    log(f"[testingbenchmark] gate: ±{int(MARGIN*100)}% around frozen baseline, "
        f"local-only, {len(BENCHMARKS)} benchmarks, "
        f"{sum(1 for k in BASELINES if k[0] in BENCHMARKS)} metrics")
    all_failures = []
    total_ok = 0
    for bench in BENCHMARKS:
        log(f"[testingbenchmark] ===== {bench} =====")
        failures, ok = check_benchmark(bench)
        all_failures.extend(failures)
        total_ok += ok

    log("-" * 60)
    if all_failures:
        for f in all_failures:
            log(f"[FAIL] {f}")
        log(f"[FAIL] testingbenchmark: {len(all_failures)} regression(s), "
            f"{total_ok} within band")
        sys.exit(1)
    log(f"[PASS] testingbenchmark: {total_ok} metric(s) within "
        f"±{int(MARGIN*100)}% of frozen baseline")
    sys.exit(0)


if __name__ == "__main__":
    main()
