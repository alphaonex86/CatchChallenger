#!/usr/bin/env python3
"""benchmarkbotactions.py -- end-to-end network/kernel-stack benchmark.

Workload: build tools/bot-actions AND catchchallenger-server-cli, spawn
the server locally on a free port with the DB pinned in RAM
(CATCHCHALLENGER_DB_FILE_RAM, see general/DEFINES.md), then run
bot-actions against it. Unlike benchmarkserversave (cold-start parse)
or benchmarkmapmanager (in-process visibility loop), this exercises
the FULL stack inside one host:

  * userspace QTcpSocket + protocol encode/decode
  * kernel send/recv path through the loopback interface
  * server epoll loop + per-tick scheduling
  * server-side packet parse + state updates

The headline metric (bots-served / wall time) is dominated by network
behaviour and reflects real-world client cost, not micro-benchmark
cost. A patch that speeds the in-process loop but regresses syscall
churn or packet-coalescing shows up here, not in the others. By
spawning the server in-process the benchmark needs no external setup
-- "python3 benchmarkbotactions.py" works on a fresh checkout.

# HEADLESS: yes
Metrics:
  * wall_s              -- /usr/bin/time -v (lower better)
  * max_rss_kb          -- peak resident set (lower better)
  * user_s / sys_s      -- CPU split; sys_s grows with kernel/syscall
                            cost, the metric most likely to move on a
                            network-stack change (lower better)
  * vol_ctx / invol_ctx -- context switches (lower better)
  * net_rx_bytes /
    net_tx_bytes        -- /proc/net/dev delta on the loopback iface
                            (lower better at fixed workload = fewer wire
                            bytes per bot-second)
  * net_rx_pkts /
    net_tx_pkts         -- packet count delta (lower better)
  * tcp_retrans         -- /proc/net/snmp Tcp:RetransSegs delta (lower)
  * binary_size_bytes   -- stripped binary footprint (lower better)

Determinism note: TCP RTT inside loopback is ~microseconds, but the
server epoll callback latency and Qt event-loop scheduling still
introduce jitter. RUN_REPEATS produces several samples; the harness
records median+stddev so the comparator can apply the noise band.

One-command target: run with no args, 1h timeout. The server is
spawned in CATCHCHALLENGER_DB_FILE_RAM mode so every disk write goes
to /dev/null -- no leftover state between runs, no tmpfs cleanup, and
the kernel write() / fsync() overhead is excluded from the measured
metrics.

Bot-actions has no --duration flag; we wrap it in `timeout` and send
SIGINT so Qt can clean up. The headline workload knob is BOTS_COUNT
* DURATION_S -- enough offered load that the network stack, not the
CPU, dominates the wall clock.
"""
import os
import sys
import json
import shutil
import signal
import subprocess
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh
import history_recorder as hr

REPO_ROOT  = bh.REPO_ROOT
BENCH_DIR  = os.path.dirname(os.path.abspath(__file__))

BOT_SRC_DIR    = os.path.join(REPO_ROOT, "tools", "bot-actions")
BOT_BIN_NAME   = "bot-actions"
SRV_SRC_DIR    = os.path.join(REPO_ROOT, "server", "cli")
SRV_BIN_NAME   = "catchchallenger-server-cli"

# Datapack used by the embedded server. Pin to the upstream vanilla
# pack and the "test" maincode (small map set, fast parse) so the
# server is ready in a couple of seconds even on slow hosts.
DATAPACK_PATH  = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"

try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
    TMPFS_ROOT = test_config.TMPFS_ROOT
except Exception:
    BUILD_ROOT = "/tmp/cc-build"
    TMPFS_ROOT = "/tmp"
BOT_BUILD_DIR  = os.path.join(BUILD_ROOT, "benchmark", "benchmarkbotactions-bot")
SRV_BUILD_DIR  = os.path.join(BUILD_ROOT, "benchmark", "benchmarkbotactions-server")
SRV_RUN_DIR    = os.path.join(TMPFS_ROOT, "cc-bench-botactions-server")

