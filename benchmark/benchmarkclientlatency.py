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

# ESP32 all-in-one server (catchchallenger-esp32): firmware-baked datapack +
# settings, fileless, WiFi, max-players ~10. It is ALREADY running on the LAN;
# the harness only points the host bot at its fixed IP -- NO ssh, NO compile
# node, NO flashing from here (network-only). WiFi => comparison-only: recorded
# in history + the cross-node chart but NEVER fed to the KEEP/DISCARD champion
# decision (a WiFi node must not be the sole signal -- benchmark/CLAUDE.md).
# Nothing in remote_nodes.json is touched (operator-owned); the node is injected
# programmatically. Override the IP/port with CC_ESP32_HOST / CC_ESP32_PORT.
ESP32_NODE = {
    "label": "catchchallenger-esp32",
    "arch":  "xtensa-lx6",
    "host":  os.environ.get("CC_ESP32_HOST", "192.168.158.160"),
    "port":  int(os.environ.get("CC_ESP32_PORT", "42498")),  # its own game port
    "net_link": "wifi",
    "network_only": True,     # marker: skip the whole ssh/build/bring-up path
}

# The ESP32 runs at its REAL RAM CEILING, not the fleet's full 4/16/50: a
# no-PSRAM ESP32 (~290 KiB heap) physically cannot seat 50 concurrent WiFi
# clients (proven: WiFi+lwIP+80 KiB stack+per-client pool wall out around the
# mid-30s for BOOTING and lower once TIME_WAIT residue on the tiny lwIP PCB pool
# is accounted for). So measure the counts the board CAN actually seat — its
# small/medium align with the fleet's labels (4-bots / 16-bots) so the ESP32
# lands in the SHARED metric panels of champion-by-execution-node.svg next to
# i7-11370H. The "large" count is the board's own ceiling (ESP32-specific panel).
# The fleet's 50-bot slice is recorded as a documented over-ceiling SKIP (see
# ESP32_OVER_CEILING) so the history makes the hardware limit explicit rather
# than silently omitting it.
ESP32_BOT_COUNTS = [(4, "small"), (16, "medium"), (24, "large")]

# Bot counts the no-PSRAM board provably CANNOT seat — recorded as documented
# OOM-skips (never attempted on the board: attempting would only churn the tiny
# lwIP pool and yield bogus "50-seat" data from the ~24 that fit). PSRAM (WROVER)
# is the only way to lift this; see server/cli/esp32/sdkconfig.defaults.
ESP32_OVER_CEILING = [(50, "large", "RAM ceiling: no-PSRAM ESP32 seats ~24 < 50")]

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
    # HARD RULE (CLAUDE.md): never point a runtime binary's datapack dir
    # at the SOURCE datapack. COPY rather than symlink so no writer can
    # ever reach the source (testinggateway.stage_gateway_datapack lesson).
    # SRV_RUN_DIR is tmpfs, wiped above; outside the measured window.
    shutil.copytree(DATAPACK_PATH, os.path.join(SRV_RUN_DIR, "datapack"),
                    symlinks=False)
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


def run_all_slices(bot_bin, host, port, budget, server_pid=None, counts=None):
    """Run every workload slice (1 warmup dropped + RUN_REPEATS measured each)
    for `budget` seconds each. Returns (slices_dict {label: metrics},
    err_or_None). slices include the mandatory cpu_percent. Stops + returns
    the error on first hard failure. `counts` overrides BOT_COUNTS (the ESP32
    passes ESP32_BOT_COUNTS to stay under its small max-players)."""
    slices = {}
    for bots, size in (counts or BOT_COUNTS):
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


