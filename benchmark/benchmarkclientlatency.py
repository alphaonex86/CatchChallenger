#!/usr/bin/env python3
"""benchmarkclientlatency.py -- client-perceived latency, measured from OUT
OF VIEW of the target hardware.

The SERVER is the thing under test: it is built and run on each exec node
(the constrained target). The measuring CLIENT (tools/bot-actions, run
offscreen with QT_QPA_PLATFORM=offscreen) runs on the orchestrating HOST and
connects to that server over the REAL network link -- so the recorded
net_link / eth / wifi fields finally matter. The local/host row is the
loopback baseline (server + client both on the host).

bot-actions is built with -DCATCHCHALLENGER_BENCHMARK=ON, which compiles in
LatencyRecorder + the --latency mode. The bot drives deterministic,
server-safe traffic and, on SIGINT, dumps a log2-ns latency histogram per
metric as "BENCH <m>_lat_hist_<i>=<count>" lines on stdout. The server is
also built with CATCHCHALLENGER_BENCHMARK so its own per-read-event histogram
(BENCH lat_hist_*) gives the server-side tail for the same run.

# HEADLESS: yes  (the bot runs offscreen; no display server needed)
Metrics (all nanoseconds, lower-is-better unless noted), per workload slice:
  * chat_p50_ns / chat_p99_ns / chat_jitter_ns   -- local chat A->server->B
                       ("time to send a message to another player"). SAMPLED.
  * rtt_p50_ns  / rtt_p99_ns  / rtt_jitter_ns    -- local chat echoed back to
                       its own sender ("query-with-reply round trip"). SAMPLED.
  * join_p50_ns / join_p99_ns / join_jitter_ns   -- a bot reaching the map ->
                       another bot sees it ("see another player update").
  * <m>_count                                    -- samples (higher = more
                       statistical confidence; not a perf metric).
  * cpu_percent (%)                              -- the measuring client's CPU
                       over the slice (lower-is-better at equal latency).
  * server_p99_ns / server_reqs                  -- server-side tail + work
                       (from the server BENCH dump; reqs higher-is-better).
Noise: chat/rtt/join are SAMPLED -- network jitter dominates, so a wifi / VPN
/ tunnel node must NOT be the sole KEEP/DISCARD signal (escalate instead).

One-command target: run with no args (1h timeout). Server runs in
CATCHCHALLENGER_DB_INTERNAL_VARS mode (DB tree in /dev/shm tmpfs -- no disk
I/O pollutes the score).
"""
import os
import sys
import time
import shlex
import shutil
import signal
import socket
import subprocess

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh
import history_recorder as hr
import benchmark_remote as br

REPO_ROOT = bh.REPO_ROOT
BENCH = "benchmarkclientlatency"

# This benchmark binds a fixed server port + drives the NIC/loopback, so it
# must run serially against other network benchmarks (shared lock).
NETWORK_EXCLUSIVE = True

BOT_SRC_DIR  = os.path.join(REPO_ROOT, "tools", "bot-actions")
BOT_BIN_NAME = "bot-actions"
SRV_SRC_DIR  = os.path.join(REPO_ROOT, "server", "cli")
SRV_BIN_NAME = "catchchallenger-server-cli"
DATAPACK_PATH = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"

try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
    TMPFS_ROOT = test_config.TMPFS_ROOT
except Exception:
    BUILD_ROOT = "/mnt/data/perso/tmpfs"
    TMPFS_ROOT = "/dev/shm"

BOT_BUILD_DIR = os.path.join(BUILD_ROOT, "benchmark", "clientlatency-bot")
SRV_BUILD_DIR = os.path.join(BUILD_ROOT, "benchmark", "clientlatency-server")
SRV_RUN_DIR   = os.path.join(TMPFS_ROOT, "cc-bench-clientlat-server")
BOT_RUN_TMP   = os.path.join(TMPFS_ROOT, "cc-bench-clientlat-bot")

SERVER_PORT = 61921

# Server build: RAM DB (DB tree in tmpfs) + HPS cache + benchmark counters.
SERVER_DEFS = {
    "CATCHCHALLENGER_DB_INTERNAL_VARS": "ON",
    "CATCHCHALLENGER_CACHE_HPS":        "ON",
    "CATCHCHALLENGER_BENCHMARK":        "ON",
    "CMAKE_BUILD_TYPE":                 "Release",
}
BOT_DEFS = {"CATCHCHALLENGER_BENCHMARK": "ON", "CMAKE_BUILD_TYPE": "Release"}