# Workload knobs. Three sizes per benchmark/CLAUDE.md "Workload variety":
#   small  -- baseline syscall floor, only a couple of bots
#   medium -- typical per-tick offered load
#   large  -- saturate the NIC tx queue / kernel sk_buff pool
BOT_COUNTS    = [4, 32, 128]
DURATION_S    = 30
RUN_REPEATS   = 3        # 1 warmup + 2 measured per (size)
BUILD_TIMEOUT = 1800
RUN_TIMEOUT   = DURATION_S + 60

# Server boot deadline. Datapack parse + bind has to finish before the
# bots start; this matches the value the test/ harness uses for an
# amd64 host (slow nodes need more).
SERVER_READY_TIMEOUT = 60
SERVER_PORT          = 61920


def _color(c, s): return f"{c}{s}{bh.C_RESET}"


# ---- build --------------------------------------------------------------

def _cmake_build(src_dir, build_dir, extra_defs=None, label="build"):
    """Run cmake configure + build; return (rc, message). Caller maps rc."""
    if not os.path.isdir(src_dir):
        return 1, f"missing src: {src_dir}"
    os.makedirs(build_dir, exist_ok=True)
    print(_color(bh.C_CYAN, f"[{label}] {src_dir} -> {build_dir}"))
    cfg = ["cmake", "-S", src_dir, "-B", build_dir,
           "-DCMAKE_BUILD_TYPE=Release"]
    if extra_defs:
        for k, v in extra_defs.items():
            cfg.append(f"-D{k}={v}")
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=600)
    if rc != 0:
        print(sout); print(serr, file=sys.stderr)
        return rc, "cmake configure FAILED"
    bld = ["cmake", "--build", build_dir, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=BUILD_TIMEOUT)
    if rc != 0:
        print(sout); print(serr, file=sys.stderr)
        return rc, "cmake build FAILED"
    return 0, "ok"


def build_bot():
    rc, msg = _cmake_build(BOT_SRC_DIR, BOT_BUILD_DIR, label="build:bot")
    if rc != 0:
        print(_color(bh.C_RED, f"[build:bot] {msg}"))
        return None
    bin_path = os.path.join(BOT_BUILD_DIR, BOT_BIN_NAME)
    if not os.path.isfile(bin_path):
        print(_color(bh.C_RED, f"[build:bot] missing binary: {bin_path}"))
        return None
    print(_color(bh.C_GREEN, f"[build:bot] OK ({bh.binary_size(bin_path)} bytes)"))
    return bin_path


def build_server():
    """Build catchchallenger-server-cli with CATCHCHALLENGER_DB_FILE_RAM
    so the bench server runs with the DB pinned in RAM (every disk
    write -> /dev/null). The HPS cache + FileDB defines are mandatory
    pre-conditions of DB_FILE_RAM."""
    if not os.path.isdir(DATAPACK_PATH):
        print(_color(bh.C_RED, f"[build:server] datapack not found: {DATAPACK_PATH}"))
        return None
    rc, msg = _cmake_build(SRV_SRC_DIR, SRV_BUILD_DIR,
                           extra_defs={"CATCHCHALLENGER_DB_FILE":     "ON",
                                       "CATCHCHALLENGER_CACHE_HPS":   "ON",
                                       "CATCHCHALLENGER_DB_FILE_RAM": "ON"},
                           label="build:server")
    if rc != 0:
        print(_color(bh.C_RED, f"[build:server] {msg}"))
        return None
    bin_path = os.path.join(SRV_BUILD_DIR, SRV_BIN_NAME)
    if not os.path.isfile(bin_path):
        print(_color(bh.C_RED, f"[build:server] missing binary: {bin_path}"))
        return None
    print(_color(bh.C_GREEN, f"[build:server] OK ({bh.binary_size(bin_path)} bytes)"))
    return bin_path


# ---- embedded server lifecycle -----------------------------------------

def _detect_maincode(datapack_src):
    """Prefer 'test' (small map set, fast parse); fall back to first dir."""
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


