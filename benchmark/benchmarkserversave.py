#!/usr/bin/env python3
"""benchmarkserversave.py -- benchmark `catchchallenger-server-cli save`.

Workload: invoke the server with the `save` argument so it parses the
datapack, writes datapack-cache.bin.tmp, then exits. This is the path
that determines cold-start latency on every production server boot
that needs to (re)build its HPS cache. Lower-is-better on every
metric: wall time, max RSS, output cache file size.

Determinism:
  * Single, pinned datapack (DATAPACK_PATH below). The datapack is
    read-only at scope of this benchmark; if it changes underfoot the
    metrics drift and the operator should re-baseline.
  * RUN_REPEATS sequential invocations; first one is dropped (warmup
    against a cold pagecache).
  * Output written to a tmpfs run dir so disk IO doesn't dominate.

One-command target. Per benchmark/CLAUDE.md, run with no args, 1h
timeout. Builds catchchallenger-server-cli with CATCHCHALLENGER_DB_FILE
+ CATCHCHALLENGER_CACHE_HPS, runs every available profiler against
each measured invocation, prints a one-line progress update per cell,
writes candidate-<sha>.json under
benchmark/results/benchmarkserversave/<arch>/. If a champion.json
exists, applies the KEEP/DISCARD/ESCALATE matrix.
"""
import os
import sys
import shutil
import subprocess
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh
import history_recorder as hr

REPO_ROOT  = bh.REPO_ROOT
BENCH_DIR  = os.path.dirname(os.path.abspath(__file__))

# Single pinned datapack path. CatchChallenger-datapack is the upstream
# vanilla data; pkmn would skew bytes-parsed across operators with /
# without the optional pack. Keep one source of truth so champion.json
# values stay comparable.
DATAPACK_PATH = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"

try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
    TMPFS_ROOT = test_config.TMPFS_ROOT
except Exception:
    BUILD_ROOT = "/tmp/cc-build"
    TMPFS_ROOT = "/tmp"

SRC_SUBDIR  = os.path.join(REPO_ROOT, "server/cli")
BUILD_DIR   = os.path.join(BUILD_ROOT, "benchmark", "benchmarkserversave")
RUN_DIR     = os.path.join(TMPFS_ROOT, "cc-bench-serversave")
BIN_NAME    = "catchchallenger-server-cli"

RUN_REPEATS  = 5         # 1 warmup + 4 measured (warmup excluded from median)
RUN_TIMEOUT  = 600
BUILD_TIMEOUT = 1800


def _color(c, s): return f"{c}{s}{bh.C_RESET}"


def build():
    if not os.path.isdir(DATAPACK_PATH):
        print(_color(bh.C_RED, f"[build] datapack not found: {DATAPACK_PATH}"))
        return None
    os.makedirs(BUILD_DIR, exist_ok=True)
    print(_color(bh.C_CYAN, f"[build] {SRC_SUBDIR} -> {BUILD_DIR}"))
    cfg = ["cmake", "-S", SRC_SUBDIR, "-B", BUILD_DIR,
           "-DCMAKE_BUILD_TYPE=Release",
           "-DCATCHCHALLENGER_DB_FILE=ON",
           "-DCATCHCHALLENGER_CACHE_HPS=ON"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=300)
    if rc != 0:
        print(sout); print(serr, file=sys.stderr)
        print(_color(bh.C_RED, "[build] cmake configure FAILED")); return None
    bld = ["cmake", "--build", BUILD_DIR, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=BUILD_TIMEOUT)
    if rc != 0:
        print(sout); print(serr, file=sys.stderr)
        print(_color(bh.C_RED, "[build] cmake build FAILED")); return None
    bin_path = os.path.join(BUILD_DIR, BIN_NAME)
    if not os.path.isfile(bin_path):
        print(_color(bh.C_RED, f"[build] missing: {bin_path}")); return None
    print(_color(bh.C_GREEN, f"[build] OK ({bh.binary_size(bin_path)} bytes)"))
    return bin_path