# Workload sweep (concurrent bots on the same map). small/medium/large per
# benchmark/CLAUDE.md "Workload variety". More bots = more co-located peers =
# more chat/join samples, but also more offered load on the constrained server.
BOT_COUNTS = [(4, "small"), (16, "medium"), (50, "large")]

# Fixed wall-clock budget per measured slice (benchmark/CLAUDE.md
# "fixed-time, not fixed-iteration"): the slice runs for BUDGET seconds and
# reports the work done in that window (a latency histogram of EVERY event
# observed + the sample count) -- never a fixed number of iterations. Many
# events accumulate inside the window, so percentiles are robust; the timing
# itself is two cheap nsecsElapsed() reads per (sparse) network event, so it
# perturbs no platform. The budget is SHORTER on execution nodes: they are the
# constrained targets (i486/MIPS/ARM) reached over a real, often slow link, so
# a long window wastes fleet wall-time without sharpening the distribution.
BUDGET_S_LOCAL  = 15     # host loopback baseline (fast)
BUDGET_S_REMOTE = 8      # constrained exec node over the real link (short)
RUN_REPEATS = 2          # measured runs per slice (median across them)
# Allowance for the bot to reach the map (datapack load + N logins) before the
# self-timed measurement window opens. The bot's own autoConnect timeout is
# 60s; this backstop must exceed it.
STARTUP_ALLOWANCE_S = 75
RUN_MARGIN_S = 20
BUILD_TIMEOUT = 1800
SERVER_READY_TIMEOUT = 90

# Hard wall-clock cap for the WHOLE batch: this benchmark must finish in <=1h.
# Once exceeded at a node boundary we stop launching more nodes and finalise
# with what completed (an un-run node is "unknown", never a regression).
MAX_BATCH_S = 3300       # 55 min, leaving headroom for record + charts

# Low-impact parallelism: the remote build + server bring-up runs ON the
# compile/exec nodes (no host CPU, only orchestration), so it is parallelised.
# The latency MEASUREMENT itself stays SERIAL -- one bot fleet on the host at a
# time -- because concurrent host load would perturb the very latency we
# measure (NOT low impact). Bandwidth: bring-up rsyncs are one-off; the
# measurement traffic is tiny (chat probes), so the link is not the bottleneck.
MAX_PARALLEL_BRINGUP = 4


def _c(colour, s):
    return f"{colour}{s}{bh.C_RESET}"


# ---- builds --------------------------------------------------------------

def _cmake_build(src_dir, build_dir, defs, label):
    """Configure + build one binary locally. Returns (rc, combined_log)."""
    os.makedirs(build_dir, exist_ok=True)
    cfg = ["cmake", "-S", src_dir, "-B", build_dir]
    for k, v in defs.items():
        cfg.append(f"-D{k}={v}")
    r1 = subprocess.run(cfg, capture_output=True, text=True, timeout=BUILD_TIMEOUT)
    if r1.returncode != 0:
        return r1.returncode, r1.stdout + r1.stderr
    bld = ["cmake", "--build", build_dir, "--", "-j", str(os.cpu_count() or 1)]
    r2 = subprocess.run(bld, capture_output=True, text=True, timeout=BUILD_TIMEOUT)
    return r2.returncode, r2.stdout + r2.stderr


def build_bot_local():
    """Build bot-actions (the measuring client) on the host. A local build
    failure aborts the whole benchmark (no cell can run without the client)."""
    print(_c(bh.C_CYAN, "[build:bot] bot-actions (CATCHCHALLENGER_BENCHMARK=ON)"))
    rc, log = _cmake_build(BOT_SRC_DIR, BOT_BUILD_DIR, BOT_DEFS, "build:bot")
    if rc != 0:
        bh.print_local_build_error("build:bot", "cmake", log, "")
        sys.exit(1)
    return os.path.join(BOT_BUILD_DIR, BOT_BIN_NAME)


def build_server_local():
    print(_c(bh.C_CYAN, "[build:server] catchchallenger-server-cli (RAM DB + BENCHMARK)"))
    rc, log = _cmake_build(SRV_SRC_DIR, SRV_BUILD_DIR, SERVER_DEFS, "build:server")
    if rc != 0:
        bh.print_local_build_error("build:server", "cmake", log, "")
        sys.exit(1)
    return os.path.join(SRV_BUILD_DIR, SRV_BIN_NAME)