def setup_server_run_dir(server_bin):
    """Stage SRV_RUN_DIR with: copy of the binary, datapack symlink, and
    server-properties.xml pinned to SERVER_PORT. Returns the staged
    binary path."""
    if os.path.isdir(SRV_RUN_DIR):
        shutil.rmtree(SRV_RUN_DIR, ignore_errors=True)
    os.makedirs(SRV_RUN_DIR, exist_ok=True)
    dst_bin = os.path.join(SRV_RUN_DIR, SRV_BIN_NAME)
    shutil.copy2(server_bin, dst_bin)
    os.chmod(dst_bin, 0o755)
    os.symlink(DATAPACK_PATH, os.path.join(SRV_RUN_DIR, "datapack"))
    maincode = _detect_maincode(DATAPACK_PATH)
    xml = os.path.join(SRV_RUN_DIR, "server-properties.xml")
    with open(xml, "w") as f:
        f.write(
            '<?xml version="1.0"?>\n'
            '<configuration>\n'
            f'    <server-port value="{SERVER_PORT}"/>\n'
            '    <automatic_account_creation value="true"/>\n'
            '    <max-players value="500"/>\n'
            '    <httpDatapackMirror value=""/>\n'
            '    <master>\n'
            f'        <external-server-port value="{SERVER_PORT}"/>\n'
            '    </master>\n'
            '    <content>\n'
            f'        <mainDatapackCode value="{maincode}"/>\n'
            '        <subDatapackCode value=""/>\n'
            '    </content>\n'
            '</configuration>\n'
        )
    return dst_bin


def start_server(staged_bin):
    """Spawn the server; block until 'correctly bind:' shows up on
    stderr or the deadline expires. Returns the Popen handle (caller
    must terminate when done)."""
    log_path = os.path.join(SRV_RUN_DIR, "server.log")
    log_fh   = open(log_path, "wb")
    p = subprocess.Popen([staged_bin],
                         cwd=SRV_RUN_DIR,
                         stdout=log_fh, stderr=subprocess.STDOUT,
                         start_new_session=True)
    deadline = time.monotonic() + SERVER_READY_TIMEOUT
    while time.monotonic() < deadline:
        if p.poll() is not None:
            log_fh.close()
            try:
                with open(log_path) as f: tail = f.read()[-2000:]
            except Exception:
                tail = ""
            print(_color(bh.C_RED, "[server] exited before ready"))
            print(tail, file=sys.stderr)
            return None
        try:
            with open(log_path, "rb") as f:
                if b"correctly bind:" in f.read():
                    print(_color(bh.C_GREEN,
                        f"[server] ready (pid={p.pid}, port={SERVER_PORT})"))
                    return p
        except FileNotFoundError:
            pass
        time.sleep(0.2)
    print(_color(bh.C_RED,
        f"[server] failed to bind within {SERVER_READY_TIMEOUT}s"))
    stop_server(p)
    return None


def stop_server(proc):
    """Send SIGINT to the server, wait briefly, escalate to SIGKILL."""
    if proc is None or proc.poll() is not None:
        return
    try:
        proc.send_signal(signal.SIGINT)
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=5)
    except Exception:
        pass


# ---- network counters ---------------------------------------------------

def _default_iface():
    """Bot traffic to the embedded server goes over loopback. The
    historical implementation used the default-route iface (for an
    external server); now that the server is in-process on localhost,
    'lo' is the only iface whose counters reflect this workload."""
    return "lo"


def _read_iface_counters(iface):
    """Return {rx_bytes, tx_bytes, rx_pkts, tx_pkts} from /proc/net/dev
    or None when the iface row is absent."""
    try:
        with open("/proc/net/dev") as f:
            for line in f:
                if ":" not in line:
                    continue
                name, _, rest = line.partition(":")
                if name.strip() != iface:
                    continue
                fields = rest.split()
                # rx: bytes packets errs drop fifo frame compressed multicast
                # tx: bytes packets errs drop fifo colls carrier compressed
                return {
                    "rx_bytes": int(fields[0]),
                    "rx_pkts":  int(fields[1]),
                    "tx_bytes": int(fields[8]),
                    "tx_pkts":  int(fields[9]),
                }
    except Exception:
        return None
    return None