def run_slices_tolerant(bot_bin, host, port, budget, counts=None):
    """Like run_all_slices but a slice that FAILS (the constrained board can't
    seat all its bots / OOMs) is SKIPPED gracefully and the sweep CONTINUES to
    the next slice, instead of aborting on the first failure. Used for the ESP32:
    its RAM ceiling means 50 bots may not all reach the map even though 4/16 do,
    and that partial result IS the headline (which counts fit). Returns
    (slices_dict {label: metrics for the slices that produced data},
    skipped_list [(label, reason), ...]). Never raises for a slice failure."""
    slices = {}
    skipped = []
    for bots, size in (counts or BOT_COUNTS):
        label = f"{bots}-bots"
        # 1 warmup (drop) so socket/TIME_WAIT churn doesn't pollute repeat 1.
        run_bot_slice(bot_bin, host, port, bots, budget, server_pid=None)
        reps = []
        slice_err = None
        rep_i = 0
        while rep_i < RUN_REPEATS:
            m, e = run_bot_slice(bot_bin, host, port, bots, budget, server_pid=None)
            if e is not None:
                slice_err = e
                break
            reps.append(m)
            rep_i += 1
        if slice_err is not None or not reps:
            skipped.append((label, slice_err or "no data"))
            short = (slice_err or "no data").splitlines()[0][:90]
            print(_c(bh.C_YELLOW, f"    [{size}] {label}: SKIP ({short})"))
        else:
            slices[label] = _median_metrics(reps)
            prog = ", ".join(
                f"{k}={int(slices[label][k]['value'])}"
                for k in ("rtt_p50_ns", "chat_p50_ns", "join_p50_ns")
                if k in slices[label])
            print(_c(bh.C_GREEN, f"    [{size}] {label}: {prog}"))
    return slices, skipped


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
    'medium' slice tails (the typical per-tick load) as the headline. When the
    medium slice is missing (a constrained node — e.g. the ESP32 — OOM-skipped
    that bot count but produced a smaller one), fall back to the largest slice
    that DID produce data, so the node still appears in the shared chart panels."""
    medium = None
    for bots, size in BOT_COUNTS:
        if size == "medium":
            medium = slices.get(f"{bots}-bots")
    if medium is None:
        # Largest available slice (by bot count) that has map-level metrics.
        best_n = -1
        for lbl, m in slices.items():
            if "rtt_p50_ns" in m:
                try:
                    n = int(lbl.split("-")[0])
                except (ValueError, IndexError):
                    n = 0
                if n > best_n:
                    best_n = n
                    medium = m
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
        # keep_skins=True: the bot logs into a fresh RAM DB with no character,
        # so it CREATES one (skinId 0). Without >=1 real skin (folder with
        # back/front/trainer.png) the server rejects skin 0 and the bot never
        # reaches the map -> no latency data. serversave/mapmanager don't need
        # this (no character creation), so the flag is clientlatency-only.
        rc, msg = br.rsync_datapack_to_exec(exec_node, DATAPACK_PATH,
                                            remote_subdir="datapack",
                                            timeout=600, server_mode=True,
                                            keep_skins=True)
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
            server_port=SERVER_PORT, verbose=True, keep_skins=True)
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


# ---- ESP32 (network-only): no ssh/build/flash, measure the running board ---

def esp32_platform_fields():
    """Synthesized platform metadata for the ESP32 history JSON. The board has
    no shell, so PlatformRecord.collect() (which shells out via a runner) cannot
    run -- we fill the fields directly. Values are the fixed ESP32 spec; ram is
    left null (the schema is int MB and 520 KiB SRAM would record a misleading
    '1 MB' -- the precise figure lives in ram_type)."""
    return {
        "arch":        "xtensa-lx6",
        "cpu_model":   "Espressif ESP32 (Xtensa LX6 dual-core)",
        "cpu_cores":   2,
        "cpu_mhz":     240.0,
        "cpu_flags":   None,
        "cpu_cache":   [],
        "ram_total_mb": None,
        "ram_type":    "SRAM ~520KiB (no DRAM)",
        "disk_root":   None,
        "disk_kind":   "flash",          # datapack baked in SPI flash, fileless
        "net_card":    "ESP32 WiFi (802.11 b/g/n)",
        "net_link":    "wifi",
        "net_link_detail": None,
        "wifi_ssid":   None,             # not host-observable
        "wifi_standard": "wifi-4 (802.11n)",
        "wifi_band":   "2.4GHz",
        "wifi_link_mbps": None,
        "eth_link_mbps": None,
        "virt_kind":   "bare-metal",
        "virt_type":   "none",
        "arch_emulated": False,
        "host_arch":   None,
        "kernel":      None,             # ESP-IDF/FreeRTOS firmware, no OS kernel
        "libc":        "newlib",
        "compiler":    "xtensa-esp32-elf-gcc",
    }


def stage_bot_datapack(src_datapack):
    """Copy a datapack next to the bot binary (applicationDirPath()/datapack/),
    so the bot loads it LOCALLY and never tries to download from the fileless
    ESP32. Uses the SAME media-stripped copy the ESP32 baked, to maximise the
    datapack-hash match the client needs to skip download and reach the map."""
    dst = os.path.join(BOT_BUILD_DIR, "datapack")
    if os.path.isdir(dst):
        shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src_datapack, dst)
    return dst


# The ESP32 datapack mirror (nginx serving the SAME media-stripped tree the
# firmware was baked from). When the bot's local datapack hash matches the
# firmware's, the client skips download entirely; the mirror is the FALLBACK so
# an empty-local-datapack bot can still reach the map. Started here (idempotent)
# right before the ESP32 measurement so it is up regardless of operator state.
ESP32_MIRROR_CTL = os.environ.get(
    "CC_ESP32_MIRROR_CTL",
    "/mnt/data/perso/progs/catchchallenger-ESP32/mirror/start_stop.sh")


def ensure_esp32_mirror():
    """Make sure the ESP32 datapack mirror is running (idempotent). Returns
    (ok, detail). Best-effort: a failure here is not fatal — when the bot's local
    datapack already hash-matches the firmware, the mirror is never fetched."""
    if not os.path.isfile(ESP32_MIRROR_CTL):
        return False, f"mirror control script not found: {ESP32_MIRROR_CTL}"
    try:
        st = subprocess.run(["bash", ESP32_MIRROR_CTL, "status"],
                            capture_output=True, text=True, timeout=20)
        if "RUNNING" in (st.stdout + st.stderr):
            return True, "mirror already running"
    except Exception:
        pass
    # Not running -> launch detached (start_stop.sh start execs nginx in the
    # foreground; nginx itself daemonises via its pid file, so we background the
    # wrapper and give it a moment to build the tree + bind :8099).
    try:
        subprocess.Popen(["bash", ESP32_MIRROR_CTL, "start"],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                         start_new_session=True)
    except Exception as ex:
        return False, f"mirror start failed to launch: {ex}"
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline:
        try:
            st = subprocess.run(["bash", ESP32_MIRROR_CTL, "status"],
                                capture_output=True, text=True, timeout=20)
            if "RUNNING" in (st.stdout + st.stderr):
                return True, "mirror started"
        except Exception:
            pass
        time.sleep(1)
    return False, "mirror did not report RUNNING within 30s"


def _pctl(sorted_vals, q):
    """Linear-interpolated percentile of a pre-sorted list (no numpy)."""
    import math
    if not sorted_vals:
        return 0.0
    if len(sorted_vals) == 1:
        return sorted_vals[0]
    idx = q * (len(sorted_vals) - 1)
    lo = int(math.floor(idx))
    hi = int(math.ceil(idx))
    if lo == hi:
        return sorted_vals[lo]
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (idx - lo)


def esp32_max_concurrent(host, port, ceiling=20):
    """Open simultaneous TCP connections until the server stops accepting;
    return how many it held at once (the lwIP socket / accept ceiling)."""
    conns = []
    while len(conns) < ceiling:
        try:
            conns.append(socket.create_connection((host, port), timeout=4))
        except OSError:
            break
    held = len(conns)
    for s in conns:
        try:
            s.close()
        except OSError:
            pass
    return held


def esp32_connect_slice(host, port, hold_n, budget_s):
    """Hold `hold_n` background connections (offered load), then measure
    fresh TCP connect latency for `budget_s`. Pure-Python, black-box: the only
    thing a FILELESS server reliably supports (no datapack download -> no map ->
    the map-level bot metrics are N/A). Connect latency = WiFi RTT + the single-
    threaded epoll accept path, which IS a real comparable signal. Returns a
    metrics dict in the harness shape."""
    bg = []
    while len(bg) < hold_n:
        try:
            bg.append(socket.create_connection((host, port), timeout=4))
        except OSError:
            break
    held = len(bg)
    t_user0 = sum(os.times()[:2])
    lat_ms = []
    ok = 0
    fail = 0
    t0 = time.monotonic()
    while (time.monotonic() - t0) < budget_s:
        t = time.perf_counter()
        try:
            c = socket.create_connection((host, port), timeout=4)
            lat_ms.append((time.perf_counter() - t) * 1000.0)
            ok += 1
            c.close()
        except OSError:
            fail += 1
        time.sleep(0.02)   # ~50 conn/s ceiling; we measure latency, not flood
    wall = time.monotonic() - t0
    for s in bg:
        try:
            s.close()
        except OSError:
            pass
    cpu_pct = (sum(os.times()[:2]) - t_user0) / wall * 100.0 if wall > 0 else 0.0
    m = {}
    if lat_ms:
        sl = sorted(lat_ms)
        med, std = bh.stats_of(lat_ms)
        m["connect_p50_ms"]    = {"value": _pctl(sl, 0.50), "unit": "ms", "better": "lower"}
        m["connect_p95_ms"]    = {"value": _pctl(sl, 0.95), "unit": "ms", "better": "lower"}
        m["connect_p99_ms"]    = {"value": _pctl(sl, 0.99), "unit": "ms", "better": "lower"}
        m["connect_jitter_ms"] = {"value": std or 0.0,      "unit": "ms", "better": "lower"}
    m["connects_per_s"] = {"value": (ok / wall if wall > 0 else 0.0), "unit": "conn/s", "better": "higher"}
    m["connect_count"]  = {"value": ok,     "unit": "samples", "better": "higher"}
    m["connect_fail"]   = {"value": fail,   "unit": "count",   "better": "lower"}
    m["bg_held"]        = {"value": held,   "unit": "conns",   "better": "higher"}
    m["cpu_percent"]    = {"value": cpu_pct, "unit": "%",      "better": "lower"}
    return m


# Background-connection hold levels for the ESP32 capacity sweep (stay within
# its small socket pool: lwIP MAX_SOCKETS=16, max-players=10).
ESP32_HOLD_LEVELS = [(0, "small"), (4, "medium"), (8, "large")]


def measure_esp32(node, bot_bin, per_subbench, per_headline, per_netfamily):
    """Bring the ESP32 server online (flash-if-needed via benchmark_esp32; fast
    no-op if it already answers), then measure it.

    FIRST attempt the SAME map-level bot benchmark the rest of the fleet runs
    (chat/rtt/join) at the SAME bot counts (BOT_COUNTS: 4/16/50) so the slice
    labels (4-bots/16-bots/50-bots) ALIGN with every other node and the ESP32
    lands in the shared metric panels of champion-by-execution-node.svg.

    Reaching the map: pre-seed the bot's datapack with the EXACT media-stripped
    tree the firmware was baked from (so the client's aggregate hash matches the
    server's and it SKIPS the mirror download -- the fileless ESP32 serves no
    files). The nginx mirror (same tree) is also brought up as a FALLBACK for an
    empty-local-datapack bot. The sweep is TOLERANT: a slice the board can't seat
    (RAM ceiling -> OOM / not all bots reach the map) is SKIPPED gracefully and
    the remaining slices still record. Whichever counts produced data are kept
    under the SAME metric names as every other node -> directly comparable.

    Only if NO slice produced map-level data do we FALL BACK to the black-box
    CONNECT-level probe (TCP connect latency = WiFi RTT + the single-threaded
    epoll accept path, connect throughput, concurrent-connection capacity) so the
    node still produces some datum. Returns (status, reason).

    BLOCKED (need serial/ESP-IDF, self-skip, never FAIL): boot-to-bind,
    on-device free-heap, on-device map-management ops/s."""
    label, host, port = node["label"], node["host"], node["port"]
    import benchmark_esp32 as esp
    # Mirror up first (idempotent): the empty-local-datapack fallback needs it; a
    # hash-matching local datapack never fetches it, so a mirror failure is not
    # fatal -- log and continue.
    mok, mdetail = ensure_esp32_mirror()
    print(_c(bh.C_CYAN if mok else bh.C_YELLOW, f"[esp32] mirror: {mdetail}"))
    online_host, detail = esp.ensure_online(flash_if_needed=True, verbose=True)
    if online_host is None:
        bh.print_node_error(BENCH, label, "SKIP",
            f"ESP32 not available: {detail}. boot-to-bind + free-heap + "
            f"on-device map-mgmt also need serial/ESP-IDF (blocked). "
            f"-- comparison datum missing this batch")
        return "SKIP", "esp32-unavailable"
    host = online_host
    print(_c(bh.C_CYAN, f"[esp32] {detail}"))
    if not _port_reachable(host, port):
        bh.print_node_error(BENCH, label, "SKIP",
            f"ESP32 game port {port} not reachable at {host}")
        return "SKIP", "esp32-unreachable"

    # ---- MAP-LEVEL attempt — DISABLED on this no-PSRAM board ---------------
    # The map-level bot sweep (rtt/chat/join, comparable to the fleet) is NOT
    # viable on a plain ESP32 and is gated OFF by ESP32_TRY_MAPLEVEL below.
    # Investigated thoroughly (2026-06): the bots connect, but reaching the map
    # for N players is blocked by a CHAIN of device-specific issues —
    #   (1) a SERVER duplicate-character-id on xtensa (the in-memory DB hands out
    #       1,1,2,3 for distinct accounts; local x86 epoll AND select give
    #       1,2,3,4, so it is not reproducible off-device),
    #   (2) a cold-boot onboarding stall (only the first account creates in 60s),
    #   (3) the ~20-socket lwIP ceiling + 60s onboarding window for >=16 bots.
    # Worse, the attempt itself hammers the tiny board into an unstable state
    # (needs a reflash), so running it every batch would corrupt the very
    # connect-level datum we DO want. Decision (operator): ship the ESP32-vs-
    # fleet comparison via benchmarkmapmanager (50-player on-device CPU, already
    # charted next to i7) and record ESP32 latency at the CONNECT level only.
    # Lifting this needs a PSRAM board (WROVER) or the xtensa server fix; flip
    # the flag back on then. The client-side libbot blockers found on the way
    # (re-entrant BOT_ABORT, cold-create on-map signal) were real and are fixed.
    ESP32_TRY_MAPLEVEL = False
    if bot_bin is not None and ESP32_TRY_MAPLEVEL:
        try:
            stage_bot_datapack(esp.STAGE_DATAPACK)
            print(_c(bh.C_CYAN, "[esp32] datapack pre-seeded next to bot; trying "
                                "map-level bot --latency at fleet counts "
                                f"({', '.join(str(b) for b, _ in ESP32_BOT_COUNTS)})"))
            mslices, mskipped = run_slices_tolerant(bot_bin, host, port,
                                                    BUDGET_S_REMOTE,
                                                    counts=ESP32_BOT_COUNTS)
        except Exception as ex:
            mslices, mskipped = {}, [("all", f"map-level attempt raised: {ex}")]
        if mslices:
            per_subbench[label] = {"latency": mslices}
            per_headline[label] = headline_metrics(mslices)
            per_netfamily[label] = "ipv4"
            got = ", ".join(sorted(mslices.keys()))
            oom = ", ".join(f"{l}({r.splitlines()[0][:40]})" for l, r in mskipped)
            print(_c(bh.C_GREEN, f"[esp32] bot reached the map -- recorded "
                                 f"map-level chat/rtt/join for: {got}"))
            if mskipped:
                print(_c(bh.C_YELLOW, f"[esp32] OOM-skipped slices (RAM ceiling): {oom}"))
            return "PASS", None
        print(_c(bh.C_YELLOW, "[esp32] no map-level slice reached the map "
                              f"({mskipped}); falling back to connect-level probe"))
    elif not ESP32_TRY_MAPLEVEL:
        print(_c(bh.C_CYAN, "[esp32] map-level disabled (no-PSRAM RAM ceiling + "
                            "xtensa server bug); using connect-level probe"))
    else:
        print(_c(bh.C_YELLOW, "[esp32] no bot binary built; connect-level probe only"))

    # ---- FALLBACK: black-box CONNECT-level probe ---------------------------
    # Latency slices FIRST (clean), each after a short settle so the previous
    # slice's TIME_WAIT churn on the ESP32's tiny socket pool has drained.
    slices = {}
    for hold, size in ESP32_HOLD_LEVELS:
        time.sleep(2)
        m = esp32_connect_slice(host, port, hold, BUDGET_S_REMOTE)
        slices[f"hold-{hold}"] = m
        p50 = m.get("connect_p50_ms", {}).get("value")
        cps = m.get("connects_per_s", {}).get("value", 0.0)
        print(_c(bh.C_GREEN, f"    [{size}] hold-{hold}: "
                 f"connect_p50={p50:.1f}ms, {cps:.0f} conn/s" if p50 is not None
                 else f"    [{size}] hold-{hold}: no successful connect"))
    # Capacity LAST: the burst of simultaneous connections churns the small lwIP
    # socket pool, so measure it after the latency slices to avoid perturbing them.
    time.sleep(2)
    cap = esp32_max_concurrent(host, port)
    print(_c(bh.C_GREEN, f"    [capacity] max concurrent TCP connections: {cap}"))
    for s in slices.values():
        s["max_concurrent"] = {"value": cap, "unit": "conns", "better": "higher"}
    if not any(s.get("connect_p50_ms") for s in slices.values()):
        bh.print_node_error(BENCH, label, "SKIP",
            "no successful TCP connect to the ESP32 (server up but not accepting?)")
        return "SKIP", "no-connect"
    per_subbench[label] = {"latency": slices}
    # headline from the "medium" (hold-4) slice, falling back to any slice.
    med = slices.get("hold-4") or next(iter(slices.values()))
    head = {}
    for k in ("connect_p50_ms", "connect_p99_ms", "connect_jitter_ms",
              "connects_per_s", "max_concurrent"):
        if k in med:
            v = med[k]
            head[k] = {"median": v["value"], "stddev": 0.0,
                       "unit": v["unit"], "better": v["better"]}
    per_headline[label] = head
    per_netfamily[label] = "ipv4"
    return "PASS", None


# ---- main ----------------------------------------------------------------

def main():
    args = bh.parse_bench_args()
    bh.set_node_filter(args.node)
    comment = args.comment

    if os.path.isdir(BOT_RUN_TMP):
        shutil.rmtree(BOT_RUN_TMP, ignore_errors=True)
    os.makedirs(BOT_RUN_TMP, exist_ok=True)

    if not os.path.isdir(DATAPACK_PATH):
        print(_c(bh.C_RED, f"[fatal] datapack not found: {DATAPACK_PATH}"))
        return 1

    # serialise against other network benchmarks
    lock = bh.acquire_network_lock(BENCH)

    arch = bh.host_arch()
    local_node = [{"label": "local", "arch": arch}] if bh.node_allowed("local", arch) else []
    nodes = local_node + bh.benchmark_exec_nodes()
    # ESP32 comparison node, injected without touching remote_nodes.json. Target
    # it with --node catchchallenger-esp32 (or --node xtensa-lx6); skipped when a
    # --node filter excludes it.
    if bh.node_allowed(ESP32_NODE["label"], ESP32_NODE["arch"]):
        nodes.append(ESP32_NODE)

    # The measuring bot is needed by every map-level (non-network_only) node AND
    # by the ESP32 node, which now ATTEMPTS map-level first (pre-seeded datapack
    # -> bot --latency) before falling back to the pure-Python connect probe.
    # Build the bot when any node will try the bot at all: any non-network_only
    # node, or an ESP32 (network_only) node in this batch.
    has_maplevel = any(not n.get("network_only") for n in nodes)
    has_esp32    = any(n.get("network_only") for n in nodes)
    bot_bin = build_bot_local() if (has_maplevel or has_esp32) else None

    # progress total: one "latency" cell per node.
    progress = bh.Progress(len(nodes), BENCH)

    batch_id = hr.new_batch_id()
    started_utc = hr.iso_now()
    # Pre-workload throttle-counter baseline for the local host so the
    # post-run sensor read attributes throttling to THIS run (delta), not
    # since-boot history. Remote nodes use the single post-run read.
    sensor_pre = hr.sensor_baseline(hr.local_runner)
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release", "-DCATCHCHALLENGER_BENCHMARK"]

    per_subbench = {}   # label -> {"latency": {slice_label: metrics}}
    per_headline = {}   # label -> {metric: {median,stddev,unit,better}}
    per_netfamily = {}  # label -> "ipv4" | "ipv6" | "ipv4+ipv6" actually run
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
            # IPv4 vs IPv6: the server binds dual-stack (server-ip="" ->
            # AF_UNSPEC), so on the loopback we can drive the SAME workload
            # over 127.0.0.1 AND ::1 and isolate the pure transport cost
            # (same machine, same server, only the address family differs).
            # Families are detected at runtime; IPv4 stays first/canonical so
            # the headline + champion compare against the historical 127.0.0.1
            # numbers, and IPv6 is recorded as additive "ipv6-*" subbenchmark
            # slices. A node with IPv6 disabled simply runs IPv4 only.
            families = bh.detect_loopback_families() or [("ipv4", "127.0.0.1")]
            head_slices = None      # first family -> headline (champion continuity)
            rec_slices  = {}        # family-prefixed -> recorded subbenchmarks
            fam_done    = []
            err = None
            try:
                for fam, addr in families:
                    fslices, ferr = run_all_slices(bot_bin, addr, SERVER_PORT,
                                                   BUDGET_S_LOCAL,
                                                   server_pid=proc.pid)
                    if ferr is not None:
                        # First family failing = hard skip; a LATER family
                        # failing still keeps what already measured.
                        if head_slices is None:
                            err = ferr
                            break
                        print(_c(bh.C_YELLOW,
                                 f"    [ipv] {fam} slice failed, kept earlier: {ferr}"))
                        continue
                    if head_slices is None:
                        head_slices = fslices
                    for lbl, m in fslices.items():
                        rec_slices[f"{fam}-{lbl}"] = m
                    fam_done.append(fam)
            finally:
                stop_local_server(proc)
            if err is not None or head_slices is None:
                progress.emit("latency", "no", "local", status="SKIP",
                              extra=(err or "no-slices")[:80])
                bh.print_node_error(BENCH, "local", "SKIP", err or "no slices")
            else:
                srv_text = read_server_bench_text(local_run_dir=SRV_RUN_DIR)
                smetrics = server_metrics_from_text(srv_text)
                if smetrics:
                    for lbl in head_slices:
                        head_slices[lbl].update(smetrics)
                    for lbl in rec_slices:
                        rec_slices[lbl].update(smetrics)
                per_subbench["local"] = {"latency": rec_slices}
                per_headline["local"] = headline_metrics(head_slices)
                per_netfamily["local"] = "+".join(fam_done)
                progress.emit("latency", "no", "local", status="PASS")

    # ---- REMOTE nodes: PARALLEL bring-up, SERIAL measurement -------------
    # network_only nodes (ESP32) skip the entire ssh/build path; handled below.
    remote_nodes = [n for n in nodes
                    if n["label"] != "local" and not n.get("network_only")]
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

    # ---- ESP32 (network-only): no bring-up, just measure the running board -
    for node in [n for n in nodes if n.get("network_only")]:
        label = node["label"]
        if _over_cap():
            progress.emit("latency", "no", label, status="SKIP", extra="batch-maxtime")
            continue
        st, reason = measure_esp32(node, bot_bin, per_subbench, per_headline,
                                   per_netfamily)
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
        if node.get("network_only"):
            continue   # WiFi comparison-only: in history + charts, NOT the
                       # cross-fleet KEEP/DISCARD decision (must not be sole signal)
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
        if node.get("network_only"):
            # No shell on the board -> collect() can't run; synthesize fields.
            pr = hr.PlatformRecord(BENCH, batch_id, label, runner=None,
                                   arch_hint=node.get("arch"))
            pr.platform.update(esp32_platform_fields())
        elif label == "local":
            pr = hr.PlatformRecord(BENCH, batch_id, label, runner=hr.local_runner,
                                   arch_hint=node.get("arch")).collect(
                                       cc_binary_path=SRV_RUN_DIR, datapack_path=DATAPACK_PATH,
                                       sensor_baseline=sensor_pre)
        else:
            runner = hr.make_ssh_runner(node.get("ssh_host"), node.get("ssh_user"),
                                        node.get("ssh_port", 22))
            wd = node.get("work_dir") or f"/tmp/cc-bench-{BENCH}"
            pr = hr.PlatformRecord(BENCH, batch_id, label, runner=runner,
                                   arch_hint=node.get("arch")).collect(
                                       cc_binary_path=wd,
                                       datapack_path=os.path.join(wd, "datapack"),
                                       sensor_baseline=None)
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
                         net_family=per_netfamily.get(label),
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