# ---- server config -------------------------------------------------------

def _detect_maincode(datapack_src):
    """Prefer 'test' (small map set, fast parse); fall back to first dir."""
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return "test"
    entries = sorted(os.listdir(map_main))
    if "test" in entries and os.path.isdir(os.path.join(map_main, "test")):
        return "test"
    for e in entries:
        if os.path.isdir(os.path.join(map_main, e)):
            return e
    return "test"


def server_properties_xml(maincode, port):
    """server-properties.xml for the benchmark. The DDOS limits are raised
    well above default (chat 5/5s, move 60/5s) on purpose: we measure
    latency, not the anti-DDOS protection, and the latency client's probe
    rate would otherwise trip the kicker. Not a production config."""
    return (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        f'    <server-port value="{port}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        '    <max-players value="500"/>\n'
        '    <httpDatapackMirror value=""/>\n'
        f'    <master><external-server-port value="{port}"/></master>\n'
        f'    <content><mainDatapackCode value="{maincode}"/><subDatapackCode value=""/></content>\n'
        '    <DDOS>\n'
        '        <kickLimitChat value="250"/>\n'
        '        <kickLimitMove value="250"/>\n'
        '        <kickLimitOther value="250"/>\n'
        '        <dropGlobalChatMessageGeneral value="250"/>\n'
        '        <dropGlobalChatMessageLocalClan value="250"/>\n'
        '        <dropGlobalChatMessagePrivate value="250"/>\n'
        '    </DDOS>\n'
        '</configuration>\n'
    )


# ---- local server lifecycle ---------------------------------------------

def setup_local_server(server_bin):
    if os.path.isdir(SRV_RUN_DIR):
        shutil.rmtree(SRV_RUN_DIR, ignore_errors=True)
    os.makedirs(SRV_RUN_DIR, exist_ok=True)
    dst = os.path.join(SRV_RUN_DIR, SRV_BIN_NAME)
    shutil.copy2(server_bin, dst)
    os.chmod(dst, 0o755)
    os.symlink(DATAPACK_PATH, os.path.join(SRV_RUN_DIR, "datapack"))
    maincode = _detect_maincode(DATAPACK_PATH)
    with open(os.path.join(SRV_RUN_DIR, "server-properties.xml"), "w") as f:
        f.write(server_properties_xml(maincode, SERVER_PORT))
    return dst


def start_local_server(staged_bin):
    log_path = os.path.join(SRV_RUN_DIR, "server.log")
    log_fh = open(log_path, "wb")
    p = subprocess.Popen([staged_bin], cwd=SRV_RUN_DIR,
                         stdout=log_fh, stderr=subprocess.STDOUT,
                         start_new_session=True)
    deadline = time.monotonic() + SERVER_READY_TIMEOUT
    while time.monotonic() < deadline:
        if p.poll() is not None:
            log_fh.close()
            try:
                tail = open(log_path).read()[-2000:]
            except Exception:
                tail = ""
            print(_c(bh.C_RED, "[server] exited before ready"))
            print(tail, file=sys.stderr)
            return None
        try:
            with open(log_path, "rb") as f:
                if b"correctly bind:" in f.read():
                    print(_c(bh.C_GREEN, f"[server] ready (pid={p.pid}, port={SERVER_PORT})"))
                    return p
        except FileNotFoundError:
            pass
        time.sleep(0.2)
    print(_c(bh.C_RED, f"[server] failed to bind within {SERVER_READY_TIMEOUT}s"))
    stop_local_server(p)
    return None


def stop_local_server(proc):
    if proc is None or proc.poll() is not None:
        return
    try:
        proc.send_signal(signal.SIGINT)
        proc.wait(timeout=10)
    except Exception:
        try:
            proc.kill()
        except Exception:
            pass


def _port_reachable(host, port, timeout=3.0):
    """True if a TCP connect to host:port succeeds within `timeout`."""
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except Exception:
        return False


def _server_cpu_ticks(pid):
    """utime+stime jiffies of a local server pid, or None."""
    try:
        with open(f"/proc/{pid}/stat") as f:
            parts = f.read().split()
        return int(parts[13]) + int(parts[14])
    except Exception:
        return None