def setup_run_dir(bin_path):
    """Stage RUN_DIR so the binary's argv[0]-relative
    `<basedir>/datapack/informations.xml` resolves and `save` produces
    `datapack-cache.bin.tmp` here. Wipe between runs so every measured
    invocation starts from the same on-disk state."""
    if os.path.isdir(RUN_DIR):
        shutil.rmtree(RUN_DIR, ignore_errors=True)
    os.makedirs(RUN_DIR, exist_ok=True)
    # The server resolves files relative to the directory containing
    # argv[0]. Copy the binary into RUN_DIR so its base path is RUN_DIR.
    dst_bin = os.path.join(RUN_DIR, BIN_NAME)
    shutil.copy2(bin_path, dst_bin)
    os.chmod(dst_bin, 0o755)
    # datapack symlink (symlinks here are runtime artefacts in tmpfs --
    # NOT in the source repo, so the "no symlinks in repo" rule is fine).
    os.symlink(DATAPACK_PATH, os.path.join(RUN_DIR, "datapack"))
    # Minimal server-properties.xml. mainDatapackCode + subDatapackCode
    # match the upstream defaults shipped in CatchChallenger-datapack.
    xml = os.path.join(RUN_DIR, "server-properties.xml")
    with open(xml, "w") as f:
        f.write(
            '<?xml version="1.0"?>\n'
            '<configuration>\n'
            '    <server-port value="61920"/>\n'
            '    <automatic_account_creation value="true"/>\n'
            '    <max-players value="200"/>\n'
            '    <httpDatapackMirror value=""/>\n'
            '    <master>\n'
            '        <external-server-port value="61920"/>\n'
            '    </master>\n'
            '    <content>\n'
            '        <mainDatapackCode value="[main]"/>\n'
            '        <subDatapackCode value="[main]"/>\n'
            '    </content>\n'
            '</configuration>\n'
        )
    return dst_bin


def cleanup_save_artifacts():
    """Remove the cache file before each measured invocation so every
    `save` does the FULL parse-then-write path -- never an incremental
    no-op short-circuit."""
    for name in ("datapack-cache.bin.tmp", "datapack-cache.bin"):
        p = os.path.join(RUN_DIR, name)
        try: os.remove(p)
        except FileNotFoundError: pass


def cell_run(bin_path_in_runfir, profiler):
    """Run one (profiler) cell. Returns flat metric dict."""
    cmd = [bin_path_in_runfir, "save"]
    metrics = {}

    if profiler == "rusage":
        # warmup pass (cold pagecache)
        cleanup_save_artifacts()
        bh.measure_time_v(cmd, timeout=RUN_TIMEOUT)
        wall_samples, rss_samples, user_samples, sys_samples = [], [], [], []
        cache_size = None
        for _ in range(RUN_REPEATS - 1):
            cleanup_save_artifacts()
            t = bh.measure_time_v(cmd, timeout=RUN_TIMEOUT)
            if t["rc"] != 0: return None
            if t["wall_s"]    is not None: wall_samples.append(t["wall_s"])
            if t["max_rss_kb"] is not None: rss_samples.append(t["max_rss_kb"])
            if t["user_s"]    is not None: user_samples.append(t["user_s"])
            if t["sys_s"]     is not None: sys_samples.append(t["sys_s"])
            cache = os.path.join(os.path.dirname(bin_path_in_runfir), "datapack-cache.bin.tmp")
            if os.path.isfile(cache):
                cache_size = os.path.getsize(cache)
        for name, samples in (("wall_s", wall_samples), ("user_s", user_samples),
                              ("sys_s", sys_samples), ("max_rss_kb", rss_samples)):
            med, std = bh.stats_of(samples)
            if med is not None:
                metrics[name] = (med, std)
        if cache_size is not None:
            metrics["cache_bytes"] = (cache_size, 0.0)
        return metrics

    if profiler == "perf-stat":
        cleanup_save_artifacts()
        out = bh.measure_perf_stat(cmd, timeout=RUN_TIMEOUT)
        if not out: return None
        for k, v in out.items():
            metrics[f"perf_{k}"] = (v, 0.0)
        return metrics

    if profiler == "callgrind":
        cleanup_save_artifacts()
        ic = bh.measure_callgrind(cmd, timeout=RUN_TIMEOUT * 30, outdir=RUN_DIR)
        if ic is None: return None
        metrics["callgrind_ir"] = (ic, 0.0)
        return metrics

    if profiler == "binary-size":
        sz = bh.binary_size(bin_path_in_runfir)
        if sz is None: return None
        metrics["binary_size_bytes"] = (sz, 0.0)
        return metrics
    return None


