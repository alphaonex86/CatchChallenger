#!/usr/bin/env python3
"""benchmark_microbench.py -- what each node's CPU really is (clock, cache
steps), measured with benchmark/microbench like SpeedSys does under DOS.

/proc/cpuinfo cannot tell it for old or minimal CPUs (a 486 reports no MHz and
no cache), and a number measured on the node itself beats one read from a
datasheet. The result is cached 24 h per physical node, so the record of every
benchmark gets it without paying the build + transfer + run each time.

ensure_missing() measures, before the benchmark starts, every node whose
result is missing or stale (bh.Progress calls it, so every benchmark does).
history_recorder only READS the cache through fields_for_label(measure=False):
measuring at record time could tear down an lxc_nfs node mid-collect.
"""
import concurrent.futures
import fcntl
import hashlib
import json
import os
import platform
import re
import subprocess
import time

import benchmark_helpers as bh

SRC_SUBDIR = "benchmark/microbench"
BIN_NAME = "benchmark_microbench"
MAX_AGE_S = 24 * 3600
CACHE_DIR = os.path.join(os.path.expanduser("~"), ".local", "share",
                         "CatchChallenger", "benchmark-microbench")
LOCAL_BUILD_DIR = os.path.join("/tmp", "cc-build", "benchmark", "microbench")
# Same gate as every benchmark (benchmark/CLAUDE.md): let the transfer's spike
# leave the 1-min loadavg window, then refuse a busy node -- its numbers would
# sit in the cache for 24 h.
GATED_RUN = f"sleep 60 && cat /proc/loadavg && ./{BIN_NAME}"


def parse(text):
    """MICRO lines -> {"mhz", "cache_kb": [..], "latency_ns_x100": {kb: ns}} or {}."""
    out = {"latency_ns_x100": {}}
    for line in text.splitlines():
        found = re.match(r"MICRO mhz=(\d+)", line)
        if found:
            out["mhz"] = int(found.group(1))
        found = re.match(r"MICRO_CACHE kb=(\d+) ns_x100=(\d+)", line)
        if found:
            out["latency_ns_x100"][int(found.group(1))] = int(found.group(2))
        found = re.match(r"MICRO cache_kb=([\d,]*)", line)
        if found:
            out["cache_kb"] = [int(v) for v in found.group(1).split(",") if v]
    if "mhz" not in out:
        return {}
    out.setdefault("cache_kb", [])
    return out