# ---- one bot slice -------------------------------------------------------

def run_bot_slice(bot_bin, host, port, bots, budget, server_pid=None):
    """Run the bot --latency for one (bots) slice for `budget` seconds, SIGINT
    it, parse the client BENCH dump (chat/join/rtt percentiles) + client CPU%.
    Returns (metrics_dict, err_or_None). metrics keys are *_p50_ns etc.,
    plus cpu_percent and (when server_pid given) server_cpu_percent."""
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    run_tmp = os.path.join(BOT_RUN_TMP, f"p{port}-b{bots}")
    os.makedirs(run_tmp, exist_ok=True)
    out_log = os.path.join(run_tmp, "bot_stdout.log")
    err_log = os.path.join(run_tmp, "bot_stderr.log")

    # The bot self-times a FIXED `budget`-second window that starts only once
    # every bot is on the map (--latency-seconds), then dumps BENCH + exits --
    # so process startup (datapack load, N logins) is EXCLUDED from the
    # measurement. `timeout` is just a hard backstop = startup allowance +
    # window + margin (always exceeds the real run; only fires on a hang).
    hardstop = STARTUP_ALLOWANCE_S + budget + RUN_MARGIN_S
    cmd = ["/usr/bin/time", "-v",
           "timeout", "--signal=INT", str(hardstop),
           bot_bin, "--host", str(host), "--port", str(port),
           "--bots", str(bots),
           "--login", "bench_%NUMBER%", "--pass", "bench_%NUMBER%",
           "--latency", "--latency-seconds", str(budget)]

    pre_ticks = _server_cpu_ticks(server_pid) if server_pid else None
    t0 = time.monotonic()
    try:
        with open(out_log, "wb") as o, open(err_log, "wb") as e:
            subprocess.run(cmd, env=env, stdout=o, stderr=e,
                           timeout=hardstop + 15)
    except subprocess.TimeoutExpired:
        return None, f"bot run timed out (bots={bots})"
    wall = time.monotonic() - t0
    post_ticks = _server_cpu_ticks(server_pid) if server_pid else None

    try:
        out_text = open(out_log, "r", errors="replace").read()
    except Exception:
        out_text = ""
    try:
        err_text = open(err_log, "r", errors="replace").read()
    except Exception:
        err_text = ""

    per = bh.parse_client_bench_stdout(out_text)
    if not per:
        return None, (f"no client BENCH output (bots={bots}); "
                      f"bot stderr tail:\n" + "\n".join(err_text.splitlines()[-20:]))

    metrics = {}
    for m in ("chat", "join", "rtt"):
        stats = per.get(m)
        if stats:
            for k in ("p50_ns", "p95_ns", "p99_ns", "p999_ns", "jitter_ns"):
                metrics[f"{m}_{k}"] = {"value": stats[k], "unit": "ns", "better": "lower"}
            metrics[f"{m}_count"] = {"value": stats["count"], "unit": "samples", "better": "higher"}

    # client CPU% from /usr/bin/time -v ("Percent of CPU this job got: 99%")
    cpu_pct = None
    for line in err_text.splitlines():
        s = line.strip()
        if s.startswith("Percent of CPU this job got:"):
            try:
                cpu_pct = float(s.split(":", 1)[1].strip().rstrip("%"))
            except Exception:
                pass
    if cpu_pct is not None:
        metrics["cpu_percent"] = {"value": cpu_pct, "unit": "%", "better": "lower"}

    # server CPU% (single-thread, bounded 100) for a local server pid
    if pre_ticks is not None and post_ticks is not None and wall > 0:
        try:
            hz = os.sysconf("SC_CLK_TCK")
        except Exception:
            hz = 100
        srv = (post_ticks - pre_ticks) / hz / wall * 100.0
        if srv > 100.0:
            srv = 100.0
        metrics["server_cpu_percent"] = {"value": srv, "unit": "%", "better": "lower"}

    return metrics, None


def _median_metrics(samples):
    """Median across repeats for each metric name; preserves unit/better."""
    if not samples:
        return {}
    keys = set()
    for s in samples:
        keys.update(s.keys())
    out = {}
    for k in keys:
        vals = [s[k]["value"] for s in samples if k in s]
        med, std = bh.stats_of(vals)
        if med is not None:
            ref = next(s[k] for s in samples if k in s)
            out[k] = {"value": med, "unit": ref["unit"], "better": ref["better"],
                      "samples": vals, "median": med, "stddev": std}
    return out