def _flat_to_metric_block(flat):
    """Convert {name -> (median, stddev)} to history-recorder schema."""
    out = {}
    for name, (med, std) in flat.items():
        unit = "s" if name.endswith("_s") else \
               "kb" if name.endswith("_kb") else \
               "bytes" if name.endswith("_bytes") or name == "cache_bytes" else \
               "count"
        out[name] = {"value": med, "median": med, "stddev": std,
                     "unit": unit, "better": "lower", "samples": None}
    return out


def to_record_metrics(flat):
    out = {}
    for name, (med, std) in flat.items():
        unit = "s" if name.endswith("_s") else \
               "kb" if name.endswith("_kb") else \
               "bytes" if name.endswith("_bytes") or name == "cache_bytes" else \
               "count"
        out[name] = {"median": med, "stddev": std, "unit": unit, "better": "lower"}
    return out


def main():
    bin_path = build()
    if bin_path is None: return 2
    run_bin = setup_run_dir(bin_path)

    arch = bh.host_arch()
    nodes = [{"label": "local", "arch": arch}] + bh.benchmark_exec_nodes()
    profilers = ["rusage", "binary-size"]
    if bh.have_tool("perf"):
        profilers.append("perf-stat")
    if bh.have_tool("valgrind") and bh.have_tool("callgrind_annotate"):
        profilers.append("callgrind")

    total = len(nodes) * len(profilers)
    progress = bh.Progress(total)

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release",
                     "-DCATCHCHALLENGER_DB_FILE=ON",
                     "-DCATCHCHALLENGER_CACHE_HPS=ON"]

    flat_local = {}
    per_tool   = {}    # node_label -> { tool -> {status, metrics} }
    for node in nodes:
        if node["label"] != "local":
            for prof in profilers:
                progress.emit(prof, "no", node["label"], status="SKIP",
                              extra="remote-dispatch-not-wired")
            continue
        per_tool[node["label"]] = {}
        for prof in profilers:
            cell = cell_run(run_bin, prof)
            if cell is None:
                progress.emit(prof, "no", node["label"], status="FAIL")
                per_tool[node["label"]][prof] = {"status": "FAIL", "metrics": {}}
                continue
            flat_local.update(cell)
            progress.emit(prof, "no", node["label"], status="PASS")
            per_tool[node["label"]][prof] = {"status": "PASS",
                                             "metrics": _flat_to_metric_block(cell)}

    sha = bh.git_sha()
    rec = {
        "commit": sha,
        "date":   time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "node":   "local",
        "metrics": to_record_metrics(flat_local),
        "profilers": {"binary-size": run_bin},
    }
    cand_p = bh.candidate_path("benchmarkserversave", arch, sha)
    bh.write_record(cand_p, rec)
    print(_color(bh.C_CYAN, f"[record] candidate -> {cand_p}"))

    # Per-platform history -- one JSON per (benchmark, run, platform).
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
        pr = hr.PlatformRecord("benchmarkserversave", batch_id,
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

    champ = bh.load_champion("benchmarkserversave", arch)
    decision, summary = bh.decide(champ, rec)
    bh.print_decision("benchmarkserversave", arch, decision, summary)

    if decision == "KEEP":
        ch_p = bh.champion_path("benchmarkserversave", arch)
        bh.write_record(ch_p, rec)
        print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

    hr.attach_decision("benchmarkserversave", batch_id, decision)
    import chart_generator
    for cp in chart_generator.regenerate("benchmarkserversave"):
        print(_color(bh.C_CYAN, f"[chart] {cp}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