def _read_tcp_retrans():
    """/proc/net/snmp Tcp: RetransSegs (column 12 per Linux convention).
    Returns None on parse failure."""
    try:
        with open("/proc/net/snmp") as f:
            header = None
            for line in f:
                if line.startswith("Tcp:"):
                    if header is None:
                        header = line.strip().split()[1:]
                    else:
                        vals = line.strip().split()[1:]
                        d = dict(zip(header, vals))
                        return int(d.get("RetransSegs", 0))
    except Exception:
        return None
    return None


# ---- run one cell -------------------------------------------------------

def _bot_cmd(bin_path, host, port, bots):
    return [bin_path,
            "--host",  str(host),
            "--port",  str(port),
            "--bots",  str(bots),
            "--login", "bench_%NUMBER%",
            "--pass",  "bench_%NUMBER%"]


def _run_once(bin_path, host, port, bots, iface):
    """Run bot-actions for DURATION_S seconds, return sample dict or
    None on failure. Uses `timeout --signal=INT` so Qt can clean up
    sockets gracefully -- a hard SIGKILL leaves TIME_WAIT churn that
    pollutes the NEXT iteration's counters."""
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"

    pre_iface = _read_iface_counters(iface) if iface else None
    pre_retrans = _read_tcp_retrans()

    cmd = ["timeout", "--signal=INT", str(DURATION_S)] + \
          _bot_cmd(bin_path, host, port, bots)

    full = ["/usr/bin/time", "-v"] + cmd
    t0 = time.monotonic()
    p = subprocess.run(full, env=env, capture_output=True, text=True,
                       timeout=RUN_TIMEOUT)
    wall = time.monotonic() - t0
    # timeout(1) returns 124 on timeout-expired (= our intended path),
    # 0 on natural exit. Anything else = bot or server failure.
    if p.returncode not in (0, 124, 128 + signal.SIGINT):
        return None

    post_iface = _read_iface_counters(iface) if iface else None
    post_retrans = _read_tcp_retrans()

    sample = {"wall_s": wall, "user_s": None, "sys_s": None,
              "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
              "minor_pf": None, "major_pf": None}
    for line in p.stderr.splitlines():
        s = line.strip()
        if s.startswith("Elapsed (wall clock) time"):
            v = bh._parse_wall(s.split(": ", 1)[1])
            if v is not None: sample["wall_s"] = v
        elif s.startswith("User time (seconds):"):
            try: sample["user_s"] = float(s.split(":", 1)[1])
            except: pass
        elif s.startswith("System time (seconds):"):
            try: sample["sys_s"] = float(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Maximum resident set size (kbytes):"):
            try: sample["max_rss_kb"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Voluntary context switches:"):
            try: sample["vol_ctx"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Involuntary context switches:"):
            try: sample["invol_ctx"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Minor (reclaiming a frame) page faults:"):
            try: sample["minor_pf"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Major (requiring I/O) page faults:"):
            try: sample["major_pf"] = int(s.split(":", 1)[1])
            except: pass

    if pre_iface and post_iface:
        sample["net_rx_bytes"] = post_iface["rx_bytes"] - pre_iface["rx_bytes"]
        sample["net_tx_bytes"] = post_iface["tx_bytes"] - pre_iface["tx_bytes"]
        sample["net_rx_pkts"]  = post_iface["rx_pkts"]  - pre_iface["rx_pkts"]
        sample["net_tx_pkts"]  = post_iface["tx_pkts"]  - pre_iface["tx_pkts"]
    if pre_retrans is not None and post_retrans is not None:
        sample["tcp_retrans"] = post_retrans - pre_retrans

    return sample


def cell_run(bin_path, host, port, bots, iface):
    """RUN_REPEATS passes (drop first as warmup). Returns per-metric
    {name -> (median, stddev)} for one (bots) cell."""
    samples = []
    for i in range(RUN_REPEATS):
        s = _run_once(bin_path, host, port, bots, iface)
        if s is None:
            return None
        if i == 0:
            continue
        samples.append(s)
    if not samples:
        return None
    keys = set()
    for s in samples:
        keys.update(k for k, v in s.items() if v is not None)
    out = {}
    for k in keys:
        vals = [s[k] for s in samples if s.get(k) is not None]
        med, std = bh.stats_of(vals)
        if med is not None:
            out[k] = (med, std)
    return out


def _unit_for(name):
    if name.endswith("_s"):     return "s"
    if name.endswith("_kb"):    return "kb"
    if name.endswith("_bytes"): return "bytes"
    if name.endswith("_pkts"):  return "count"
    return "count"


def _flat_to_metric_block(flat):
    out = {}
    for name, (med, std) in flat.items():
        out[name] = {"value": med, "median": med, "stddev": std,
                     "unit": _unit_for(name), "better": "lower",
                     "samples": None}
    return out


def main():
    # Build BOTH binaries first so a build failure aborts before we
    # spend wall-time on cell runs / server boot.
    bin_path = build_bot()
    if bin_path is None:
        return 2
    server_bin = build_server()
    if server_bin is None:
        return 2

    staged = setup_server_run_dir(server_bin)
    server_proc = start_server(staged)
    if server_proc is None:
        return 2
    try:
        return _run_with_server(bin_path, server_proc)
    finally:
        print(_color(bh.C_CYAN, "[server] stopping"))
        stop_server(server_proc)


def _run_with_server(bin_path, server_proc):
    host = "127.0.0.1"
    port = SERVER_PORT
    print(_color(bh.C_CYAN, f"[bench] target = {host}:{port} (embedded)"))

    iface = _default_iface()

    arch = bh.host_arch()
    # bot-actions is GUI-linked (Qt6 Widgets); on remote exec_nodes it
    # only runs where has_gui=True OR QT_QPA_PLATFORM=offscreen works,
    # which is the same gate the headless flag in this file's doc-
    # block flips. The remote dispatch is not wired in this benchmark
    # so we emit SKIP for every remote node.
    nodes = [{"label": "local", "arch": arch}] + bh.benchmark_exec_nodes()
    all_profilers = ["rusage", "binary-size", "perf-stat"]

    # Pre-resolve per-node profiler availability against the persisted
    # benchmark_disabled_tools list + a live probe.
    node_profilers = {}
    node_skips     = {}
    for node in nodes:
        avail, skips = bh.profilers_runnable_on(node, all_profilers)
        node_profilers[node["label"]] = avail
        node_skips[node["label"]]     = skips

    total = sum(len(all_profilers) + max(len(BOT_COUNTS) - 1, 0) for _ in nodes)
    progress = bh.Progress(total)

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release"]

    per_tool = {}     # node_label -> { tool -> {status, metrics} }
    for node in nodes:
        label = node["label"]
        per_tool[label] = {}
        if label != "local":
            for prof in all_profilers:
                progress.emit(prof, "no", label, status="SKIP",
                              extra="remote-dispatch-not-wired")
                per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
            # account for the extra BOT_COUNTS-1 emits the local rusage path makes
            for _ in range(max(len(BOT_COUNTS) - 1, 0)):
                progress.emit("rusage", "no", label, status="SKIP",
                              extra="remote-dispatch-not-wired")
            continue

        # rusage cell: one record per BOT_COUNTS size; metrics keyed
        # with b<count>_ prefix so the schema stays one-flat-dict.
        if "rusage" in node_profilers[label]:
            all_metrics = {}
            rusage_status = "PASS"
            for bots in BOT_COUNTS:
                cell = cell_run(bin_path, host, port, bots, iface)
                if cell is None:
                    progress.emit("rusage", "no", label, status="FAIL",
                                  extra=f"bots={bots}")
                    rusage_status = "FAIL"
                    continue
                for name, (med, std) in cell.items():
                    all_metrics[f"b{bots}_{name}"] = (med, std)
                progress.emit("rusage", "no", label, status="PASS",
                              extra=f"bots={bots}")
            per_tool[label]["rusage"] = {
                "status":  rusage_status,
                "metrics": _flat_to_metric_block(all_metrics),
            }
        else:
            for bots in BOT_COUNTS:
                progress.emit("rusage", "no", label, status="SKIP",
                              extra=node_skips[label].get("rusage", "tool-missing"))
            per_tool[label]["rusage"] = {"status": "SKIP", "metrics": {}}

        # binary-size cell (deterministic, single value)
        if "binary-size" in node_profilers[label]:
            sz = bh.binary_size(bin_path)
            if sz is not None:
                per_tool[label]["binary-size"] = {
                    "status":  "PASS",
                    "metrics": _flat_to_metric_block({"binary_size_bytes": (sz, 0.0)}),
                }
                progress.emit("binary-size", "no", label, status="PASS")
            else:
                per_tool[label]["binary-size"] = {"status": "FAIL", "metrics": {}}
                progress.emit("binary-size", "no", label, status="FAIL")
        else:
            progress.emit("binary-size", "no", label, status="SKIP",
                          extra=node_skips[label].get("binary-size", "tool-missing"))
            per_tool[label]["binary-size"] = {"status": "SKIP", "metrics": {}}

        # perf-stat cell: medium workload only (small=noise floor,
        # large=multi-process scheduler effects make perf counters
        # noisy). Single run, no aggregation -- perf is deterministic.
        if "perf-stat" in node_profilers[label]:
            env = os.environ.copy()
            env["QT_QPA_PLATFORM"] = "offscreen"
            cmd = ["timeout", "--signal=INT", str(DURATION_S)] + \
                  _bot_cmd(bin_path, host, port, BOT_COUNTS[len(BOT_COUNTS) // 2])
            out = bh.measure_perf_stat(cmd, env=env, timeout=RUN_TIMEOUT)
            if out:
                m = {}
                for evt, val in out.items():
                    m[f"perf_{evt}"] = (val, 0.0)
                per_tool[label]["perf-stat"] = {
                    "status":  "PASS",
                    "metrics": _flat_to_metric_block(m),
                }
                progress.emit("perf-stat", "no", label, status="PASS")
            else:
                per_tool[label]["perf-stat"] = {"status": "FAIL", "metrics": {}}
                progress.emit("perf-stat", "no", label, status="FAIL")
        else:
            progress.emit("perf-stat", "no", label, status="SKIP",
                          extra=node_skips[label].get("perf-stat", "tool-missing"))
            per_tool[label]["perf-stat"] = {"status": "SKIP", "metrics": {}}

    # Persist + champion compare (host arch only; remote skipped).
    sha = bh.git_sha()
    local_tools = per_tool.get("local", {})
    # Flatten every tool's metrics into one record for champion compare.
    flat_record_metrics = {}
    for tool, blk in local_tools.items():
        for name, m in blk["metrics"].items():
            flat_record_metrics[name] = {
                "median": m["median"], "stddev": m["stddev"] or 0.0,
                "unit":   m["unit"],   "better": m["better"],
            }
    rec = {
        "commit":   sha,
        "date":     time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "node":     "local",
        "metrics":  flat_record_metrics,
        "profilers": {"binary-size": bin_path},
    }
    cand_p = bh.candidate_path("benchmarkbotactions", arch, sha)
    bh.write_record(cand_p, rec)
    print(_color(bh.C_CYAN, f"[record] candidate -> {cand_p}"))

    champ = bh.load_champion("benchmarkbotactions", arch)
    decision, summary = bh.decide(champ, rec)
    bh.print_decision("benchmarkbotactions", arch, decision, summary)

    if decision == "KEEP":
        ch_p = bh.champion_path("benchmarkbotactions", arch)
        bh.write_record(ch_p, rec)
        print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

    # Per-platform history.
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
        pr = hr.PlatformRecord("benchmarkbotactions", batch_id,
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

    hr.attach_decision("benchmarkbotactions", batch_id, decision)
    import chart_generator
    for cp in chart_generator.regenerate("benchmarkbotactions"):
        print(_color(bh.C_CYAN, f"[chart] {cp}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