def run_all_slices(bot_bin, host, port, budget, server_pid=None):
    """Run every workload slice (1 warmup dropped + RUN_REPEATS measured each)
    for `budget` seconds each. Returns (slices_dict {label: metrics},
    err_or_None). slices include the mandatory cpu_percent. Stops + returns
    the error on first hard failure."""
    slices = {}
    for bots, size in BOT_COUNTS:
        label = f"{bots}-bots"
        # 1 warmup (drop) so socket/TIME_WAIT churn doesn't pollute repeat 1.
        _m, _e = run_bot_slice(bot_bin, host, port, bots, budget, server_pid)
        reps = []
        for _ in range(RUN_REPEATS):
            m, e = run_bot_slice(bot_bin, host, port, bots, budget, server_pid)
            if e is not None:
                return slices, f"slice {label}: {e}"
            reps.append(m)
        slices[label] = _median_metrics(reps)
        prog = ", ".join(
            f"{k}={int(slices[label][k]['value'])}"
            for k in ("rtt_p50_ns", "chat_p50_ns", "join_p50_ns")
            if k in slices[label])
        print(_c(bh.C_GREEN, f"    [{size}] {label}: {prog}"))
    return slices, None


def read_server_bench_text(local_run_dir=None, exec_node=None):
    """Return the server.log text holding its BENCH dump (local file or via
    ssh on the exec node). '' when unavailable."""
    if local_run_dir is not None:
        try:
            return open(os.path.join(local_run_dir, "server.log"), "r", errors="replace").read()
        except Exception:
            return ""
    if exec_node is not None:
        ewd = exec_node["work_dir"]
        rc, so, _se = br.ssh_run(exec_node["user"], exec_node["host"],
                                 exec_node.get("port", 22),
                                 f"cat {shlex.quote(ewd + '/server.log')} 2>/dev/null",
                                 timeout=20)
        return so if rc == 0 else ""
    return ""


def server_metrics_from_text(text):
    """Server-side tail + work from its BENCH dump: server_p99_ns,
    server_jitter_ns, server_reqs. {} when no histogram was dumped."""
    import re
    buckets = {}
    for m in re.finditer(r"BENCH lat_hist_(\d+)=(\d+)", text):
        buckets[int(m.group(1))] = int(m.group(2))
    out = {}
    stats = bh.bench_hist_percentiles(buckets)
    if stats:
        out["server_p99_ns"] = {"value": stats["p99_ns"], "unit": "ns", "better": "lower"}
        out["server_jitter_ns"] = {"value": stats["jitter_ns"], "unit": "ns", "better": "lower"}
    mp = re.search(r"BENCH packets_in=(\d+)", text)
    if mp:
        out["server_reqs"] = {"value": int(mp.group(1)), "unit": "events", "better": "higher"}
    return out


# ---- headline (flat) metrics for the decision matrix ---------------------

def headline_metrics(slices):
    """Flat per-node metrics the cross-fleet decision matrix compares. Use the
    'medium' slice tails (the typical per-tick load) as the headline."""
    medium = None
    for bots, size in BOT_COUNTS:
        if size == "medium":
            medium = slices.get(f"{bots}-bots")
    if medium is None:
        return {}
    out = {}
    for k in ("rtt_p50_ns", "rtt_p99_ns", "chat_p50_ns", "chat_p99_ns",
              "join_p99_ns"):
        if k in medium:
            v = medium[k]
            out[k] = {"median": v.get("median", v["value"]),
                      "stddev": v.get("stddev", 0.0),
                      "unit": v["unit"], "better": v["better"]}
    return out


# ---- remote bring-up (server on exec node, bot on host over the LAN) ------

