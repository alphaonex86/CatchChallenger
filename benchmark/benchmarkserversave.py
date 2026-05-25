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
timeout. Builds catchchallenger-server-cli at -O2 with
CATCHCHALLENGER_DB_INTERNAL_VARS (RAM DB, implies DB_FILE) +
CATCHCHALLENGER_CACHE_HPS, runs every available profiler against
each measured invocation, prints a one-line progress update per cell,
writes candidate-<sha>.json under
benchmark/results/benchmarkserversave/<arch>/. If a champion.json
exists, applies the KEEP/DISCARD/ESCALATE matrix.
"""
import os
import shlex
import sys
import shutil
import subprocess
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh
import history_recorder as hr
import benchmark_remote as br

REPO_ROOT  = bh.REPO_ROOT
BENCH_DIR  = os.path.dirname(os.path.abspath(__file__))

# Concurrency marker: `server save` parses the datapack and writes the HPS
# cache, then exits -- it binds no port and uses no network, so it is safe
# to run in parallel with other benchmarks. (See benchmarkbotactions for
# the network-exclusive case.)
NETWORK_EXCLUSIVE = False

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
           # -O2 (not the Release default -O3): serversave measures the
           # one-shot datapack-cache build; -O2 is the optimisation level
           # production boots actually ship and -O3 only inflates the
           # binary here without a reproducible speed-up.
           "-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG",
           # RAM DB only: DB_INTERNAL_VARS keeps the file-DB tree in
           # /dev/shm (tmpfs), never disk, and implies DB_FILE in CMake --
           # so we don't pass DB_FILE here. (DB_FILE only governs
           # player/character persistence; the datapack-cache path this
           # benchmark measures is independent of the DB backend.)
           "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
           "-DCATCHCHALLENGER_CACHE_HPS=ON"]
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
    rc, sout, serr, _ = bh.run_capture(bld, timeout=BUILD_TIMEOUT)
    if rc != 0:
        bh.print_local_build_error("build", "cmake build", sout, serr)
        return None
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
    # Pick a maincode that actually exists in the datapack (server's
    # CommonSettingsServer regex requires ^[a-z0-9]+$ — brackets break
    # it). Prefer "test" when present (small map set, fastest parse).
    maincode = _detect_maincode(DATAPACK_PATH)
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
            f'        <mainDatapackCode value="{maincode}"/>\n'
            '        <subDatapackCode value=""/>\n'
            '    </content>\n'
            '</configuration>\n'
        )
    return dst_bin


def _detect_maincode(datapack_src):
    """Pick a maincode from `datapack/map/main/`. Prefer "test" (small
    map set) for benchmark speed; fall back to the first alphabetical
    maincode otherwise. Mirrors test/remote_build.py:_detect_maincode."""
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return "test"
    entries = sorted(os.listdir(map_main))
    for e in entries:
        if e == "test" and os.path.isdir(os.path.join(map_main, e)):
            return "test"
    for e in entries:
        if os.path.isdir(os.path.join(map_main, e)):
            return e
    return "test"


def cleanup_save_artifacts():
    """Remove the cache file before each measured invocation so every
    `save` does the FULL parse-then-write path -- never an incremental
    no-op short-circuit."""
    for name in ("datapack-cache.bin.tmp", "datapack-cache.bin"):
        p = os.path.join(RUN_DIR, name)
        try: os.remove(p)
        except FileNotFoundError: pass


def cell_run(bin_path_in_runfir, profiler):
    """Run one (profiler) cell. Returns (flat_metric_dict, skip_reason).
    On success: (metrics, None). On skip: (None, 'SKIP:reason').
    On failure: (None, None)."""
    cmd = [bin_path_in_runfir, "save"]
    metrics = {}

    if profiler == "rusage":
        # warmup pass (cold pagecache). cwd=RUN_DIR (tmpfs) so the
        # server's own status/marker files + any core never land in
        # the checked-in benchmark/ tree.
        cleanup_save_artifacts()
        bh.measure_time_v(cmd, timeout=RUN_TIMEOUT, cwd=RUN_DIR)
        wall_samples, rss_samples, user_samples, sys_samples = [], [], [], []
        cache_size = None
        for _ in range(RUN_REPEATS - 1):
            cleanup_save_artifacts()
            t = bh.measure_time_v(cmd, timeout=RUN_TIMEOUT, cwd=RUN_DIR)
            if t["rc"] != 0: return None, None
            if t["wall_s"]    is not None: wall_samples.append(t["wall_s"])
            if t["max_rss_kb"] is not None: rss_samples.append(t["max_rss_kb"])
            if t["user_s"]    is not None: user_samples.append(t["user_s"])
            if t["sys_s"]     is not None: sys_samples.append(t["sys_s"])
            cache_dir = os.path.dirname(bin_path_in_runfir)
            for name in ("datapack-cache.bin.tmp", "datapack-cache.bin"):
                p = os.path.join(cache_dir, name)
                if os.path.isfile(p):
                    cache_size = os.path.getsize(p)
                    break
        for name, samples in (("wall_s", wall_samples), ("user_s", user_samples),
                              ("sys_s", sys_samples), ("max_rss_kb", rss_samples)):
            med, std = bh.stats_of(samples)
            if med is not None:
                metrics[name] = (med, std)
        if cache_size is not None:
            metrics["cache_bytes"] = (cache_size, 0.0)
        # cpu_percent per CLAUDE.md "CPU% per sub-benchmark". This binary
        # is single-shot save (no workload axis), so we derive one
        # aggregate value from the (user+sys)/wall medians across the
        # measured passes. Server is single-threaded -> clamp at 100.
        pct_samples = []
        for u, sv, w in zip(user_samples, sys_samples, wall_samples):
            if w and w > 0:
                p = (u + sv) / w * 100.0
                pct_samples.append(min(100.0, p))
        if pct_samples:
            pmed, pstd = bh.stats_of(pct_samples)
            if pmed is not None:
                metrics["cpu_percent"] = (pmed, pstd)
        return metrics, None

    if profiler == "perf-stat":
        cleanup_save_artifacts()
        out = bh.measure_perf_stat(cmd, timeout=RUN_TIMEOUT, cwd=RUN_DIR)
        if not out:
            return None, "SKIP:" + bh.perf_no_hw_skip("local")
        for k, v in out.items():
            metrics[f"perf_{k}"] = (v, 0.0)
        return metrics, None

    if profiler == "callgrind":
        cleanup_save_artifacts()
        ic = bh.measure_callgrind(cmd, timeout=RUN_TIMEOUT * 30, outdir=RUN_DIR)
        if ic is None: return None, None
        metrics["callgrind_ir"] = (ic, 0.0)
        return metrics, None

    if profiler == "binary-size":
        sz = bh.binary_size(bin_path_in_runfir)
        if sz is None: return None, None
        metrics["binary_size_bytes"] = (sz, 0.0)
        return metrics, None
    return None, None


def _flat_to_metric_block(flat):
    """Convert {name -> (median, stddev)} to history-recorder schema."""
    out = {}
    for name, (med, std) in flat.items():
        unit = "s" if name.endswith("_s") else \
               "kb" if name.endswith("_kb") else \
               "bytes" if name.endswith("_bytes") or name == "cache_bytes" else \
               "%" if name == "cpu_percent" else \
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
               "%" if name == "cpu_percent" else \
               "count"
        out[name] = {"median": med, "stddev": std, "unit": unit, "better": "lower"}
    return out


def _run_remote_cells_serversave(node, avail_profilers, skips, all_profilers,
                                 progress, per_tool, compile_flags):
    """Remote dispatch for one exec node: build on compile node, rsync datapack,
    write server-properties.xml, run `./catchchallenger-server-cli save` under
    each available profiler. Updates per_tool[label] in place."""
    label        = node["label"]
    compile_node = node.get("compile_node")
    if compile_node is None:
        for prof in all_profilers:
            progress.emit(prof, "no", label, status="SKIP", extra="no-compile-node")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        return

    exec_node = {"label": label,
                 "user":  node.get("ssh_user"),
                 "host":  node.get("ssh_host"),
                 "port":  node.get("ssh_port", 22),
                 "work_dir": node.get("work_dir") or "/tmp/cc-bench-serversave",
                 "cflags":   node.get("cflags"),
                 "cxxflags": node.get("cxxflags"),
                 "ldflags":  node.get("ldflags"),
                 "lxc_nfs": node.get("lxc_nfs")}

    runnable = [p for p in all_profilers if p in avail_profilers]
    for prof in all_profilers:
        if prof not in runnable:
            reason = skips.get(prof, "tool-missing")
            progress.emit(prof, "no", label, status="SKIP", extra=reason)
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
    if not runnable:
        return

    # NFS-LXC bring-up (no-op for ordinary nodes) BEFORE any exec-node I/O:
    # the datapack rsync + xml write below must target the guest container,
    # not the device pre-chroot host. teardown in finally.
    orig_exec_node = exec_node
    rt_node, prep_msg = br.nfs_lxc_prepare(orig_exec_node, verbose=True)
    if rt_node is None:
        for prof in runnable:
            progress.emit(prof, "no", label, status="FAIL", extra=prep_msg)
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
        br.nfs_lxc_teardown(orig_exec_node, verbose=True)
        return
    exec_node = rt_node
    try:
        _serversave_remote_body(node, exec_node, compile_node, label,
                                runnable, progress, per_tool)
    finally:
        br.nfs_lxc_teardown(orig_exec_node, verbose=True)


def _serversave_remote_body(node, exec_node, compile_node, label,
                            runnable, progress, per_tool):
    # Rsync the datapack to exec node before building — it's needed at runtime.
    if os.path.isdir(DATAPACK_PATH):
        rc_dp, msg_dp = br.rsync_datapack_to_exec(exec_node, DATAPACK_PATH,
                                                   remote_subdir="datapack",
                                                   timeout=600, server_mode=True)
        if rc_dp != 0:
            for prof in runnable:
                progress.emit(prof, "no", label, status="FAIL",
                              extra=f"datapack-rsync-failed: {msg_dp[:80]}")
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
            return

    # Detect maincode from the local datapack (same logic as setup_run_dir).
    maincode = _detect_maincode(DATAPACK_PATH)
    ewd = exec_node["work_dir"]
    br.write_server_properties_on_exec(exec_node, maincode, port=61921,
                                       run_subdir=ewd)

    cmake_defs = {"CMAKE_BUILD_TYPE": "Release",
                  "CMAKE_CXX_FLAGS_RELEASE": "-O2 -DNDEBUG",  # -O2, not Release -O3
                  "CATCHCHALLENGER_DB_INTERNAL_VARS": "ON",  # RAM DB (tmpfs); implies DB_FILE
                  "CATCHCHALLENGER_CACHE_HPS": "ON"}
    # runtime_cmd: wipe the cache file before each `save` so every invocation
    # does the full parse. The binary and datapack are both in work_dir.
    runtime_cmd = (f"rm -f {shlex.quote(ewd)}/datapack-cache.bin.tmp "
                   f"{shlex.quote(ewd)}/datapack-cache.bin && "
                   f"./{BIN_NAME} save")

    out, msg = br.run_benchmark_on_exec(
        compile_node=compile_node,
        exec_node=exec_node,
        cmake_src_subdir="server/cli",
        build_subdir=f"benchmarkserversave-{label}",
        bin_name=BIN_NAME,
        runtime_cmd=runtime_cmd,
        profilers=runnable,
        cmake_defs=cmake_defs,
        run_timeout=RUN_TIMEOUT,
        verbose=True,
    )

    for prof in runnable:
        res = out.get(prof)
        if isinstance(res, dict) and res.get("rc") not in (None, 0):
            err_msg = res.get("error", f"exit code {res.get('rc')}")
            progress.emit(prof, "no", label, status="FAIL", extra=err_msg)
            per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err_msg}
            continue
        if res is None:
            if prof == "perf-stat" and msg == "ok":
                reason = bh.perf_no_hw_skip(label)
                progress.emit(prof, "no", label, status="SKIP", extra=reason)
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
            elif isinstance(msg, str) and msg.startswith("SKIP:"):
                # Infra failure (compile/push/bring-up) tagged by
                # benchmark_remote: unknown metric, not a regression.  Show the
                # full cause + re-run command under a banner.
                detail = msg[5:]
                bh.print_node_error("benchmarkserversave", label, "SKIP", detail)
                progress.emit(prof, "no", label, status="SKIP",
                              extra=detail[:80])
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
            else:
                detail = msg if msg != "ok" else "no-data"
                bh.print_node_error("benchmarkserversave", label, "FAIL", detail)
                progress.emit(prof, "no", label, status="FAIL",
                              extra=detail[:80])
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
            continue
        if prof == "rusage":
            cell = {}
            if res.get("wall_s") is not None:
                cell["wall_s"] = res["wall_s"]
            if res.get("max_rss_kb") is not None:
                cell["max_rss_kb"] = res["max_rss_kb"]
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _flat_to_metric_block(
                                         {k: (v, 0.0) for k, v in cell.items()})}
        elif prof in ("perf-stat", "callgrind", "binary-size"):
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _flat_to_metric_block(
                                         {str(k): (v, 0.0)
                                          for k, v in (res if isinstance(res, dict) else {}).items()
                                          if isinstance(v, (int, float))})}
        else:
            per_tool[label][prof] = {"status": "PASS", "metrics": {}}
        progress.emit(prof, "no", label, status="PASS")


def main():
    args = bh.parse_bench_args()
    bh.set_node_filter(args.node)
    if bh.acquire_singleton_lock("benchmarkserversave") is None:
        return 3
    br.set_benchmark_label("benchmarkserversave")
    if NETWORK_EXCLUSIVE:
        if bh.acquire_network_lock("benchmarkserversave") is None:
            return 3
    comment = args.comment
    maxtime = args.maxtime
    bin_path = build()
    if bin_path is None: return 2
    run_bin = setup_run_dir(bin_path)

    if args.profile:
        cleanup_save_artifacts()
        tools = bh.profile_tools(args.profile)
        local_timeouts = {t: RUN_TIMEOUT * {"callgrind": 30, "massif": 20,
                                            "perf": 2}.get(t, 1) for t in tools}
        maincode = _detect_maincode(DATAPACK_PATH)

        def _stage_serversave(runtime_node):
            if os.path.isdir(DATAPACK_PATH):
                rc_dp, msg_dp = br.rsync_datapack_to_exec(
                    runtime_node, DATAPACK_PATH, remote_subdir="datapack",
                    timeout=600, server_mode=True)
                if rc_dp != 0:
                    return False, f"datapack-rsync: {msg_dp[:80]}"
            br.write_server_properties_on_exec(runtime_node, maincode,
                                               port=61921,
                                               run_subdir=runtime_node["work_dir"])
            return True, "ok"

        remote_spec = {
            "cmake_src_subdir": "server/cli",
            "build_subdir_base": "benchmarkserversave",
            "bin_name": BIN_NAME,
            "cmake_defs": {"CMAKE_BUILD_TYPE": "Release",
                           "CMAKE_CXX_FLAGS_RELEASE": "-O2 -DNDEBUG",
                           "CATCHCHALLENGER_DB_INTERNAL_VARS": "ON",  # RAM DB; implies DB_FILE
                           "CATCHCHALLENGER_CACHE_HPS": "ON"},
            "runtime_cmd": (f"rm -f datapack-cache.bin.tmp datapack-cache.bin "
                            f"&& ./{BIN_NAME} save"),
            "stage_fn": _stage_serversave,
        }
        br.profile_fleet("benchmarkserversave", tools, [run_bin, "save"],
                         RUN_DIR, local_timeouts, remote_spec=remote_spec)
        return 0

    arch = bh.host_arch()
    # --node may exclude the host baseline; only prepend "local" when allowed.
    local_node = [{"label": "local", "arch": arch}] if bh.node_allowed("local", arch) else []
    nodes = local_node + bh.benchmark_exec_nodes()
    all_profilers = ["rusage", "binary-size", "perf-stat", "callgrind"]

    # Pre-resolve per-node profiler availability against the persisted
    # benchmark_disabled_tools list + a live runtime probe. Missing tools
    # may prompt the operator once (and the answer is persisted).
    node_profilers = {}
    node_skips     = {}
    for node in nodes:
        avail, skips = bh.profilers_runnable_on(node, all_profilers)
        node_profilers[node["label"]] = avail
        node_skips[node["label"]]     = skips

    total = sum(len(node_profilers[node["label"]]) for node in nodes)
    progress = bh.Progress(total, "benchmarkserversave")
    deadline = bh.FleetDeadline()

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    compile_flags = ["-O2", "-DCMAKE_BUILD_TYPE=Release",
                     "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
                     "-DCATCHCHALLENGER_CACHE_HPS=ON"]

    flat_local = {}
    per_tool   = {}    # node_label -> { tool -> {status, metrics} }
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
        # Per-node budget: restart the clock so a slow earlier node never
        # starves this one (see FleetDeadline).
        deadline.start_node(label)
        if deadline.reached():
            per_tool[label] = deadline.skip_node(label, all_profilers,
                                                 progress)
            continue
        if label != "local":
            _run_remote_cells_serversave(
                node, node_profilers[label], node_skips[label],
                all_profilers, progress, per_tool, compile_flags)
            continue
        for prof in all_profilers:
            if prof not in node_profilers[label]:
                reason = node_skips[label].get(prof, "tool-missing")
                progress.emit(prof, "no", label, status="SKIP", extra=reason)
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
                continue
            deadline.note(label, prof)
            cell, skip = cell_run(run_bin, prof)
            if cell is None:
                if skip and skip.startswith("SKIP:"):
                    reason = skip[5:]
                    progress.emit(prof, "no", label, status="SKIP", extra=reason)
                    per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
                else:
                    progress.emit(prof, "no", label, status="FAIL")
                    per_tool[label][prof] = {"status": "FAIL", "metrics": {}}
                continue
            flat_local.update(cell)
            progress.emit(prof, "no", label, status="PASS")
            per_tool[label][prof] = {"status": "PASS",
                                     "metrics": _flat_to_metric_block(cell)}

    sha = bh.git_sha()
    cand_stamp = started_utc.replace(":", "-")

    ended_utc = hr.iso_now()
    rec = {
        "commit": sha,
        "comment": comment,
        "date":   ended_utc,
        "started_utc": started_utc,
        "ended_utc": ended_utc,
        "duration_seconds": hr.duration_seconds(started_utc, ended_utc),
        "batch_id": batch_id,
        "benchmark": "benchmarkserversave",
        "nodes": {},
    }
    for node in nodes:
        label = node["label"]
        if label == "local" and flat_local:
            rec["nodes"][label] = {
                "arch": node.get("arch", "?"),
                "libs": bh.LIBS_BY_NODE.get(label, {}),
                "metrics": to_record_metrics(flat_local),
            }
        elif label != "local" and label in per_tool:
            m = {}
            for tool, blk in per_tool[label].items():
                for k, v in blk.get("metrics", {}).items():
                    m[k] = {"median": v.get("median", v.get("value")),
                            "stddev": v.get("stddev", 0.0),
                            "unit":   v.get("unit"),
                            "better": v.get("better", "lower")}
            if m:
                rec["nodes"][label] = {"arch": node.get("arch", "?"),
                                        "libs": bh.LIBS_BY_NODE.get(label, {}),
                                        "metrics": m}
    cand_p = bh.candidate_path("benchmarkserversave", cand_stamp)
    bh.write_record(cand_p, rec)
    print(_color(bh.C_CYAN, f"[record] candidate -> {cand_p}"))

    # Per-platform history -- one JSON per (benchmark, run, platform).
    for node in nodes:
        if node["label"] not in per_tool:
            continue
        if node["label"] == "local":
            runner = hr.local_runner
        else:
            runner = hr.make_ssh_runner(node.get("ssh_host"),
                                        node.get("ssh_user"),
                                        node.get("ssh_port", 22))
        if node["label"] == "local":
            cc_path, dp_path = RUN_DIR, DATAPACK_PATH
        else:
            wd = node.get("work_dir") or "/tmp/cc-bench-serversave"
            cc_path, dp_path = wd, os.path.join(wd, "datapack")
        pr = hr.PlatformRecord("benchmarkserversave", batch_id,
                               node["label"], runner=runner,
                               arch_hint=node.get("arch")).collect(
                                   cc_binary_path=cc_path,
                                   datapack_path=dp_path)
        for tool, blk in per_tool[node["label"]].items():
            pr.add_result(tool, blk["metrics"], status=blk["status"])
            # Single-workload benchmark -> emit one "default" slice
            # carrying cpu_percent so the per-CLAUDE.md mandatory
            # field is present in the JSON.
            if tool == "rusage" and blk["status"] == "PASS":
                slice_keys = ("cpu_percent", "wall_s", "user_s", "sys_s",
                              "max_rss_kb", "cache_bytes")
                slice_metrics = {k: blk["metrics"][k]
                                 for k in slice_keys if k in blk["metrics"]}
                if slice_metrics:
                    pr.add_subbenchmark(tool, "default", slice_metrics)
        out_p = pr.write(commit=sha, started_utc=started_utc,
                         ended_utc=ended_utc,
                         compile_flags=compile_flags
                             + list(br.exec_node_flag_defs(node).values()),
                         simd_tier="generic",
                         harness_version=hr.harness_version(),
                         comment=comment)
        if out_p is not None:
            print(_color(bh.C_CYAN, f"[history] {out_p}"))

    # Cross-platform champion compare. SKIP on a --node run OR a --maxtime
    # truncated run: decision + champion promotion need the WHOLE fleet.
    if not bh.node_filter_active() and not truncated:
        champ = bh.load_champion("benchmarkserversave")
        decision, summary = bh.decide_multi_node(champ, rec)
        bh.print_decision("benchmarkserversave", decision, summary)

        if decision == "KEEP":
            ch_p = bh.champion_path("benchmarkserversave")
            bh.write_record(ch_p, rec)
            print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

        hr.attach_decision("benchmarkserversave", batch_id, decision)
    import chart_generator
    for cp in chart_generator.regenerate("benchmarkserversave", cand_stamp):
        print(_color(bh.C_CYAN, f"[chart] {cp}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