def _source_hash():
    """A result measured by another version of the microbench is not reused."""
    with open(os.path.join(bh.REPO_ROOT, SRC_SUBDIR, "main.cpp"), "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def _fresh(record):
    return time.time() - record["measured_at"] < MAX_AGE_S \
        and record.get("source") == _source_hash()


def parse_gated(label, text):
    """parse() of a GATED_RUN output, or {} when the node was busy."""
    first = text.split("\n", 1)[0].split()
    try:
        load = float(first[0])
    except (IndexError, ValueError):
        print(f"[microbench] {label}: no loadavg line in the output")
        return {}
    if load >= 1.0:
        print(f"[microbench] {label}: SKIP load={load:.2f}, not measured")
        return {}
    return parse(text)


def _cache_file(label):
    return os.path.join(CACHE_DIR, re.sub(r"[^A-Za-z0-9_.-]", "_", label) + ".json")


def _is_stale(label):
    try:
        with open(_cache_file(label)) as handle:
            return not _fresh(json.load(handle))
    except (OSError, ValueError, KeyError):
        return True


def cached(label, producer):
    """The microbench result of `label`, reused for 24 h; else producer()'s
    (a parsed dict, or {} on failure, which is never cached). producer=None
    only reads."""
    path = _cache_file(label)
    try:
        with open(path) as handle:
            got = json.load(handle)
        # Reading only (the recorder): any result of this microbench version,
        # whatever its age -- the run checked freshness when it started, and an
        # entry expiring mid-run must not vanish from that run's record.
        if _fresh(got) or (producer is None and got.get("source") == _source_hash()):
            return got["result"]
    except (OSError, ValueError, KeyError):
        pass
    if producer is None:
        return {}
    result = producer()
    if result:
        os.makedirs(CACHE_DIR, exist_ok=True)
        with open(path + ".tmp", "w") as handle:
            json.dump({"measured_at": time.time(), "source": _source_hash(),
                       "result": result}, handle)
        os.replace(path + ".tmp", path)
    return result


def _measure_local():
    os.makedirs(LOCAL_BUILD_DIR, exist_ok=True)
    binary = os.path.join(LOCAL_BUILD_DIR, BIN_NAME)
    rc, sout, serr, _ = bh.run_capture(
        ["g++", "-O2", "-std=c++11", "-o", binary,
         os.path.join(bh.REPO_ROOT, SRC_SUBDIR, "main.cpp")], timeout=300)
    if rc != 0:
        print(f"[microbench] local build failed: {serr.strip()[-300:]}")
        return {}
    rc, sout, serr, _ = bh.run_capture([binary], timeout=300)
    if rc != 0:
        print(f"[microbench] local run failed (rc {rc}): {serr.strip()[-300:]}")
        return {}
    return parse(sout)


def _measure_remote(label):
    import benchmark_remote as br
    cfg = bh.load_remote_nodes()
    compile_node, exec_cfg = br.compile_node_for_exec(cfg, label)
    if compile_node is None:
        print(f"[microbench] {label}: no compile node pairs with it, skipped")
        return {}
    ex = br.exec_node_dict(dict(exec_cfg, ssh_user=exec_cfg.get("user"),
                                ssh_host=exec_cfg.get("host"),
                                ssh_port=exec_cfg.get("port", 22)))
    rc, msg = br.stage_source_once(compile_node)
    if rc != 0:
        print(f"[microbench] {label}: source staging failed: {msg}")
        return {}
    # One build dir per exec node: siblings on one compile node carry their
    # own -march flags and are built concurrently by ensure_missing().
    rc, msg, build_dir = br.build_on_compile_node(compile_node, SRC_SUBDIR,
                                                  "microbench-" + label, exec_node=ex)
    if rc != 0:
        print(f"[microbench] {label}: build failed: {str(msg)[-300:]}")
        return {}
    runtime, msg = br.nfs_lxc_prepare(ex)
    if runtime is None:
        print(f"[microbench] {label}: {msg}")
        br.nfs_lxc_teardown(ex)
        return {}
    try:
        rc, exec_bin, msg = br.push_binary_to_exec(compile_node, runtime, build_dir, BIN_NAME)
        if rc != 0:
            print(f"[microbench] {label}: push failed: {msg}")
            return {}
        rc, sout, serr = br.run_remote_cmd(
            runtime, f"cd {os.path.dirname(exec_bin)} && {GATED_RUN}", timeout=300)
        if rc != 0:
            print(f"[microbench] {label}: run failed (rc {rc}): {serr.strip()[-300:]}")
            return {}
        return parse_gated(label, sout)
    finally:
        br.nfs_lxc_teardown(ex)


def result_for_label(label, measure=True):
    """Result for one node label (-m32 siblings share their base hardware), or
    {}. measure=False only reads the cache. The ESP32 has nothing to run it on."""
    base = label[:-len("-m32")] if label.endswith("-m32") else label
    if base == "esp32":
        return {}
    if not measure:
        return cached(base, None)
    if base == "local":
        return cached(base, _measure_local)
    if base == "toshiba-T2100":
        import benchmark_t2100 as t2100
        return cached(base, t2100.measure_microbench)
    return cached(base, lambda: _measure_remote(base))


def fields_for_label(label, measure=True):
    """Platform fields for history_recorder: the stable result under `microbench`,
    plus `cpu_mhz_measured` and `cpu_cache_measured` so a recorder can fill the
    gaps /proc leaves without overwriting what it did read."""
    try:
        got = result_for_label(label, measure)
    except (OSError, ValueError, subprocess.SubprocessError, KeyError) as e:
        print(f"[microbench] {label}: {e}")
        return {}
    if not got:
        return {}
    # platform.json is git-tracked and rewritten only when it changes, so it
    # gets the STABLE part: MHz to 2 significant digits and the cache steps.
    # The full latency table stays in the local cache file.
    import history_series
    mhz = float(history_series.round_sig(got["mhz"], 2))
    steps = [{"type": "Unified", "level": level + 1, "size": f"{kb}K",
              "line_size_bytes": None} for level, kb in enumerate(got["cache_kb"])]
    return {"microbench": {"mhz": mhz, "cache_kb": got["cache_kb"]},
            "cpu_mhz_measured": mhz, "cpu_cache_measured": steps}


_ENSURED = False


def ensure_missing():
    """Measure every node of this run whose microbench result is missing or
    older than 24 h, BEFORE the benchmark starts (called once per process by
    bh.Progress). Nodes are read from remote_nodes.json without touching them
    (the load gate belongs to the benchmark), filtered by --node. A node that
    cannot be measured is reported and left out; the benchmark goes on."""
    global _ENSURED
    if _ENSURED:
        return
    _ENSURED = True
    labels = []
    if bh.node_allowed("local", platform.machine()):
        labels.append("local")
    for node in bh.load_remote_nodes().get("nodes", []):
        if node.get("enabled", False):
            for ex in node.get("execution_nodes", []):
                label = ex.get("label")
                if ex.get("enabled", False) and ex.get("benchmark", False) \
                        and bh.node_allowed(label, node.get("arch")) and label not in labels:
                    labels.append(label)
    if bh.node_allowed("toshiba-T2100", "i486"):
        labels.append("toshiba-T2100")
    # Two benchmarks starting together must not measure the same node twice
    # at once: the second waits here, then finds the cache fresh.
    os.makedirs(CACHE_DIR, exist_ok=True)
    with open(os.path.join(CACHE_DIR, ".lock"), "a") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        _measure_stale(labels)


def _measure_stale(labels):
    todo = [label for label in labels if _is_stale(label)]
    if not todo:
        return
    print(f"[microbench] no fresh measurement for {', '.join(todo)}: measuring "
          f"before the benchmark")
    results = []
    # The host alone first: the other nodes' transfers and builds (and the LXC
    # nodes living on this host) would skew its cache latencies.
    if todo[0] == "local":
        results.append(fields_for_label("local"))
    remote = todo[len(results):]
    if remote:
        with concurrent.futures.ThreadPoolExecutor(max_workers=len(remote)) as pool:
            results += list(pool.map(fields_for_label, remote))
    for label, got in zip(todo, results):
        if not got:
            print(f"[microbench] {label}: no data this run")