def bringup_remote_server(node):
    """PHASE A (parallelised, low host-impact): build the server on the exec
    node's compile parent, push it + datapack + HPS cache, start it on the
    exec node and wait for bind. Returns a dict describing the outcome -- it
    NEVER touches shared progress/record state (thread-safe), so the caller
    (main thread) emits progress + records serially. On any infra failure
    the dict status is "SKIP" (an unmeasured node is unknown, never a
    regression). Banners are printed here (stdout interleave is acceptable)."""
    label = node["label"]
    compile_node = node.get("compile_node")
    if compile_node is None:
        return {"node": node, "status": "SKIP", "reason": "no-compile-node"}
    exec_node = {"label": label,
                 "user": node.get("ssh_user"), "host": node.get("ssh_host"),
                 "port": node.get("ssh_port", 22),
                 "work_dir": node.get("work_dir") or f"/tmp/cc-bench-{BENCH}",
                 "lxc_nfs": node.get("lxc_nfs")}
    maincode = _detect_maincode(DATAPACK_PATH)
    try:
        rc, msg = br.rsync_datapack_to_exec(exec_node, DATAPACK_PATH,
                                            remote_subdir="datapack",
                                            timeout=600, server_mode=True)
        if rc != 0:
            raise RuntimeError(f"datapack-rsync: {msg}")
        rc, msg, rbuild = br.build_on_compile_node(
            compile_node, cmake_src_subdir="server/cli",
            build_subdir=f"{BENCH}-srv-{label}",
            cmake_defs=SERVER_DEFS, exec_node=exec_node, verbose=True)
        if rc != 0:
            bh.print_node_error(BENCH, label, "SKIP", f"remote server build failed:\n{msg}")
            return {"node": node, "status": "SKIP", "reason": "server-build-failed"}
        rc, exec_bin, msg = br.push_binary_to_exec(compile_node, exec_node,
                                                   rbuild, SRV_BIN_NAME, verbose=True)
        if rc != 0:
            raise RuntimeError(f"push-binary: {msg}")
        xml = server_properties_xml(maincode, SERVER_PORT)
        esc = xml.replace("'", "'\\''")
        br.ssh_run(exec_node["user"], exec_node["host"], exec_node["port"],
                   f"printf '%s' '{esc}' > {shlex.quote(exec_node['work_dir'] + '/server-properties.xml')}",
                   timeout=15)
        rc, rcache, msg = br.pregenerate_datapack_cache(
            compile_node, rbuild, SRV_BIN_NAME, DATAPACK_PATH, maincode,
            server_port=SERVER_PORT, verbose=True)
        if rc == 0:
            br.stage_cache_on_exec(compile_node, exec_node, rcache, verbose=True)
        srv_ssh, srv_pid, reason = br.start_server_popen(exec_node, SRV_BIN_NAME)
        if srv_ssh is None:
            raise RuntimeError(f"server-start: {reason}")
        eu, eh, ep = exec_node["user"], exec_node["host"], exec_node["port"]
        log_q = shlex.quote(exec_node["work_dir"] + "/server.log")
        bound = False
        deadline = time.monotonic() + SERVER_READY_TIMEOUT
        while time.monotonic() < deadline:
            rc, st, _ = br.ssh_run(eu, eh, ep,
                f"if grep -q 'correctly bind:' {log_q} 2>/dev/null; then echo BIND; "
                f"elif kill -0 {srv_pid} 2>/dev/null; then echo WAIT; else echo DEAD; fi",
                timeout=10)
            if "BIND" in st:
                bound = True
                break
            if "DEAD" in st:
                break
            time.sleep(0.5)
        if not bound:
            try:
                br.stop_server_popen(srv_ssh, exec_node, srv_pid)
            except Exception:
                pass
            raise RuntimeError("server did not bind in time")
        return {"node": node, "status": "READY", "exec_node": exec_node,
                "srv_ssh": srv_ssh, "srv_pid": srv_pid}
    except Exception as ex:
        bh.print_node_error(BENCH, label, "SKIP", f"remote bring-up failed: {ex}")
        return {"node": node, "status": "SKIP", "reason": str(ex)}


def measure_remote_server(ready, bot_bin, per_subbench, per_headline):
    """PHASE B (serial, main thread): run the local bot --latency against one
    already-running remote server over the real link, record, then stop it.
    Serial on purpose: concurrent host load would perturb the latency."""
    node = ready["node"]
    label = node["label"]
    exec_node = ready["exec_node"]
    srv_ssh = ready["srv_ssh"]
    srv_pid = ready["srv_pid"]
    try:
        # Fast reachability gate: the server bound on the node, but its game
        # port must also be reachable from THIS host (the client) over the
        # link. If it isn't (NAT / firewall / SSH-only fleet), skip in ~3s
        # instead of letting the bot wait out its full startup allowance.
        if not _port_reachable(exec_node["host"], SERVER_PORT):
            br.stop_server_popen(srv_ssh, exec_node, srv_pid)
            srv_ssh = None
            bh.print_node_error(BENCH, label, "SKIP",
                f"game port {SERVER_PORT} not reachable from host at "
                f"{exec_node['host']} (server bound on node, but host->node "
                f"port is closed: NAT/firewall/SSH-only fleet)")
            return "SKIP", "game-port-unreachable-from-host"
        slices, err = run_all_slices(bot_bin, exec_node["host"], SERVER_PORT,
                                     BUDGET_S_REMOTE, server_pid=None)
        br.stop_server_popen(srv_ssh, exec_node, srv_pid)
        srv_ssh = None
        if err is not None or not slices:
            bh.print_node_error(BENCH, label, "SKIP", err or "no slices produced")
            return "SKIP", (err or "no-slices")
        srv_text = read_server_bench_text(exec_node=exec_node)
        smetrics = server_metrics_from_text(srv_text)
        if smetrics:
            for lbl in slices:
                slices[lbl].update(smetrics)
        per_subbench[label] = {"latency": slices}
        per_headline[label] = headline_metrics(slices)
        return "PASS", None
    except Exception as ex:
        bh.print_node_error(BENCH, label, "SKIP", f"remote measure failed: {ex}")
        return "SKIP", str(ex)
    finally:
        if srv_ssh is not None:
            try:
                br.stop_server_popen(srv_ssh, exec_node, srv_pid)
            except Exception:
                pass


# ---- main ----------------------------------------------------------------

def main():
    args = bh.parse_bench_args()
    comment = args.comment

    if os.path.isdir(BOT_RUN_TMP):
        shutil.rmtree(BOT_RUN_TMP, ignore_errors=True)
    os.makedirs(BOT_RUN_TMP, exist_ok=True)

    if not os.path.isdir(DATAPACK_PATH):
        print(_c(bh.C_RED, f"[fatal] datapack not found: {DATAPACK_PATH}"))
        return 1

    # serialise against other network benchmarks
    lock = bh.acquire_network_lock(BENCH)

    bot_bin = build_bot_local()

    arch = bh.host_arch()
    local_node = [{"label": "local", "arch": arch}] if bh.node_allowed("local", arch) else []
    nodes = local_node + bh.benchmark_exec_nodes()

    # progress total: one "latency" cell per node.
    progress = bh.Progress(len(nodes), BENCH)

    batch_id = hr.new_batch_id()
    started_utc = hr.iso_now()
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release", "-DCATCHCHALLENGER_BENCHMARK"]

    per_subbench = {}   # label -> {"latency": {slice_label: metrics}}
    per_headline = {}   # label -> {metric: {median,stddev,unit,better}}
    t_batch = time.monotonic()

    def _over_cap():
        return (time.monotonic() - t_batch) > MAX_BATCH_S

    # ---- LOCAL row (serial, loopback baseline) ---------------------------
    if any(n["label"] == "local" for n in nodes):
        srv_bin = build_server_local()
        staged = setup_local_server(srv_bin)
        proc = start_local_server(staged)
        if proc is None:
            progress.emit("latency", "no", "local", status="SKIP", extra="server-start")
        else:
            try:
                slices, err = run_all_slices(bot_bin, "127.0.0.1", SERVER_PORT,
                                             BUDGET_S_LOCAL, server_pid=proc.pid)
            finally:
                stop_local_server(proc)
            if err is not None or not slices:
                progress.emit("latency", "no", "local", status="SKIP",
                              extra=(err or "no-slices")[:80])
                bh.print_node_error(BENCH, "local", "SKIP", err or "no slices")
            else:
                srv_text = read_server_bench_text(local_run_dir=SRV_RUN_DIR)
                smetrics = server_metrics_from_text(srv_text)
                if smetrics:
                    for lbl in slices:
                        slices[lbl].update(smetrics)
                per_subbench["local"] = {"latency": slices}
                per_headline["local"] = headline_metrics(slices)
                progress.emit("latency", "no", "local", status="PASS")

    # ---- REMOTE nodes: PARALLEL bring-up, SERIAL measurement -------------
    remote_nodes = [n for n in nodes if n["label"] != "local"]
    if remote_nodes:
        import concurrent.futures
        readies = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_PARALLEL_BRINGUP) as ex:
            futs = {ex.submit(bringup_remote_server, n): n for n in remote_nodes}
            for fut in concurrent.futures.as_completed(futs):
                try:
                    readies.append(fut.result())
                except Exception as e:
                    readies.append({"node": futs[fut], "status": "SKIP", "reason": str(e)})
        # Serial measurement keeps the latency clean; honour the 1h cap.
        for r in readies:
            label = r["node"]["label"]
            if r["status"] != "READY":
                progress.emit("latency", "no", label, status="SKIP",
                              extra=(r.get("reason") or "skip")[:80])
                continue
            if _over_cap():
                try:
                    br.stop_server_popen(r["srv_ssh"], r["exec_node"], r["srv_pid"])
                except Exception:
                    pass
                progress.emit("latency", "no", label, status="SKIP", extra="batch-maxtime")
                continue
            st, reason = measure_remote_server(r, bot_bin, per_subbench, per_headline)
            progress.emit("latency", "no", label, status=st,
                          extra=("" if st == "PASS" else (reason or "skip")[:80]))

    # ---- candidate record + cross-fleet decision -------------------------
    sha = bh.git_sha()
    cand_stamp = started_utc.replace(":", "-")
    ended_utc = hr.iso_now()
    rec = {
        "commit": sha, "comment": comment, "date": ended_utc,
        "started_utc": started_utc, "ended_utc": ended_utc,
        "batch_id": batch_id, "benchmark": BENCH, "nodes": {},
    }
    for node in nodes:
        label = node["label"]
        m = per_headline.get(label)
        if m:
            rec["nodes"][label] = {"arch": node.get("arch", "?"),
                                   "libs": bh.LIBS_BY_NODE.get(label, {}),
                                   "metrics": m}
    cand_p = bh.candidate_path(BENCH, cand_stamp)
    bh.write_record(cand_p, rec)
    print(_c(bh.C_CYAN, f"[record] candidate -> {cand_p}"))

    decision = None
    # A partial run (node filter, or the 1h cap truncated the fleet) must NOT
    # promote/demote the champion -- that needs the whole fleet.
    partial = bh.node_filter_active() or _over_cap()
    if not partial:
        champ = bh.load_champion(BENCH)
        decision, summary = bh.decide_multi_node(champ, rec)
        bh.print_decision(BENCH, decision, summary)
        if decision == "KEEP":
            bh.write_record(bh.champion_path(BENCH), rec)
            print(_c(bh.C_GREEN, f"[champion] promoted -> {bh.champion_path(BENCH)}"))

    # ---- per-platform history --------------------------------------------
    for node in nodes:
        label = node["label"]
        if label not in per_subbench:
            continue
        if label == "local":
            runner = hr.local_runner
            cc_path, dp_path = SRV_RUN_DIR, DATAPACK_PATH
        else:
            runner = hr.make_ssh_runner(node.get("ssh_host"), node.get("ssh_user"),
                                        node.get("ssh_port", 22))
            wd = node.get("work_dir") or f"/tmp/cc-bench-{BENCH}"
            cc_path, dp_path = wd, os.path.join(wd, "datapack")
        pr = hr.PlatformRecord(BENCH, batch_id, label, runner=runner,
                               arch_hint=node.get("arch")).collect(
                                   cc_binary_path=cc_path, datapack_path=dp_path)
        # flat headline as the tool's metrics; slices as subbenchmarks.
        flat = {}
        for k, v in per_headline.get(label, {}).items():
            flat[k] = {"value": v["median"], "unit": v["unit"], "better": v["better"],
                       "median": v["median"], "stddev": v["stddev"]}
        pr.add_result("latency", flat, status="PASS")
        for slabel, smetrics in per_subbench[label].get("latency", {}).items():
            pr.add_subbenchmark("latency", slabel, smetrics)
        out_p = pr.write(commit=sha, started_utc=started_utc, ended_utc=ended_utc,
                         compile_flags=compile_flags, simd_tier="generic",
                         harness_version=hr.harness_version(), comment=comment)
        if out_p is not None:
            print(_c(bh.C_CYAN, f"[history] {out_p}"))

    if not partial:
        hr.attach_decision(BENCH, batch_id, decision)
    import chart_generator
    for cp in chart_generator.regenerate(BENCH, cand_stamp):
        print(_c(bh.C_CYAN, f"[chart] {cp}"))

    del lock
    return 0


if __name__ == "__main__":
    sys.exit(main())
