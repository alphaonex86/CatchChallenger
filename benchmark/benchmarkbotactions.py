#!/usr/bin/env python3
"""benchmarkbotactions.py -- end-to-end network/kernel-stack benchmark.

Workload: build tools/bot-bench (the Qt-FREE benchmark client) AND
catchchallenger-server-cli, run the server with the DB pinned in RAM
(CATCHCHALLENGER_DB_INTERNAL_VARS, see general/DEFINES.md), then saturate it
with bot-bench --spam. Unlike benchmarkserversave (cold-start parse) or
benchmarkmapmanager (in-process visibility loop), this exercises the FULL stack
inside ONE machine:

  * userspace socket + protocol encode/decode
  * kernel send/recv path through the loopback interface
  * server epoll loop + per-tick scheduling
  * server-side packet parse + state updates

CLIENT AND SERVER ARE ALWAYS ON THE SAME HARDWARE. On the host row both run
here; on every remote row the client is built for that arch on the exec node's
compile parent, pushed to the node and run THERE against 127.0.0.1. The old
shape -- server on the node, Qt bot-actions on the orchestrating host over the
LAN -- recorded the HOST process's rusage under the NODE's label and could not
produce a server CPU% at all, so the per-node history was measuring the wrong
machine. It also excluded 11 of the 15 benchmark nodes from ever running a
client: they are headless boards with no Qt6 runtime.

CATCHCHALLENGER_BENCHMARK is mandatory on every server build here: the
anti-flood filter is `#ifndef CATCHCHALLENGER_BENCHMARK`, so a saturating
client would otherwise be kicked after ~60 moves.

# HEADLESS: yes  (Qt-free client: POSIX sockets + select(), no display at all)
Metrics, per workload slice (b<N>_ prefix in the flat keys):
  * req_per_s           -- moves the fleet pushed per second of the measured
                            window: THE work metric (higher better). Every
                            resource number below is only readable against it.
  * moves / survivors /
    bots_on_map         -- window totals + how many bots the node seated
                            (higher better)
  * cpu_percent         -- the SERVER's CPU over the slice. Single-threaded
                            epoll, so bounded at 100 = one core saturated
                            (lower better at equal req_per_s)
  * client_cpu_percent  -- the node-local client's own CPU; on a single-core
                            board this is what it takes away from the server
  * wall_s              -- window + onboarding (lower better)
  * max_rss_kb          -- client peak resident set (lower better)
  * user_s / sys_s      -- client CPU split; sys_s grows with kernel/syscall
                            cost (lower better)
  * vol_ctx / invol_ctx -- context switches (lower better)
  * net_rx_bytes / net_tx_bytes / net_rx_pkts / net_tx_pkts
                        -- /proc/net/dev delta, HOST ROW ONLY (a node's
                            loopback totals are a different quantity, so they
                            are left null rather than silently redefined)
  * tcp_retrans         -- /proc/net/snmp Tcp:RetransSegs delta, host row only
  * binary_size_bytes   -- stripped client footprint (lower better)
Server-side, once per node run (cumulative over its slices, from the server's
BENCH dump on SIGINT):
  * bench_packets_in    -- parser dispatches the server served (higher better).
                            NOT a syscall count: io_uring's recv_multishot
                            harvests many completions per io_uring_enter.
  * latency_p50/p95/p99/p999_ns + latency_jitter_ns -- per-read-event
                            processing tail (lower better). req/s is never
                            reported without it: a throughput win that inflates
                            p99 is a tick-stability regression on the
                            constrained targets.
  * loop_busy_pct / loop_us_per_wakeup -- event-loop self-probe. NOT
                            comparable across event-loop backends (see
                            benchmark/CLAUDE.md), only across commits on one.

Determinism note: TCP RTT inside loopback is ~microseconds, but epoll callback
latency and scheduling still introduce jitter. RUN_REPEATS produces several
samples; the harness records median+stddev so the comparator can apply the
noise band.

One-command target: run with no args, 1h timeout. The server runs in
CATCHCHALLENGER_DB_INTERNAL_VARS mode so the entire database tree lives under
/dev/shm/cc-server-<pid>/ (tmpfs, RAM-backed) for the lifetime of the process
and is removed on exit by an atexit() hook. Reconnects within the same run see
their state; no disk writes ever happen.

The client self-times its window (fixed-time, not fixed-iteration) and exits on
its own; `timeout` is only a hard backstop for a hang. The workload knob is
BOT_COUNTS x DURATION_S -- enough offered load that the server, not the client,
is the bottleneck.
"""
import os
import re
import shlex
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
import benchmark_remote as br

REPO_ROOT  = bh.REPO_ROOT
BENCH_DIR  = os.path.dirname(os.path.abspath(__file__))

# Concurrency marker (read by main() + any orchestrator). True = this
# benchmark binds a fixed server port and drives the NIC/loopback, so it
# must run SERIALLY against every other network benchmark (it waits on the
# shared cc-bench-network.lock). False = pure in-process / no network, safe
# to run in parallel with anything.
NETWORK_EXCLUSIVE = True

BOT_SRC_DIR    = os.path.join(REPO_ROOT, "tools", "bot-bench")
BOT_BIN_NAME   = "bot-bench"
# One account per bot: bot-bench appends the bot index to the login, so the
# per-account character cap can never limit the fleet and no two bots race to
# create the same account.
BOT_LOGIN      = "bench"
BOT_PASS       = "bench"
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
BOT_BUILD_DIR  = "/mnt/data/perso/tmpfs/benchmark/botactions-botbench"
SRV_BUILD_DIR  = os.path.join(BUILD_ROOT, "benchmark", "benchmarkbotactions-server")
SRV_RUN_DIR    = os.path.join(TMPFS_ROOT, "cc-bench-botactions-server")

# Workload knobs. Three sizes per benchmark/CLAUDE.md "Workload variety":
#   small  -- baseline syscall floor, only a couple of bots
#   medium -- typical per-tick offered load
#   large  -- saturate the NIC tx queue / kernel sk_buff pool
BOT_COUNTS    = [2, 8, 30, 50, 150]
DURATION_S    = 15
RUN_REPEATS   = 3        # 1 warmup + 2 measured per (size)
BUILD_TIMEOUT = 1800
# Onboarding allowance handed to the client (--timeout): 150 bots log in in
# waves, and each wave costs a full login round trip on a 66MHz-class board.
# It is NOT part of the measured window (the client self-times that), only the
# budget before the window may open.
BOT_ONBOARD_TIMEOUT_S = 120
# Hard backstop for one client run = onboarding + window + margin. It always
# exceeds the real run, so it only fires on a hang.
RUN_TIMEOUT   = BOT_ONBOARD_TIMEOUT_S + DURATION_S + 45

# Server boot deadline. Datapack parse + bind has to finish before the
# bots start; this matches the value the test/ harness uses for an
# amd64 host (slow nodes need more).
SERVER_READY_TIMEOUT = 60
# Cold boot (no cache -> full XML parse) on a constrained MIPS/ARM box can
# take minutes; give the boot-time benchmark a generous ceiling so the
# no-cache slice is actually captured rather than recorded as a timeout.
SERVER_BOOT_TIMEOUT  = 300
SERVER_PORT          = 61920

# DB defines shared by EVERY botactions server build (local, remote, and
# the io_uring variants). DB_FILE_RAM is ALWAYS on so the whole DB tree
# lives in /dev/shm (RAM) and NEVER touches the exec node's microSD/eMMC --
# the embedded boards (rpi-zero-w, rtl9607c, ...) would otherwise burn flash
# write cycles every run. DB_FILE is its MANDATORY base (DB_FILE_RAM #errors
# without it, server/base/ServerStructures.hpp), so it stays on but the
# RAM variant redirects all writes off disk. CACHE_HPS lets the
# pre-generated datapack cache load. Single source so no build path can
# drift back to disk-backed DB_FILE.
# CATCHCHALLENGER_BENCHMARK is MANDATORY here, not a nicety: the anti-flood
# filter is derived from `#ifndef CATCHCHALLENGER_BENCHMARK`
# (server/base/VariableServer.hpp), so a saturating client is kicked after ~60
# moves against a normal build and the cell measures the kicker instead of the
# stack. It also compiles in the per-read-event counter + latency histogram
# (BENCH lines on SIGINT) that give every cell its work-done metric. The whole
# block is #ifdef'd out of production builds.
SERVER_RAM_DB_DEFS = {
    "CATCHCHALLENGER_DB_FILE":     "ON",
    "CATCHCHALLENGER_CACHE_HPS":   "ON",
    "CATCHCHALLENGER_DB_INTERNAL_VARS": "ON",
    "CATCHCHALLENGER_BENCHMARK":   "ON",
}

# io_uring tuning variants compared on the LOCAL host (the controlled
# bench box; SQPOLL behaviour is kernel/CPU specific and only
# reproducible there). "with and without" each sub-option => baseline
# io_uring vs the opt-in tuning flags, all on top of the RAM-DB defines.
# (IOPOLL was dropped: IORING_SETUP_IOPOLL only completes pollable
# O_DIRECT block I/O SUBMITTED TO THE RING. The server does have file I/O
# — datapack load at boot, FILE_DB database saves — but it is plain
# blocking fopen/fread/write, NOT on the ring; the ring carries only
# sockets (+splice/sendfile for the datapack-to-client send). So IOPOLL
# has nothing pollable to complete and just busy-spins a core at 100%
# serving ~0 bytes. It would only become meaningful if those file paths
# were moved onto the ring with O_DIRECT — not the case today.)
# liburing-dev required; a variant whose build fails (no liburing) is
# skipped, not failed.
# CATCHCHALLENGER_BENCHMARK enables the server-side throughput counter
# (a "BENCH packets_in=<N>" line dumped on SIGINT) so each variant gets a
# work-done metric (requests/s) to contrast against its CPU%/RSS, not a
# bare resource number. The define is benchmark-only — compiled out of
# every production build (server/cli/CMakeLists.txt).
IO_URING_BASE_DEFS = dict(SERVER_RAM_DB_DEFS, CATCHCHALLENGER_IO_URING="ON",
                          CATCHCHALLENGER_BENCHMARK="ON")
IO_URING_VARIANTS = [
    ("iouring",              {}),
    ("iouring-sqpoll",       {"CATCHCHALLENGER_IO_URING_SQPOLL": "ON"}),
    ("iouring-coop-taskrun", {"CATCHCHALLENGER_IO_URING_COOP_TASKRUN": "ON"}),
    # TASKRUN_FLAG has no effect without COOP_TASKRUN, so enable both.
    ("iouring-taskrun-flag", {"CATCHCHALLENGER_IO_URING_COOP_TASKRUN": "ON",
                              "CATCHCHALLENGER_IO_URING_TASKRUN_FLAG": "ON"}),
    ("iouring-no-sqarray",   {"CATCHCHALLENGER_IO_URING_NO_SQARRAY": "ON"}),
]

# Visibility-broadcast strategies swept for EACH io_uring variant. The mode
# is a RUNTIME setting (server-properties.xml mapVisibility/minimize), so no
# rebuild is needed — each io_uring binary is run once per mode. The two
# strategies live in server/base/MapManagement/MapVisibilityAlgorithm.cpp:
#   "balanced" -> min_balanced(): per-recipient diff, fewer bytes, more CPU.
#                 It was named "network" before min_range() took that name;
#                 this sweep stays on min_balanced() so its history keeps
#                 comparing the SAME algorithm.
#   "cpu"     -> min_CPU(): compose the 0x65+0x6B player block ONCE and fan
#                the SAME datablock out to every socket on the map, more
#                bytes but cache-hot + far less CPU.
# Crossing this with the io_uring tuning options answers "does any io_uring
# flag shift the CPU-vs-bytes tradeoff between the two strategies?" — the
# hypothesis being that min_CPU's shared datablock stays in cache and wins
# on server CPU%/latency, paying for it in net_tx_bytes.
IO_URING_VISIBILITY_MODES = ["balanced", "cpu"]
# Bot count for the io_uring x visibility sweep. Deliberately high (vs the
# 'medium' used by the bare network cells): the shared-datablock cache
# effect only shows when many players share a single map, so maximise
# per-map density. Wall time is fixed at DURATION_S regardless of count, so
# a high count costs no extra wall time — only more offered load. Capped
# below mapVisibility Max=254 so the broadcast runs instead of early-outing.
IO_URING_VIS_BOTS = 200

# Compression sweep (LOCAL): protocol Zstandard at level 6 vs OFF, set at
# RUNTIME via server-properties.xml `compression` (one BENCHMARK-enabled
# build, re-run per variant — no rebuild). Only the character-load packet
# is compressed (ClientHeavyLoadSelectCharFinal.cpp), so the headline is
# the CPU vs bytes-on-wire tradeoff: "none" should cut server CPU but
# raise tx bytes. The bytes-on-wire metric (net_tx_bytes on lo) is what
# makes that tradeoff legible — without it "none uses less CPU" is the
# meaningless conclusion CLAUDE.md warns against. Swept across all three
# BOT_COUNTS workload sizes because the threshold/CPU tradeoff differs by
# load. (value, level) — level ignored for "none".
COMPRESSION_VARIANTS = [
    ("zstd6", "zstd", 6),
    ("none",  "none", 6),
]
COMPRESSION_BASE_DEFS = dict(SERVER_RAM_DB_DEFS, CATCHCHALLENGER_BENCHMARK="ON")


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
    cfg += bh.cmake_accel_defs()
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=600)
    if rc != 0:
        bh.print_local_build_error(label, "cmake configure", sout, serr)
        return rc, "cmake configure FAILED"
    # Record which libs (system vs vendored) the local build linked, for
    # the "local" node's history record (no-op on a non-CC configure log).
    bh.record_libs("local", sout)
    bld = ["cmake", "--build", build_dir, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=BUILD_TIMEOUT)
    if rc != 0:
        bh.print_local_build_error(label, "cmake build", sout, serr)
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
    bh.chatter(_color(bh.C_GREEN, f"[build:bot] OK ({bh.binary_size(bin_path)} bytes)"))
    return bin_path


def build_server():
    """Build catchchallenger-server-cli with CATCHCHALLENGER_DB_INTERNAL_VARS
    so the bench server runs with the DB pinned in RAM
    (/dev/shm/cc-server-<pid>/database/...). Reads + writes stay
    coherent so a bot can disconnect and reconnect within the same
    server process and find their account/character intact; nothing
    touches the disk. The HPS cache + FileDB defines are mandatory
    pre-conditions of DB_FILE_RAM (CMake fails fast otherwise)."""
    if not os.path.isdir(DATAPACK_PATH):
        print(_color(bh.C_RED, f"[build:server] datapack not found: {DATAPACK_PATH}"))
        return None
    rc, msg = _cmake_build(SRV_SRC_DIR, SRV_BUILD_DIR,
                           extra_defs=dict(SERVER_RAM_DB_DEFS),
                           label="build:server")
    if rc != 0:
        print(_color(bh.C_RED, f"[build:server] {msg}"))
        return None
    bin_path = os.path.join(SRV_BUILD_DIR, SRV_BIN_NAME)
    if not os.path.isfile(bin_path):
        print(_color(bh.C_RED, f"[build:server] missing binary: {bin_path}"))
        return None
    bh.chatter(_color(bh.C_GREEN, f"[build:server] OK ({bh.binary_size(bin_path)} bytes)"))
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
    # HARD RULE (CLAUDE.md): never point a runtime binary's datapack dir
    # at the SOURCE datapack. COPY rather than symlink so no writer can
    # ever reach the source (testinggateway.stage_gateway_datapack lesson).
    # SRV_RUN_DIR is tmpfs, wiped above; outside the measured window.
    shutil.copytree(DATAPACK_PATH, os.path.join(SRV_RUN_DIR, "datapack"),
                    symlinks=False)
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


def profile_server(staged_bin, bot_bin, tool):
    """--profile path: profile the SERVER (the single-threaded hot path
    this benchmark exists to optimise) under bot load. Launch the server
    wrapped in `tool`, wait until it binds, drive the largest bot count
    against it for DURATION_S, then SIGINT so the profiler dumps. Returns
    0 on success, 2 on failure. perf has ~2x overhead, callgrind ~30x,
    so callgrind/massif need a much longer ready+run budget."""
    full, out, run_env = bh.profiler_wrap([staged_bin], tool)
    if full is None:
        return 2
    slow = 30 if tool == "callgrind" else (20 if tool == "massif" else 2)
    log_path = os.path.join(SRV_RUN_DIR, "server.log")
    try: os.remove(log_path)
    except FileNotFoundError: pass
    log_fh = open(log_path, "wb")
    srv = subprocess.Popen(full, cwd=SRV_RUN_DIR, env=run_env,
                           stdout=log_fh, stderr=subprocess.STDOUT,
                           start_new_session=True)
    deadline = time.monotonic() + SERVER_READY_TIMEOUT * slow
    ready = False
    while time.monotonic() < deadline:
        if srv.poll() is not None:
            break
        try:
            with open(log_path, "rb") as f:
                if b"correctly bind:" in f.read():
                    ready = True
                    break
        except FileNotFoundError:
            pass
        time.sleep(0.5)
    if not ready:
        print(_color(bh.C_RED,
            f"[profile] server failed to bind under {tool}"))
        stop_server(srv)
        return 2
    print(_color(bh.C_GREEN,
        f"[profile] server bound under {tool} (pid={srv.pid}); "
        f"driving {BOT_COUNTS[-1]} bots for {DURATION_S}s"))
    cmd = ["timeout", "--signal=INT", str(RUN_TIMEOUT)] + \
          _bot_cmd(bot_bin, "127.0.0.1", SERVER_PORT, BOT_COUNTS[-1])
    env = os.environ.copy()
    # The client prints one line per bot state transition; under --profile that
    # adds nothing and drowns the harness output — redirect it to a log file in
    # the run dir so the terminal stays quiet (path printed only if it fails).
    bot_log = os.path.join(SRV_RUN_DIR, "bot-bench.log")
    bot_fh = open(bot_log, "wb")
    try:
        subprocess.run(cmd, env=env, timeout=RUN_TIMEOUT * slow,
                       stdout=bot_fh, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        pass
    finally:
        bot_fh.close()
    # SIGINT the server so valgrind/perf flush their artefact, then wait
    # long enough for callgrind to write the (large) out file.
    stop_server(srv)
    log_fh.close()
    if not os.path.isfile(out):
        print(_color(bh.C_RED,
            f"[profile] {tool} produced no artefact (expected {out})"))
        return 2
    bh.profile_artefact_hint(tool, out)
    return 0


def _time_local_boot(staged_bin):
    """Start the local server, time wall-seconds to 'correctly bind:',
    then stop it. Returns boot_s or None (crash / timeout)."""
    run_dir  = os.path.dirname(staged_bin)
    log_path = os.path.join(run_dir, "server.log")
    try: os.remove(log_path)
    except FileNotFoundError: pass
    t0 = time.monotonic()
    log_fh = open(log_path, "wb")
    p = subprocess.Popen([staged_bin], cwd=run_dir, stdout=log_fh,
                         stderr=subprocess.STDOUT, start_new_session=True)
    boot = None
    deadline = t0 + SERVER_BOOT_TIMEOUT
    while time.monotonic() < deadline:
        if p.poll() is not None:
            break
        try:
            with open(log_path, "rb") as f:
                if b"correctly bind:" in f.read():
                    boot = time.monotonic() - t0
                    break
        except FileNotFoundError:
            pass
        # 20ms poll: the local fast-host boot can be <100ms, so a coarser
        # cadence would floor cold and warm at the same value and hide the
        # cache delta. (Remote nodes use a 0.5s cadence -- their boots are
        # seconds, and ssh round-trip dominates anyway.)
        time.sleep(0.02)
    log_fh.close()
    stop_server(p)
    return boot


def _start_server_in(staged_bin, ready_timeout=SERVER_BOOT_TIMEOUT):
    """Start a server whose run dir is the dir holding `staged_bin`; block
    until 'correctly bind:' or it exits/times out. Returns the Popen or
    None. (start_server() is hard-wired to SRV_RUN_DIR; this variant works
    for the per-io_uring-variant run dirs.)"""
    run_dir  = os.path.dirname(staged_bin)
    log_path = os.path.join(run_dir, "server.log")
    try: os.remove(log_path)
    except FileNotFoundError: pass
    log_fh = open(log_path, "wb")
    p = subprocess.Popen([staged_bin], cwd=run_dir, stdout=log_fh,
                         stderr=subprocess.STDOUT, start_new_session=True)
    deadline = time.monotonic() + ready_timeout
    while time.monotonic() < deadline:
        if p.poll() is not None:
            log_fh.close()
            return None
        try:
            with open(log_path, "rb") as f:
                if b"correctly bind:" in f.read():
                    return p
        except FileNotFoundError:
            pass
        time.sleep(0.1)
    stop_server(p)
    return None


def _stage_variant_run_dir(server_bin, run_dir, port,
                           compression=None, compression_level=6,
                           visibility_minimize=None):
    """Stage a run dir (binary copy + datapack symlink + server-properties
    on `port`) for an io_uring / compression variant server.

    compression: None -> omit the key (server default = zstd). "zstd" or
    "none" -> write the key so the compression sweep can toggle protocol
    compression at runtime. main-unix2.cpp reads it and loadAndFixSettings()
    applies it to CompressionProtocol::compressionTypeServer AFTER the
    BaseServer2 ctor default, so the setting actually sticks.

    visibility_minimize: None -> omit the <mapVisibility> group entirely
    (server default = disabled, so the MapVisibilityAlgorithm broadcast
    never fires). "cpu"/"balanced"/"network" -> enable the per-tick broadcast
    and pick the strategy at RUNTIME (main-unix2.cpp mapVisibility/minimize):
    "balanced" routes through MapVisibilityAlgorithm::min_balanced() (per-
    recipient diff), "cpu" through min_CPU() (one shared datablock fanned
    out to every socket). This is the axis the io_uring sweep crosses so
    the cost of each broadcast strategy is visible under each io_uring
    config. Max=254 is the algorithm's hard per-map cap (above it the
    ">=max -> stop update" early-out fires and nothing is sent); Reshow=3
    is the engine minimum (BaseServerSettings clamps anything lower)."""
    if os.path.isdir(run_dir):
        shutil.rmtree(run_dir, ignore_errors=True)
    os.makedirs(run_dir, exist_ok=True)
    dst = os.path.join(run_dir, SRV_BIN_NAME)
    shutil.copy2(server_bin, dst)
    os.chmod(dst, 0o755)
    # HARD RULE (CLAUDE.md): never point a runtime binary's datapack dir
    # at the SOURCE datapack. COPY rather than symlink so no writer can
    # ever reach the source (testinggateway.stage_gateway_datapack lesson).
    # run_dir is tmpfs, wiped above; outside the measured window.
    shutil.copytree(DATAPACK_PATH, os.path.join(run_dir, "datapack"),
                    symlinks=False)
    maincode = _detect_maincode(DATAPACK_PATH)
    compression_xml = ""
    if compression is not None:
        compression_xml = (
            f'    <compression value="{compression}"/>\n'
            f'    <compressionLevel value="{compression_level}"/>\n')
    visibility_xml = ""
    if visibility_minimize is not None:
        visibility_xml = (
            '    <mapVisibility>\n'
            '        <enable value="true"/>\n'
            '        <Max value="254"/>\n'
            '        <Reshow value="3"/>\n'
            f'        <minimize value="{visibility_minimize}"/>\n'
            '    </mapVisibility>\n')
    with open(os.path.join(run_dir, "server-properties.xml"), "w") as f:
        f.write(
            '<?xml version="1.0"?>\n<configuration>\n'
            f'    <server-port value="{port}"/>\n'
            '    <automatic_account_creation value="true"/>\n'
            '    <max-players value="500"/>\n'
            '    <httpDatapackMirror value=""/>\n'
            f'{compression_xml}'
            f'{visibility_xml}'
            f'    <master>\n        <external-server-port value="{port}"/>\n    </master>\n'
            f'    <content>\n        <mainDatapackCode value="{maincode}"/>\n'
            '        <subDatapackCode value=""/>\n    </content>\n</configuration>\n')
    return dst


def _bench_packets_in_from_text(text):
    """'BENCH packets_in=<N>' (the LAST one: the counter is cumulative and a
    node may dump more than once). None when absent -- server built without
    CATCHCHALLENGER_BENCHMARK, or killed before it could dump.

    Note it counts parser DISPATCHES, not syscalls: under io_uring's
    recv_multishot many completions are harvested per io_uring_enter."""
    found = re.findall(r"BENCH packets_in=(\d+)", text)
    if not found:
        return None
    return int(found[-1])


def _read_bench_packets_in(run_dir):
    """_bench_packets_in_from_text() for a locally-run server's log."""
    try:
        with open(os.path.join(run_dir, "server.log"), "r",
                  errors="replace") as f:
            return _bench_packets_in_from_text(f.read())
    except FileNotFoundError:
        return None


def _bench_latency_from_text(text):
    """Reconstruct the server's per-read-event processing-latency distribution
    from its 'BENCH lat_hist_<i>=<count>' log2-ns buckets. Returns
    latency_p50/p95/p99/p999_ns + latency_jitter_ns, or {} when nothing was
    dumped. The bucket math is the shared one (bh.bench_hist_percentiles)."""
    buckets = {}
    for m in re.finditer(r"BENCH lat_hist_(\d+)=(\d+)", text):
        buckets[int(m.group(1))] = int(m.group(2))
    stats = bh.bench_hist_percentiles(buckets)
    if not stats:
        return {}
    out = {"latency_jitter_ns": stats["jitter_ns"]}
    for key in ("p50_ns", "p95_ns", "p99_ns", "p999_ns"):
        out["latency_" + key] = stats[key]
    return out


def _read_bench_latency(run_dir):
    """_bench_latency_from_text() for a locally-run server's log."""
    try:
        with open(os.path.join(run_dir, "server.log"), "r",
                  errors="replace") as f:
            return _bench_latency_from_text(f.read())
    except FileNotFoundError:
        return {}


def _run_iouring_variants(bot_bin, iface):
    """LOCAL io_uring x visibility-mode sweep. For every io_uring tuning
    variant (baseline / +SQPOLL / +COOP_TASKRUN / +TASKRUN_FLAG /
    +NO_SQARRAY) the server is built ONCE, then run twice — once with the
    MapVisibilityAlgorithm broadcast in min_balanced mode and once in min_CPU
    mode (mapVisibility/minimize toggled at RUNTIME, no rebuild). Per
    (variant, mode) it records wall/sys/user/RSS + server CPU% + requests/s
    + latency tail + bytes-on-wire so the CPU-vs-bytes tradeoff of the two
    broadcast strategies is legible under each io_uring config. Returns
    (flat {name:(med,std)}, slices {"<variant>-<mode>": metric_block}). A
    variant whose build fails (no liburing) or whose server won't bind is
    skipped with a note -- never a hard failure."""
    flat, slices = {}, {}
    bots = IO_URING_VIS_BOTS
    port_seq = 0
    for i, (vlabel, extra) in enumerate(IO_URING_VARIANTS):
        defs = dict(IO_URING_BASE_DEFS)
        defs.update(extra)
        bdir = os.path.join(BUILD_ROOT, "benchmark", f"botactions-{vlabel}")
        rc, msg = _cmake_build(SRV_SRC_DIR, bdir, extra_defs=defs,
                               label=f"build:{vlabel}")
        if rc != 0:
            print(_color(bh.C_YELLOW, f"[iouring] {vlabel}: build failed "
                  f"({msg}); skipping (liburing-dev missing?)"))
            continue
        sbin = os.path.join(bdir, SRV_BIN_NAME)
        if not os.path.isfile(sbin):
            print(_color(bh.C_YELLOW, f"[iouring] {vlabel}: binary missing; skip"))
            continue
        # Same binary, two runtime visibility strategies. min_balanced vs
        # min_CPU is the axis this sweep exists to compare.
        for mode in IO_URING_VISIBILITY_MODES:
            slabel = f"{vlabel}-{mode}"
            port = SERVER_PORT + 10 + port_seq
            port_seq += 1
            rdir = os.path.join(TMPFS_ROOT, f"cc-bench-botactions-{slabel}")
            # Re-check per variant: the build tree lives on tmpfs, which another
            # run's cleanup can wipe mid-sweep. The one check before the loop is
            # then stale and the copy below dies with FileNotFoundError, killing
            # the whole benchmark - this function's contract is to SKIP instead.
            if not os.path.isfile(sbin):
                print(_color(bh.C_YELLOW, f"[io_uring] {slabel}: server binary "
                             f"vanished ({sbin}); stop the sweep"))
                return flat, slices
            staged = _stage_variant_run_dir(sbin, rdir, port,
                                            visibility_minimize=mode)
            # pre-generate the cache so the variant server boots fast
            bh.run_capture([staged, "save"], timeout=RUN_TIMEOUT, cwd=rdir,
                           preexec_fn=bh._drop_core_rlimit)
            proc = _start_server_in(staged)
            if proc is None:
                print(_color(bh.C_YELLOW, f"[iouring] {slabel}: server not ready; skip"))
                continue
            try:
                sample, sample_err = _run_once(
                    bot_bin, "127.0.0.1", port, bots, iface,
                    server_pid=proc.pid,
                    datapack=os.path.join(rdir, "datapack"))
            finally:
                stop_server(proc)
            if sample is None:
                print(_color(bh.C_YELLOW,
                      f"[iouring] {slabel}: bot run failed; skip ({sample_err})"))
                continue
            sm = {}
            for k in ("wall_s", "sys_s", "user_s", "max_rss_kb"):
                v = sample.get(k)
                if v is not None:
                    flat[f"{slabel}_{k}"] = (v, 0.0)
                    sm[k] = {"value": v, "median": v, "stddev": 0.0,
                             "unit": "kb" if k.endswith("_kb") else "s",
                             "better": "lower", "samples": [v]}
            # bytes-on-wire (loopback rx/tx): THE metric that distinguishes
            # the two broadcast strategies. min_CPU rebroadcasts every
            # player every tick (high tx); min_balanced sends only the diff
            # (low tx). Without it "min_CPU uses less CPU" is the meaningless
            # half-conclusion benchmark/CLAUDE.md warns against — the CPU win
            # only counts if paired with its bandwidth cost.
            for k in ("net_tx_bytes", "net_rx_bytes"):
                v = sample.get(k)
                if v is not None:
                    flat[f"{slabel}_{k}"] = (v, 0.0)
                    sm[k] = {"value": v, "median": v, "stddev": 0.0,
                             "unit": "bytes", "better": "lower", "samples": [v]}
            cpu = sample.get("cpu_percent")
            if cpu is not None:
                flat[f"{slabel}_cpu_percent"] = (cpu, 0.0)
                sm["cpu_percent"] = {"value": cpu, "median": cpu, "stddev": 0.0,
                                     "unit": "%", "better": "lower", "samples": [cpu]}
            # Work-done metric: server read-events processed (BENCH counter)
            # divided by the run's wall time => requests/s. This is the number
            # CPU%/RSS must be contrasted against — less CPU at a LOWER
            # requests/s is a regression, not a win (benchmark/CLAUDE.md:
            # "CPU%/RSS are never a conclusion on their own").
            rps = None
            packets = _read_bench_packets_in(rdir)
            wall = sample.get("wall_s")
            if packets is not None and wall and wall > 0:
                rps = packets / wall
                flat[f"{slabel}_requests_per_s"] = (rps, 0.0)
                sm["requests_per_s"] = {"value": rps, "median": rps,
                                        "stddev": 0.0, "unit": "req/s",
                                        "better": "higher", "samples": [rps]}
            # Latency tail + jitter from the server's processing-time histogram.
            # The throughput headline (req/s) MUST ride with its latency/jitter:
            # a req/s win that inflates p99 or jitter is a tick-stability
            # regression on the constrained targets (benchmark/CLAUDE.md
            # "Latency vs throughput").
            lat = _read_bench_latency(rdir)
            for k, v in lat.items():
                flat[f"{slabel}_{k}"] = (v, 0.0)
                sm[k] = {"value": v, "median": v, "stddev": 0.0, "unit": "ns",
                         "better": "lower", "samples": [v]}
            # event-loop self-probe: busy% + us/wakeup (time-based, cross-arch)
            for k, v in _read_loop_selfprobe(rdir).items():
                flat[f"{slabel}_{k}"] = (v["value"], 0.0)
                sm[k] = {"value": v["value"], "median": v["value"],
                         "stddev": 0.0, "unit": v["unit"],
                         "better": v["better"], "samples": [v["value"]]}
            if sm:
                slices[slabel] = sm
            rps_s = f"{rps:.0f}req/s" if rps is not None else "req/s=?"
            tx = sample.get("net_tx_bytes")
            tx_s = f" tx={tx}B" if tx is not None else ""
            p99 = lat.get("latency_p99_ns")
            jit = lat.get("latency_jitter_ns")
            lat_s = (f" p99={p99:.0f}ns jitter={jit:.0f}ns"
                     if p99 is not None else "")
            print(_color(bh.C_GREEN, f"[iouring] {slabel}: wall={sample.get('wall_s')}s "
                  f"sys={sample.get('sys_s')}s cpu={cpu}% {rps_s}{tx_s}{lat_s}"))
    return flat, slices


def _read_loop_selfprobe(run_dir):
    """Event-loop self-probe metrics (bh.parse_loop_selfprobe: busy% of the
    serve window + us per wakeup, TIME-based so cross-arch comparable) from
    the server.log BENCH loop_* dump. {} when absent (server built without
    CATCHCHALLENGER_BENCHMARK, or killed before it could dump)."""
    log_path = os.path.join(run_dir, "server.log")
    try:
        with open(log_path, "r", errors="replace") as f:
            return bh.parse_loop_selfprobe(f.read())
    except FileNotFoundError:
        return {}


_SIZE_LABELS = ["small", "medium", "large"]


def _size_label(i):
    """Map a BOT_COUNTS index to small/medium/large; fall back to the
    raw count when there are more than three sizes."""
    if i < len(_SIZE_LABELS) and len(BOT_COUNTS) <= len(_SIZE_LABELS):
        return _SIZE_LABELS[i]
    return f"{BOT_COUNTS[i]}-bots"


def _run_compression_variants(bot_bin, iface):
    """LOCAL compression sweep: zstd-6 vs none, set at RUNTIME (one
    BENCHMARK-enabled build, re-staged per variant). Sweeps every
    BOT_COUNTS workload size. Per (variant, size) records the work metric
    (requests/s) + its resource cost (server cpu_percent) + the latency
    tail (p50/p95/p99) + bytes-on-wire (net_tx_bytes) so the CPU/bandwidth
    tradeoff is legible. Returns (flat {name:(med,std)}, slices
    {"<variant>-<size>": metric_block}). A build/boot failure is skipped
    with a note, never a hard failure (mirrors _run_iouring_variants)."""
    flat, slices = {}, {}
    # One benchmark-enabled build, default compression; runtime config
    # selects zstd/none per run.
    bdir = os.path.join(BUILD_ROOT, "benchmark", "botactions-compression")
    rc, msg = _cmake_build(SRV_SRC_DIR, bdir, extra_defs=dict(COMPRESSION_BASE_DEFS),
                           label="build:compression")
    if rc != 0:
        print(_color(bh.C_YELLOW, f"[compression] build failed ({msg}); skip sweep"))
        return flat, slices
    sbin = os.path.join(bdir, SRV_BIN_NAME)
    if not os.path.isfile(sbin):
        print(_color(bh.C_YELLOW, "[compression] binary missing; skip sweep"))
        return flat, slices
    port_base = SERVER_PORT + 40
    pidx = 0
    for vi, (vlabel, comp, level) in enumerate(COMPRESSION_VARIANTS):
        for si, bots in enumerate(BOT_COUNTS):
            size = _size_label(si)
            slabel = f"{vlabel}-{size}"
            port = port_base + pidx
            pidx += 1
            rdir = os.path.join(TMPFS_ROOT, f"cc-bench-botactions-comp-{vlabel}-{size}")
            # Re-check per variant: see the io_uring sweep - a concurrent tmpfs
            # cleanup can delete the build tree between two variants, and an
            # unguarded copy would abort the whole benchmark instead of skipping.
            if not os.path.isfile(sbin):
                print(_color(bh.C_YELLOW, f"[compression] {slabel}: server binary "
                             f"vanished ({sbin}); stop the sweep"))
                return flat, slices
            staged = _stage_variant_run_dir(sbin, rdir, port,
                                            compression=comp, compression_level=level)
            # warm the HPS cache so boot is fast and the run isn't polluted
            # by cold XML parse (benchmark/CLAUDE.md: profile a WARM boot).
            bh.run_capture([staged, "save"], timeout=RUN_TIMEOUT, cwd=rdir,
                           preexec_fn=bh._drop_core_rlimit)
            proc = _start_server_in(staged)
            if proc is None:
                print(_color(bh.C_YELLOW, f"[compression] {slabel}: server not ready; skip"))
                continue
            try:
                sample, sample_err = _run_once(
                    bot_bin, "127.0.0.1", port, bots, iface,
                    server_pid=proc.pid,
                    datapack=os.path.join(rdir, "datapack"))
            finally:
                stop_server(proc)
            if sample is None:
                print(_color(bh.C_YELLOW,
                      f"[compression] {slabel}: bot run failed; skip ({sample_err})"))
                continue
            sm = {}
            # server CPU% (the resource cost)
            cpu = sample.get("cpu_percent")
            if cpu is not None:
                flat[f"{slabel}_cpu_percent"] = (cpu, 0.0)
                sm["cpu_percent"] = {"value": cpu, "median": cpu, "stddev": 0.0,
                                     "unit": "%", "better": "lower", "samples": [cpu]}
            # bytes-on-wire (server tx): the bandwidth side of the tradeoff.
            tx = sample.get("net_tx_bytes")
            if tx is not None:
                flat[f"{slabel}_net_tx_bytes"] = (tx, 0.0)
                sm["net_tx_bytes"] = {"value": tx, "median": tx, "stddev": 0.0,
                                      "unit": "bytes", "better": "lower",
                                      "samples": [tx]}
            # work metric: requests/s (CPU% is meaningless without it).
            rps = None
            packets = _read_bench_packets_in(rdir)
            wall = sample.get("wall_s")
            if packets is not None and wall and wall > 0:
                rps = packets / wall
                flat[f"{slabel}_requests_per_s"] = (rps, 0.0)
                sm["requests_per_s"] = {"value": rps, "median": rps, "stddev": 0.0,
                                        "unit": "req/s", "better": "higher",
                                        "samples": [rps]}
            # latency tail (p50/p95/p99 ride with throughput).
            lat = _read_bench_latency(rdir)
            for k, v in lat.items():
                flat[f"{slabel}_{k}"] = (v, 0.0)
                sm[k] = {"value": v, "median": v, "stddev": 0.0, "unit": "ns",
                         "better": "lower", "samples": [v]}
            # event-loop self-probe: busy% + us/wakeup (time-based, cross-arch)
            for k, v in _read_loop_selfprobe(rdir).items():
                flat[f"{slabel}_{k}"] = (v["value"], 0.0)
                sm[k] = {"value": v["value"], "median": v["value"],
                         "stddev": 0.0, "unit": v["unit"],
                         "better": v["better"], "samples": [v["value"]]}
            if sm:
                slices[slabel] = sm
            rps_s = f"{rps:.0f}req/s" if rps is not None else "req/s=?"
            tx_s = f" tx={tx}B" if tx is not None else ""
            p99 = lat.get("latency_p99_ns")
            p99_s = f" p99={p99:.0f}ns" if p99 is not None else ""
            print(_color(bh.C_GREEN, f"[compression] {slabel}: cpu={cpu}% "
                  f"{rps_s}{tx_s}{p99_s}"))
    return flat, slices


def _measure_local_boot(staged_bin):
    """Local server boot-time benchmark: time-to-'correctly bind:' cold
    (no datapack cache -> XML parse) and warm (cache present). `save`
    generates the cache AND sets its mtime to match server-properties.xml
    so the warm boot actually loads it. Leaves a valid cache in place so
    the subsequent persistent server boot is fast. Returns
    (boot_flat {name:(median,std)}, boot_slices)."""
    run_dir = os.path.dirname(staged_bin)
    def _rm_cache():
        for n in ("datapack-cache.bin", "datapack-cache.bin.tmp"):
            try: os.remove(os.path.join(run_dir, n))
            except FileNotFoundError: pass
    def _slice(v):
        return {"boot_to_bind_s": {"value": v, "median": v, "stddev": 0.0,
                                   "unit": "s", "better": "lower",
                                   "samples": [v]}}
    boot_flat, boot_slices = {}, {}
    _rm_cache()
    cold = _time_local_boot(staged_bin)
    if cold is not None:
        boot_flat["boot_no_cache_s"] = (cold, 0.0)
        boot_slices["no-cache"] = _slice(cold)
    _rm_cache()
    bh.run_capture([staged_bin, "save"], timeout=RUN_TIMEOUT, cwd=run_dir,
                   preexec_fn=bh._drop_core_rlimit)
    warm = _time_local_boot(staged_bin)
    if warm is not None:
        boot_flat["boot_with_cache_s"] = (warm, 0.0)
        boot_slices["with-cache"] = _slice(warm)
    return boot_flat, boot_slices


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

_CLK_TCK = os.sysconf("SC_CLK_TCK") if hasattr(os, "sysconf") else 100

# Temp dir for per-run client stdout / stderr logs. Named by
# port+bots so concurrent io_uring variant runs don't collide.
_BOT_RUN_TMP = "/tmp/cc-bench-botrun"

# /usr/bin/time -v line prefixes that appear in stderr but belong to the
# timing wrapper, not the bot.  Filtered out when extracting the bot's
# own stderr tail for crash reports.
_TIME_V_PREFIXES = (
    "Command being timed:", "User time", "System time", "Percent of CPU",
    "Elapsed (wall", "Maximum resident", "Average ", "Major (", "Minor (",
    "Voluntary", "Involuntary", "Swaps:", "File system", "Socket ",
    "Page size", "Exit status", "Signals delivered",
    "Average stack", "Average total", "Average resident", "Average shared",
)


def _bot_crash_report(stdout_log, stderr_log, gdb_log):
    """Assemble a crash report string: the last 50 lines of client stdout and
    the last 50 lines of client stderr (with /usr/bin/time -v metric lines
    stripped from stderr). `gdb_log` is kept for a caller that has a backtrace
    from elsewhere; the load path no longer runs the client under gdb (it
    perturbed the very throughput being measured)."""
    parts = []
    if gdb_log and os.path.isfile(gdb_log):
        try:
            bt = open(gdb_log, "r", errors="replace").read().strip()
            if bt:
                parts.append(f"--- GDB BACKTRACE ---\n{bt}")
        except Exception:
            pass
    for label, path in (("BOT STDOUT", stdout_log), ("BOT STDERR", stderr_log)):
        if not path or not os.path.isfile(path):
            continue
        try:
            lines = open(path, "r", errors="replace").readlines()
            if label == "BOT STDERR":
                lines = [ln for ln in lines
                         if not ln.strip().startswith(_TIME_V_PREFIXES)]
            tail = "".join(lines[-50:]).rstrip()
            if tail:
                parts.append(f"--- {label} (last 50 lines) ---\n{tail}")
        except Exception:
            pass
    return "\n".join(parts)

def _server_cpu_ticks(pid):
    """Return (utime + stime) for `pid` in clock ticks, or None if the
    process is gone. Reads /proc/<pid>/stat fields 14 (utime) and 15
    (stime). The comm field at index 2 can contain spaces and parens,
    so we slice from the last ')' before splitting."""
    try:
        with open(f"/proc/{pid}/stat", "rb") as f:
            raw = f.read().decode("ascii", errors="replace")
        rp = raw.rfind(")")
        if rp < 0:
            return None
        rest = raw[rp + 1:].split()
        # rest[0] is field 3 (state); utime is field 14 -> rest[11].
        return int(rest[11]) + int(rest[12])
    except (FileNotFoundError, ProcessLookupError, ValueError, IndexError):
        return None


def _bot_argv(host, port, bots, datapack):
    """bot-bench arguments for one cell.

    --spam is the offered load: once every bot is on the map each one walks
    between two validated tiles as fast as the server drains its socket, so the
    pacing is the server's own back-pressure. That is what saturates a
    single-threaded event loop, and it yields MOVES / REQ_PER_S -- the work-done
    metric without which the CPU% and RSS of a cell mean nothing.

    The window is self-timed (CLOCK_MONOTONIC around the phase only), so login,
    character creation and datapack parse are excluded from it; --timeout is the
    onboarding budget, not the window."""
    return ["--host",  str(host),
            "--port",  str(port),
            "--bots",  str(bots),
            "--login", BOT_LOGIN,
            "--pass",  BOT_PASS,
            "--datapack", datapack,
            "--timeout", str(int(BOT_ONBOARD_TIMEOUT_S * 1000)),
            "--spam", "--seconds", str(int(DURATION_S))]


def _bot_cmd(bin_path, host, port, bots, datapack=None):
    return [bin_path] + _bot_argv(
        host, port, bots,
        datapack or os.path.join(SRV_RUN_DIR, "datapack"))


# bot-bench's fixed output contract (tools/bot-bench/main.cpp).
_SPAM_RE = {
    "moves":     re.compile(r"^MOVES (\d+)$", re.M),
    "req_per_s": re.compile(r"^REQ_PER_S ([0-9.]+)$", re.M),
    "survivors": re.compile(r"^SURVIVORS (\d+)/(\d+)$", re.M),
    "on_map":    re.compile(r"^ON_MAP (\d+)/(\d+)$", re.M),
}


def _parse_spam_output(text):
    """Pull the saturation numbers out of a bot-bench run. Returns
    (metrics_dict, err_or_None) -- err when the run produced no REQ_PER_S line
    at all, which means nothing was measured (a cell with no work metric is a
    FAIL, not a slow result)."""
    out = {}
    m = _SPAM_RE["req_per_s"].search(text)
    if m is None:
        return None, "no REQ_PER_S line from " + BOT_BIN_NAME
    out["req_per_s"] = float(m.group(1))
    m = _SPAM_RE["moves"].search(text)
    if m is not None:
        out["moves"] = int(m.group(1))
    m = _SPAM_RE["survivors"].search(text)
    if m is not None:
        out["survivors"] = int(m.group(1))
    m = _SPAM_RE["on_map"].search(text)
    if m is not None:
        out["bots_on_map"] = int(m.group(1))
    return out, None


def _run_once(bin_path, host, port, bots, iface, server_pid=None,
              datapack=None):
    """Run the client for one HOST-ROW cell (client and server both here) and
    return a sample dict, or None on failure. The client self-times its
    DURATION_S window and exits on its own; `timeout --signal=INT` is only a
    backstop, and SIGINT rather than SIGKILL so the sockets close cleanly -- a
    hard kill leaves TIME_WAIT churn that pollutes the NEXT iteration.

    The remote counterpart is _run_once_on_exec(), which runs the client on the
    exec node beside its server.

    On a crash the exit code is surfaced together with the last 50 lines of
    client stdout and stderr.

    When `server_pid` is given, sample the server's /proc/<pid>/stat
    cpu ticks pre/post and derive its single-thread CPU% over the run.
    The server is intentionally single-threaded (epoll), so the value
    is bounded at 100 % -- a reading above that means a wrong PID
    was timed (clamp + log, don't pretend it's multi-core)."""
    env = os.environ.copy()

    pre_iface = _read_iface_counters(iface) if iface else None
    pre_retrans = _read_tcp_retrans()
    pre_srv_ticks = _server_cpu_ticks(server_pid) if server_pid else None

    # Per-run temp dir: named by port+bots so concurrent variant runs on
    # different ports don't collide; files are overwritten each iteration
    # (cell_run() stops on first failure, so they always belong to that run).
    run_tmp = os.path.join(_BOT_RUN_TMP, f"p{port}-b{bots}")
    os.makedirs(run_tmp, exist_ok=True)
    stdout_log = os.path.join(run_tmp, "bot_stdout.log")
    stderr_log = os.path.join(run_tmp, "bot_stderr.log")
    for stale in (stdout_log, stderr_log):
        try: os.remove(stale)
        except FileNotFoundError: pass

    bot_args = _bot_cmd(bin_path, host, port, bots, datapack=datapack)

    # NOT under gdb. The client used to be wrapped in `gdb --batch` with catch
    # commands for SIGABRT/SIGSEGV/SIGBUS; that was viable while the bot had no
    # end of its own and the metrics came from /usr/bin/time -v (a SIGINT kill
    # with rc=124 was the normal path). It is not viable now, and was never a
    # good idea in a throughput benchmark: measured here, every cell under gdb
    # blew past a 180s backstop while the same window bare takes 5.5s, and the
    # client never got to print its MOVES/REQ_PER_S line. A benchmark must
    # measure the plain binary. A crash still surfaces -- bad exit code +
    # _bot_crash_report() with both output tails.
    inner = bot_args

    # timeout(1) is only a hard backstop: the client self-times its window and
    # exits on its own (rc 0). --signal=INT so the sockets close cleanly if it
    # ever does fire; 124 then means the run hung and produced no work metric.
    # /usr/bin/time -v wraps the chain for the client's resource metrics.
    full = ["/usr/bin/time", "-v",
            "timeout", "--signal=INT", str(RUN_TIMEOUT)] + inner

    t0 = time.monotonic()
    with open(stdout_log, "wb") as out_fh, open(stderr_log, "wb") as err_fh:
        p = subprocess.run(full, env=env, stdout=out_fh, stderr=err_fh,
                           timeout=RUN_TIMEOUT)
    wall = time.monotonic() - t0

    # Read stderr: bot stderr + /usr/bin/time -v report (both land here).
    # errors="replace": player pseudos forwarded via mapVisibility contain
    # raw Latin-1 bytes; lossy-decode is harmless – we only parse ASCII
    # /usr/bin/time -v lines from this stream.
    try:
        stderr_text = open(stderr_log, "r", errors="replace").read()
    except Exception:
        stderr_text = ""

    # Crash detection: a bad exit code. 0 is the normal path (the client
    # self-exits after its window); 124 / 130 mean the backstop fired, which
    # the missing work metric below reports as a failure anyway.
    bad_rc = p.returncode not in (0, 124, 128 + signal.SIGINT)
    if bad_rc:
        crash_info = _bot_crash_report(stdout_log, stderr_log, None)
        msg = f"bot host={host}:{port} bots={bots} rc={p.returncode}"
        if crash_info:
            msg = msg + "\n" + crash_info
        return None, msg

    post_iface = _read_iface_counters(iface) if iface else None
    post_retrans = _read_tcp_retrans()
    post_srv_ticks = _server_cpu_ticks(server_pid) if server_pid else None

    # The work done in the fixed window. Without it the resource numbers below
    # are unreadable: less CPU because the cell pushed fewer moves is a
    # regression, not a win (benchmark/CLAUDE.md).
    try:
        stdout_text = open(stdout_log, "r", errors="replace").read()
    except OSError:
        stdout_text = ""
    spam, spam_err = _parse_spam_output(stdout_text)
    if spam is None:
        return None, (f"bot host={host}:{port} bots={bots}: {spam_err}\n" +
                      _bot_crash_report(stdout_log, stderr_log, None))

    sample = {"wall_s": wall, "user_s": None, "sys_s": None,
              "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
              "minor_pf": None, "major_pf": None}
    sample.update(spam)
    for line in stderr_text.splitlines():
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

    if (pre_srv_ticks is not None and post_srv_ticks is not None
            and wall > 0 and _CLK_TCK > 0):
        srv_cpu_s = (post_srv_ticks - pre_srv_ticks) / float(_CLK_TCK)
        pct = (srv_cpu_s / wall) * 100.0
        if pct > 100.0:
            # Server is single-threaded; >100% means our PID points at
            # the wrong process. Surface the bug instead of pretending.
            print(_color(bh.C_YELLOW,
                f"[warn] server cpu_percent={pct:.1f} >100 — wrong PID? "
                f"(pid={server_pid}, ticks={post_srv_ticks - pre_srv_ticks}, "
                f"wall={wall:.2f}s)"), file=sys.stderr)
            pct = 100.0
        sample["cpu_percent"] = pct

    return sample, None


def _run_once_on_exec(exec_node, bots, server_pid):
    """One pass with the client running ON the exec node, beside the server.

    This is the remote counterpart of _run_once(): same sample keys, but the
    rusage now belongs to the client process on the BOARD (where it competes
    with the server for the CPU) instead of to a process on the orchestrating
    host. The /proc/net counters are deliberately absent -- they would be the
    node's loopback totals, which is a different quantity from the host's LAN
    interface delta, so they are left None rather than silently redefined."""
    argv = _bot_argv("127.0.0.1", SERVER_PORT, bots, "datapack")
    ok, res = br.run_client_on_exec(
        exec_node, " ".join(shlex.quote(a) for a in argv),
        server_pid=server_pid, timeout=RUN_TIMEOUT + 60,
        bin_name=BOT_BIN_NAME)
    if not ok:
        return None, f"node-local client run failed (bots={bots}): {res['error']}"
    if res["client_rc"] not in (0, None):
        return None, (f"{BOT_BIN_NAME} rc={res['client_rc']} on "
                      f"{exec_node['label']} (bots={bots})\n" +
                      "\n".join(res["err"].splitlines()[-25:]))
    spam, spam_err = _parse_spam_output(res["out"])
    if spam is None:
        return None, (f"{exec_node['label']} bots={bots}: {spam_err}\n" +
                      "\n".join(res["err"].splitlines()[-25:]))
    sample = {"wall_s": res["wall_s"], "user_s": None, "sys_s": None,
              "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
              "minor_pf": None, "major_pf": None}
    sample.update(spam)
    for line in res["err"].splitlines():
        s = line.strip()
        if s.startswith("User time (seconds):"):
            sample["user_s"] = _to_float(s.split(":", 1)[1])
        elif s.startswith("System time (seconds):"):
            sample["sys_s"] = _to_float(s.split(":", 1)[1])
        elif s.startswith("Maximum resident set size (kbytes):"):
            sample["max_rss_kb"] = _to_float(s.split(":", 1)[1])
        elif s.startswith("Voluntary context switches:"):
            sample["vol_ctx"] = _to_float(s.split(":", 1)[1])
        elif s.startswith("Involuntary context switches:"):
            sample["invol_ctx"] = _to_float(s.split(":", 1)[1])
        elif s.startswith("Minor (reclaiming a frame) page faults:"):
            sample["minor_pf"] = _to_float(s.split(":", 1)[1])
        elif s.startswith("Major (requiring I/O) page faults:"):
            sample["major_pf"] = _to_float(s.split(":", 1)[1])
    if res["server_cpu_percent"] is not None:
        sample["cpu_percent"] = res["server_cpu_percent"]
    if res["client_cpu_percent"] is not None:
        sample["client_cpu_percent"] = res["client_cpu_percent"]
    return sample, None


def cell_run(bin_path, host, port, bots, iface, server_pid=None,
             exec_node=None, datapack=None):
    """RUN_REPEATS passes (drop first as warmup). Returns
    (per-metric {name -> (median, stddev)}, None) for one (bots) cell, or
    (None, reason) on failure so the caller can surface WHY it failed.

    `exec_node` runs the client on that node next to the server (every remote
    row); None runs it locally against a local server (the host row)."""
    samples = []
    for i in range(RUN_REPEATS):
        if exec_node is not None:
            s, err = _run_once_on_exec(exec_node, bots, server_pid)
        else:
            s, err = _run_once(bin_path, host, port, bots, iface,
                               server_pid=server_pid, datapack=datapack)
        if s is None:
            return None, err
        if i == 0:
            continue
        samples.append(s)
    if not samples:
        return None, "no measured samples after warmup"
    keys = set()
    for s in samples:
        keys.update(k for k, v in s.items() if v is not None)
    out = {}
    for k in keys:
        vals = [s[k] for s in samples if s.get(k) is not None]
        med, std = bh.stats_of(vals)
        if med is not None:
            out[k] = (med, std)
    return out, None


def _to_float(text):
    """Numeric field of a `/usr/bin/time -v` line, or None when it is not a
    number (a node's BusyBox time prints a different layout). Never raises:
    a missing metric is recorded as null, never as 0."""
    try:
        return float(text.strip())
    except ValueError:
        return None


def _unit_for(name):
    if name.endswith("req_per_s"): return "moves/s"
    if name.endswith("_s"):       return "s"
    if name.endswith("_kb"):      return "kb"
    if name.endswith("_bytes"):   return "bytes"
    if name.endswith("_pkts"):    return "count"
    if name == "cpu_percent" or name.endswith("_cpu_percent"): return "%"
    return "count"


# Metrics where MORE is better. Everything else is a cost (time, bytes, CPU,
# faults) and defaults to lower-is-better. Names are matched on the suffix
# because the flat keys are prefixed with the workload slice ("b30_req_per_s").
_HIGHER_IS_BETTER_SUFFIXES = ("req_per_s", "moves", "survivors", "bots_on_map",
                              "bench_packets_in")


def _better_for(name):
    for suffix in _HIGHER_IS_BETTER_SUFFIXES:
        if name == suffix or name.endswith("_" + suffix):
            return "higher"
    return "lower"


def _flat_to_metric_block(flat):
    out = {}
    for name, (med, std) in flat.items():
        out[name] = {"value": med, "median": med, "stddev": std,
                     "unit": _unit_for(name), "better": _better_for(name),
                     "samples": None}
    return out


def _run_remote_cells_botactions(node, avail_profilers, skips, all_profilers,
                                 extra_rusage_count, progress, per_tool,
                                 per_subbench, compile_flags, local_bot_bin):
    """Remote dispatch for benchmarkbotactions on one exec node.

    BOTH binaries are built on the compile node and run on the exec node: the
    server, and the Qt-free client that loads it over 127.0.0.1. Nothing about
    the measurement happens on the orchestrating host, which only holds the ssh
    connections.

    `extra_rusage_count` = max(len(BOT_COUNTS)-1, 0): number of extra progress
    ticks the rusage path emits for additional BOT_COUNT slices so the total
    counter stays in sync."""
    label        = node["label"]
    compile_node = node.get("compile_node")
    if compile_node is None:
        for prof in all_profilers:
            progress.emit(prof, "no", label, status="SKIP", extra="no-compile-node")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        for _ in range(extra_rusage_count):
            progress.emit("rusage", "no", label, status="SKIP", extra="no-compile-node")
        return

    exec_node = {"label": label,
                 "user":  node.get("ssh_user"),
                 "host":  node.get("ssh_host"),
                 "port":  node.get("ssh_port", 22),
                 "work_dir": node.get("work_dir") or "/tmp/cc-bench-botactions",
                 "lxc_nfs": node.get("lxc_nfs")}

    runnable = [p for p in all_profilers if p in avail_profilers]
    for prof in all_profilers:
        if prof not in runnable:
            reason = skips.get(prof, "tool-missing")
            progress.emit(prof, "no", label, status="SKIP", extra=reason)
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
    rusage_skip_reason = skips.get("rusage", "tool-missing")
    for _ in range(extra_rusage_count):
        if "rusage" not in runnable:
            progress.emit("rusage", "no", label, status="SKIP", extra=rusage_skip_reason)
    if not runnable:
        return

    def _finish_all(status, reason):
        # The banner (print_node_error) already showed the full reason; keep
        # the one-line live counter readable with a truncated copy.
        short = reason if len(reason) <= 80 else reason[:77] + "..."
        for prof in runnable:
            if per_tool[label].get(prof) is None:
                progress.emit(prof, "no", label, status=status, extra=short)
                per_tool[label][prof] = {"status": status, "metrics": {}}
        for _ in range(extra_rusage_count):
            progress.emit("rusage", "no", label, status=status, extra=short)

    def _fail_all(reason):
        # Show the FULL cause under a banner + the re-run command; the live
        # progress line only carries a short reason.
        bh.print_node_error("benchmarkbotactions", label, "FAIL", reason)
        _finish_all("FAIL", reason)

    # Infra failures (compile/push/bring-up/datapack/cache) leave NO metric:
    # the node is unmeasured, which the decision matrix treats as unknown --
    # never a regression.  Mark those SKIP, reserving FAIL for a benchmark
    # that actually ran and produced a bad/garbled result.
    def _skip_all(reason):
        bh.print_node_error("benchmarkbotactions", label, "SKIP", reason)
        _finish_all("SKIP", reason)

    # NFS-LXC bring-up (no-op for ordinary nodes) BEFORE any exec-node I/O.
    orig_exec_node = exec_node
    rt_node, prep_msg = br.nfs_lxc_prepare(orig_exec_node, verbose=True)
    if rt_node is None:
        _skip_all(f"bringup-failed: {prep_msg}")
        br.nfs_lxc_teardown(orig_exec_node, verbose=True)
        return
    try:
        _botactions_remote_body(rt_node, compile_node, label, runnable,
                                extra_rusage_count, progress, per_tool,
                                per_subbench, _fail_all, _skip_all,
                                local_bot_bin)
    finally:
        br.nfs_lxc_teardown(orig_exec_node, verbose=True)


def _botactions_remote_body(exec_node, compile_node, label, runnable,
                            extra_rusage_count, progress, per_tool,
                            per_subbench, _fail_all, _skip_all, local_bot_bin):
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]

    # 0. binary-size is a STATIC metric of the locally-built bot binary; it
    # needs no datapack, no server build, and no live server.  Collect it
    # FIRST so that ANY later server-pipeline failure (datapack rsync, server
    # build, push, missing-SSE2 boot crash) still leaves this datum recorded —
    # _fail_all() skips tools already filled in here.
    if "binary-size" in runnable:
        sz = bh.binary_size(local_bot_bin)
        if sz is not None:
            progress.emit("binary-size", "no", label, status="PASS")
            per_tool[label]["binary-size"] = {
                "status": "PASS",
                "metrics": _flat_to_metric_block(
                    {"binary_size_bytes": (sz, 0.0)})}
        else:
            progress.emit("binary-size", "no", label, status="FAIL",
                          extra="binary-size-failed")
            per_tool[label]["binary-size"] = {"status": "FAIL", "metrics": {}}

    # 1. rsync datapack to exec node
    if os.path.isdir(DATAPACK_PATH):
        # keep_skins=True: the client logs into a fresh RAM DB with no
        # character, so it CREATES one (skinId 0). Without >=1 real skin (a
        # folder with back/front/trainer.png) the server refuses skin 0 with
        # "the datapack has no skin, no character can be created" and no bot ever
        # reaches the map -> no offered load at all. It did not matter while the
        # remote rows only timed a host-side process that never needed a
        # character; it does now.
        rc_dp, msg_dp = br.rsync_datapack_to_exec(exec_node, DATAPACK_PATH,
                                                   remote_subdir="datapack",
                                                   timeout=600, server_mode=True,
                                                   keep_skins=True)
        if rc_dp != 0:
            _skip_all(f"datapack-rsync-failed: {msg_dp}")
            return

    maincode = _detect_maincode(DATAPACK_PATH)
    br.write_server_properties_on_exec(exec_node, maincode, port=SERVER_PORT,
                                       run_subdir=ewd)

    # 2. Build server-cli on compile node. DB_FILE_RAM keeps the DB in
    # /dev/shm so embedded boards never burn flash write cycles.
    # Stage the source tree FIRST: build_on_compile_node() compiles whatever is
    # already in <work>/sources/, so without this the node would build a tree
    # left behind by an earlier run and the candidate under test would never
    # reach the fleet. Once per compile node per run.
    rc_st, msg_st = br.stage_source_once(compile_node, verbose=True)
    if rc_st != 0:
        _skip_all(f"source-stage-failed: {msg_st}")
        return
    rc_srv, msg_srv, remote_srv_bld = br.build_on_compile_node(
        compile_node,
        cmake_src_subdir="server/cli",
        build_subdir=f"benchmarkbotactions-srv-{label}",
        cmake_defs=dict(SERVER_RAM_DB_DEFS, CMAKE_BUILD_TYPE="Release"),
        exec_node=exec_node,
        verbose=True,
    )
    if rc_srv != 0:
        # Compile-node build failed -> do NOT touch the exec node; SKIP.
        _skip_all(f"server-build-failed: {msg_srv}")
        return
    # Carry this compile node's system-vs-vendored verdict onto the exec
    # node so its history record stamps the libs it actually ran.
    bh.alias_libs(compile_node["label"], label)

    # 3. Push server binary to exec node.
    rc_ps, exec_srv_bin, msg_ps = br.push_binary_to_exec(
        compile_node, exec_node, remote_srv_bld, SRV_BIN_NAME, verbose=True)
    if rc_ps != 0:
        _skip_all(f"push-server-failed: {msg_ps}")
        return

    # 3a-bis. The measuring client for THIS arch, built on the same compile node
    # and pushed beside the server. It runs ON the node over 127.0.0.1: the
    # offered load, the client's own rusage and the server's CPU% then all belong
    # to the hardware under test, which is the whole point of the exercise.
    rc_bb, msg_bb, remote_bot_bld = br.build_bot_bench_on_compile_node(
        compile_node, build_subdir=f"botactions-bot-{label}",
        exec_node=exec_node, benchmark=True, verbose=True)
    if rc_bb != 0:
        _skip_all(f"client-build-failed: {msg_bb}")
        return
    rc_pb, _bot_exec_bin, msg_pb = br.push_bot_bench_to_exec(
        compile_node, exec_node, remote_bot_bld, verbose=True)
    if rc_pb != 0:
        _skip_all(f"push-client-failed: {msg_pb}")
        return

    # 3b. Pre-generate the datapack HPS cache on the compile node so the
    # embedded exec node boots from cache, not raw XML.
    rc_cg, remote_cache, msg_cg = br.pregenerate_datapack_cache(
        compile_node, remote_srv_bld, SRV_BIN_NAME, DATAPACK_PATH,
        maincode, server_port=SERVER_PORT, verbose=True, keep_skins=True)

    # 3c. Server boot-time benchmark (cold vs warm).
    # Cache is always staged for the main benchmark run (step 4); the cold-boot
    # measurement temporarily removes it and re-stages it afterward.
    boot_slices = {}
    boot_flat   = {}
    cache_staged = False
    if rc_cg == 0 and remote_cache:
        # Cold boot: remove cache so server parses XML.
        br.ssh_run(eu, eh, ep,
                   f"rm -f {shlex.quote(ewd + '/datapack-cache.bin')} "
                   f"{shlex.quote(ewd + '/datapack-cache.bin.tmp')}", timeout=15)
        cold_s, cold_msg = br.measure_remote_server_boot(
            exec_node, SRV_BIN_NAME, ready_timeout=SERVER_BOOT_TIMEOUT,
            verbose=True)
        if cold_s is not None:
            boot_flat["boot_no_cache_s"] = (cold_s, 0.0)
            boot_slices["no-cache"] = {"boot_to_bind_s": {
                "value": cold_s, "median": cold_s, "stddev": 0.0,
                "unit": "s", "better": "lower", "samples": [cold_s]}}
        else:
            print(_color(bh.C_YELLOW,
                  f"[server-boot] {label}: cold boot not measured ({cold_msg})"))
        # Warm boot: stage cache from compile node, then boot.
        rc_sc, msg_sc = br.stage_cache_on_exec(
            compile_node, exec_node, remote_cache, verbose=True)
        if rc_sc == 0:
            cache_staged = True
            warm_s, warm_msg = br.measure_remote_server_boot(
                exec_node, SRV_BIN_NAME, ready_timeout=SERVER_BOOT_TIMEOUT,
                verbose=True)
            if warm_s is not None:
                boot_flat["boot_with_cache_s"] = (warm_s, 0.0)
                boot_slices["with-cache"] = {"boot_to_bind_s": {
                    "value": warm_s, "median": warm_s, "stddev": 0.0,
                    "unit": "s", "better": "lower", "samples": [warm_s]}}
            else:
                print(_color(bh.C_YELLOW, f"[server-boot] {label}: warm boot "
                      f"not measured ({warm_msg})"))
        else:
            print(_color(bh.C_YELLOW, f"[server] {label}: cache stage failed "
                  f"({msg_sc})"))
    else:
        print(_color(bh.C_YELLOW, f"[server] {label}: cache pre-gen skipped "
              f"({msg_cg})"))
    if boot_flat:
        per_tool[label]["server-boot"] = {
            "status": "PASS", "metrics": _flat_to_metric_block(boot_flat)}
        per_subbench.setdefault(label, {})["server-boot"] = boot_slices

    # Ensure the cache is staged for the main benchmark run even if warm-boot
    # measurement was skipped or staging above failed.
    if not cache_staged:
        if rc_cg == 0 and remote_cache:
            rc_sc2, msg_sc2 = br.stage_cache_on_exec(
                compile_node, exec_node, remote_cache, verbose=True)
            if rc_sc2 != 0:
                _skip_all(f"cache-stage-failed: {msg_sc2}")
                return
        else:
            _skip_all(f"no-datapack-cache: {msg_cg}")
            return

    # 4. Start server on exec node; wait for ready.
    # Python holds a dedicated SSH Popen for the server (foreground on remote).
    # No exec/setsid/nohup needed — the connection lifetime is managed here.
    srv_ssh_proc, srv_pid, srv_start_reason = br.start_server_popen(
        exec_node, SRV_BIN_NAME)
    if srv_ssh_proc is None:
        # Server binary built+pushed but won't launch on this node (e.g. an
        # illegal-instruction arch mismatch): no measurement -> SKIP, not a
        # perf regression.
        _skip_all(srv_start_reason)
        return

    log_q = shlex.quote(ewd + "/server.log")
    ready = False
    deadline_t = time.monotonic() + SERVER_READY_TIMEOUT
    while time.monotonic() < deadline_t:
        time.sleep(1)
        rc_chk, st, _ = br.ssh_run(eu, eh, ep,
            f"if grep -q 'correctly bind:' {log_q} 2>/dev/null; then echo BIND; "
            f"elif kill -0 {srv_pid} 2>/dev/null; then echo WAIT; "
            f"else echo DEAD; fi", timeout=10)
        state = st.strip() if rc_chk == 0 else ""
        if state == "BIND":
            ready = True
            break
        if state == "DEAD":
            _, tail, _ = br.ssh_run(
                eu, eh, ep, f"tail -n 3 {log_q} 2>/dev/null | tr '\\n' '|'",
                timeout=10)
            log = tail.strip().strip("|")
            if "llegal instruction" in log:
                missing = br._missing_baseline_isa(exec_node) or "SSE2"
                reason = (f"illegal-instruction: missing {missing} — binary "
                          f"built for a too-modern -march; rebuild for this arch")
            else:
                reason = "server-died-before-bind: " + (log or "no server.log")
            br.stop_server_popen(srv_ssh_proc, exec_node, srv_pid)
            _skip_all(reason)
            return
    if not ready:
        br.stop_server_popen(srv_ssh_proc, exec_node, srv_pid)
        _skip_all(f"server-not-ready-in-{SERVER_READY_TIMEOUT}s")
        return

    # 5. Run the client ON the exec node, against that node's own server.
    try:
        if "rusage" in runnable:
            all_metrics_flat = {}
            rusage_status    = "PASS"
            for i, bots in enumerate(BOT_COUNTS):
                cell, cell_err = cell_run(None, "127.0.0.1", SERVER_PORT, bots,
                                          iface=None, server_pid=srv_pid,
                                          exec_node=exec_node)
                if cell is None:
                    rusage_status = "FAIL"
                    reason = cell_err or f"bots={bots}: no data"
                    bh.print_node_error("benchmarkbotactions", label, "FAIL",
                                        reason)
                    progress.emit("rusage", "no", label, status="FAIL",
                                  extra=f"bots={bots}: {reason[:60]}")
                    per_tool[label]["rusage"] = {"status": "FAIL",
                                                 "metrics": {}, "error": reason}
                    continue
                for name, (med, std) in cell.items():
                    all_metrics_flat[f"b{bots}_{name}"] = (med, std)
                # Per-workload slice, so the mandatory cpu_percent sits next to
                # the work metric it rode on instead of only in the flat keys.
                per_subbench.setdefault(label, {}).setdefault("rusage", {})[
                    f"{bots}-bots"] = _flat_to_metric_block(cell)
                progress.emit("rusage", "no", label, status="PASS",
                              extra=f"bots={bots}")
            if rusage_status == "PASS":
                per_tool[label]["rusage"] = {
                    "status": "PASS",
                    "metrics": _flat_to_metric_block(all_metrics_flat)}

        # binary-size already collected at the top of the body (step 0).
        if "perf-stat" in runnable:
            # perf runs ON the node now, around the node-local client: counting
            # cycles of a process on the orchestrating host said nothing about
            # this board. The SERVER's own work + tail comes from its BENCH dump
            # below, which needs no perf and works on the nodes where perf is
            # unavailable or paranoid-blocked.
            bots = BOT_COUNTS[len(BOT_COUNTS) // 2]
            argv = _bot_argv("127.0.0.1", SERVER_PORT, bots, "datapack")
            cmd_str = (f"cd {shlex.quote(ewd)} && ./{shlex.quote(BOT_BIN_NAME)} "
                       + " ".join(shlex.quote(a) for a in argv))
            out = br.remote_perf_stat(exec_node, cmd_str,
                                      timeout=RUN_TIMEOUT + 60)
            perf_err = None if out else "perf unavailable or paranoid-blocked"
            if out:
                progress.emit("perf-stat", "no", label, status="PASS")
                per_tool[label]["perf-stat"] = {
                    "status": "PASS",
                    "metrics": _flat_to_metric_block(
                        {f"perf_{k}": (v, 0.0) for k, v in out.items()
                         if isinstance(v, (int, float))})}
            else:
                reason = perf_err or "perf-no-hw-counters"
                progress.emit("perf-stat", "no", label, status="SKIP",
                              extra=reason)
                per_tool[label]["perf-stat"] = {"status": "SKIP", "metrics": {},
                                               "skip_reason": reason}
    finally:
        # SIGINT makes the server dump its BENCH counters before exiting, so
        # stop it BEFORE reading the log.
        br.stop_server_popen(srv_ssh_proc, exec_node, srv_pid)
        # Server-side work + latency tail for this node's whole load run
        # (cumulative over every slice: the dump happens once, at exit). Never
        # report req/s without its tail -- a throughput win that inflates p99 is
        # a tick-stability regression on the constrained targets.
        srv_flat = {}
        rc_log, srv_text, _e = br.ssh_run(
            eu, eh, ep, f"cat {shlex.quote(ewd + '/server.log')} 2>/dev/null",
            timeout=60)
        if rc_log == 0 and srv_text:
            packets = _bench_packets_in_from_text(srv_text)
            if packets is not None:
                srv_flat["bench_packets_in"] = (packets, 0.0)
            for name, value in _bench_latency_from_text(srv_text).items():
                srv_flat[name] = (value, 0.0)
            for name, block in bh.parse_loop_selfprobe(srv_text).items():
                srv_flat[name] = (block["value"], 0.0)
        if srv_flat:
            per_tool[label]["server-bench"] = {
                "status": "PASS", "metrics": _flat_to_metric_block(srv_flat)}
        else:
            print(_color(bh.C_YELLOW, f"[server-bench] {label}: no BENCH dump in "
                                      f"server.log (killed before it could dump?)"))


def main():
    args = bh.parse_bench_args()
    bh.set_node_filter(args.node)
    if bh.acquire_singleton_lock("benchmarkbotactions") is None:
        return 3
    br.set_benchmark_label("benchmarkbotactions")
    # Network benchmark: binds a fixed server port + drives the NIC/loopback,
    # so it must run serially against every other network benchmark (waits
    # its turn on the shared lock). Non-network benchmarks skip this.
    if NETWORK_EXCLUSIVE:
        if bh.acquire_network_lock("benchmarkbotactions") is None:
            return 3
    comment = args.comment
    maxtime = args.maxtime
    # Build BOTH binaries first so a build failure aborts before we
    # spend wall-time on cell runs / server boot.
    bin_path = build_bot()
    if bin_path is None:
        return 2
    server_bin = build_server()
    if server_bin is None:
        return 2

    staged = setup_server_run_dir(server_bin)
    if args.profile:
        # Profile the server under bot load for each requested tool. This
        # benchmark's server+bot workload is local-only (SQPOLL is
        # kernel/CPU specific and the remote dispatch isn't wired), so
        # --profile stays on the host; artefacts are still node/core-tagged.
        tools = bh.profile_tools(args.profile)
        # Warm the HPS datapack cache BEFORE profiling. Without this the
        # server boots cold and re-parses the whole XML datapack, so
        # tinyXML2 / isalpha / isspace dominate the profile (~10% Ir) and
        # drown the steady-state hot path this benchmark exists to
        # optimise. `save` writes datapack-cache.bin so the profiled boot
        # loads the binary cache instead. Mirrors _measure_local_boot's
        # warm step on the non-profile path.
        bh.run_capture([staged, "save"], timeout=RUN_TIMEOUT, cwd=SRV_RUN_DIR,
                       preexec_fn=bh._drop_core_rlimit)
        rc = 0
        for t in tools:
            if profile_server(staged, bin_path, t) != 0:
                rc = 2
        print(_color(bh.C_YELLOW, "[profile] benchmarkbotactions: server+bot "
              "workload is local-only — remote exec nodes not profiled"))
        return rc
    # Boot-time benchmark BEFORE the persistent server start: cold (no
    # cache) vs warm (cache via `save`). Leaves a valid cache behind so
    # the persistent start_server() below boots warm/fast.
    local_boot_flat, local_boot_slices = _measure_local_boot(staged)
    server_proc = start_server(staged)
    if server_proc is None:
        return 2
    try:
        return _run_with_server(bin_path, server_proc, comment,
                                local_boot_flat, local_boot_slices, maxtime)
    finally:
        print(_color(bh.C_CYAN, "[server] stopping"))
        stop_server(server_proc)


def _run_with_server(bin_path, server_proc, comment,
                     local_boot_flat=None, local_boot_slices=None,
                     maxtime=None):
    host = "127.0.0.1"
    port = SERVER_PORT
    print(_color(bh.C_CYAN, f"[bench] target = {host}:{port} (embedded)"))

    iface = _default_iface()

    arch = bh.host_arch()
    # The client is Qt-FREE (tools/bot-bench: POSIX sockets + select()), so it
    # builds and runs on every arch of the fleet -- including the 11 headless
    # boards with no Qt6 runtime, where the old GUI-linked bot-actions could
    # not run at all. Each remote row therefore measures client AND server on
    # the same hardware; see _run_remote_cells_botactions().
    # --node may exclude the host baseline; only prepend "local" when allowed.
    local_node = [{"label": "local", "arch": arch}] if bh.node_allowed("local", arch) else []
    nodes = local_node + bh.benchmark_exec_nodes()
    all_profilers = ["rusage", "binary-size", "perf-stat"]

    # Pre-resolve per-node profiler availability against the persisted
    # benchmark_disabled_tools list + a live probe.
    # 'rusage' now runs the client ON the exec node, so /usr/bin/time must be
    # probed THERE. It is not a hard requirement: without it the run still
    # happens and only the client's own CPU%/RSS is missing (recorded null),
    # so the probe result is advisory -- the local flag below just keeps the
    # host row's behaviour unchanged.
    local_has_time = (os.path.isfile("/usr/bin/time") and
                      os.access("/usr/bin/time", os.X_OK))
    node_profilers = {}
    node_skips     = {}
    for node in nodes:
        if node["label"] == "local":
            avail, skips = bh.profilers_runnable_on(node, all_profilers)
        else:
            # Don't probe /usr/bin/time on the remote exec node — rusage runs
            # the bot locally, so only the local /usr/bin/time matters.
            remote_profilers = [p for p in all_profilers if p != "rusage"]
            avail, skips = bh.profilers_runnable_on(node, remote_profilers)
            if local_has_time:
                avail.append("rusage")
            else:
                skips["rusage"] = "missing-tool:/usr/bin/time"
        node_profilers[node["label"]] = avail
        node_skips[node["label"]]     = skips

    total = sum(len(node_profilers[node["label"]]) + max(len(BOT_COUNTS) - 1, 0)
                for node in nodes)
    progress = bh.Progress(total, "benchmarkbotactions")
    deadline = bh.FleetDeadline()

    batch_id    = hr.new_batch_id()
    started_utc = hr.iso_now()
    # Pre-workload throttle-counter baseline for the local host so the
    # post-run sensor read attributes throttling to THIS run (delta), not
    # since-boot history. Remote nodes use the single post-run read.
    sensor_pre  = hr.sensor_baseline(hr.local_runner)
    compile_flags = ["-O3", "-DCMAKE_BUILD_TYPE=Release"]

    per_tool = {}     # node_label -> { tool -> {status, metrics} }
    per_subbench = {} # node_label -> { tool -> { label -> {metric -> {value,unit,better,...}} } }
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
        # Inject the local boot-time benchmark (measured in main() before
        # the persistent server started). Remote nodes record their own
        # server-boot inside _run_remote_cells_botactions.
        if label == "local" and local_boot_flat:
            per_tool[label]["server-boot"] = {
                "status": "PASS",
                "metrics": _flat_to_metric_block(local_boot_flat)}
            if local_boot_slices:
                per_subbench[label]["server-boot"] = local_boot_slices
        if deadline.reached():
            per_tool[label] = deadline.skip_node(label, all_profilers,
                                                 progress)
            # mirror the remote path's extra BOT_COUNTS-1 rusage emits so
            # the progress counter total still reconciles.
            for _ in range(max(len(BOT_COUNTS) - 1, 0)):
                progress.emit("rusage", "no", label, status="SKIP",
                              extra="deadline")
            continue
        if label != "local":
            _run_remote_cells_botactions(
                node, node_profilers[label], node_skips[label],
                all_profilers,
                extra_rusage_count=max(len(BOT_COUNTS) - 1, 0),
                progress=progress, per_tool=per_tool,
                per_subbench=per_subbench,
                compile_flags=compile_flags,
                local_bot_bin=bin_path)
            continue

        # rusage cell: one record per BOT_COUNTS size; metrics keyed
        # with b<count>_ prefix so the schema stays one-flat-dict.
        if "rusage" in node_profilers[label]:
            all_metrics = {}
            rusage_status = "PASS"
            sub = {}  # sub-bench label -> per-metric block
            deadline.note(label, "rusage")
            for bots in BOT_COUNTS:
                cell, cell_err = cell_run(bin_path, host, port, bots, iface,
                                          server_pid=server_proc.pid)
                if cell is None:
                    reason = cell_err or f"bots={bots}: no data"
                    bh.print_node_error("benchmarkbotactions", label, "FAIL",
                                        reason)
                    progress.emit("rusage", "no", label, status="FAIL",
                                  extra=f"bots={bots}: {reason[:60]}")
                    rusage_status = "FAIL"
                    continue
                # 1. flatten into the legacy b<count>_<name> dict for the
                #    champion-compare path (unchanged behaviour).
                # 2. drop a per-workload sub-bench record carrying CPU%
                #    plus the other rusage fields, keyed by "<n>-bots".
                slice_flat = {}
                for name, (med, std) in cell.items():
                    all_metrics[f"b{bots}_{name}"] = (med, std)
                    slice_flat[name] = (med, std)
                sub[f"{bots}-bots"] = _flat_to_metric_block(slice_flat)
                progress.emit("rusage", "no", label, status="PASS",
                              extra=f"bots={bots}")
            per_tool[label]["rusage"] = {
                "status":  rusage_status,
                "metrics": _flat_to_metric_block(all_metrics),
            }
            per_subbench[label]["rusage"] = sub
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
            cmd = ["timeout", "--signal=INT", str(RUN_TIMEOUT)] + \
                  _bot_cmd(bin_path, host, port, BOT_COUNTS[len(BOT_COUNTS) // 2])
            deadline.note(label, "perf-stat")
            out = bh.measure_perf_stat(cmd, env=env, timeout=RUN_TIMEOUT + 30)
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
                reason = bh.perf_no_hw_skip("local")
                per_tool[label]["perf-stat"] = {"status": "SKIP", "metrics": {}}
                progress.emit("perf-stat", "no", label, status="SKIP", extra=reason)
        else:
            progress.emit("perf-stat", "no", label, status="SKIP",
                          extra=node_skips[label].get("perf-stat", "tool-missing"))
            per_tool[label]["perf-stat"] = {"status": "SKIP", "metrics": {}}

        # io_uring x visibility-mode sweep (LOCAL only): each io_uring
        # tuning variant (baseline / +SQPOLL / +COOP_TASKRUN /
        # +TASKRUN_FLAG / +NO_SQARRAY) run under BOTH MapVisibilityAlgorithm
        # broadcast strategies — min_balanced and min_CPU — toggled at
        # runtime. Recorded as the "server-iouring" tool + per-(variant,mode)
        # sub-benchmark slices carrying CPU% + req/s + bytes-on-wire + the
        # latency tail, so the CPU-vs-bytes tradeoff of the two strategies is
        # reviewable under every io_uring config.
        # note() before each sweep: both spin up server+bot fleets several
        # times (minutes each), so without their own breadcrumb a deadline
        # firing here would be misattributed to the last note()'d cell
        # (perf-stat) instead of the sweep that actually ate the time.
        deadline.note(label, "server-iouring")
        iou_flat, iou_slices = _run_iouring_variants(bin_path, iface)
        if iou_flat:
            per_tool[label]["server-iouring"] = {
                "status": "PASS", "metrics": _flat_to_metric_block(iou_flat)}
            per_subbench.setdefault(label, {})["server-iouring"] = iou_slices

        # Compression sweep (LOCAL only): zstd-6 vs none across all workload
        # sizes. Recorded as the "server-compression" tool + per-(variant,
        # size) sub-benchmark slices carrying req/s + cpu% + bytes-on-wire +
        # latency tail, so the CPU/bandwidth tradeoff is reviewable.
        deadline.note(label, "server-compression")
        comp_flat, comp_slices = _run_compression_variants(bin_path, iface)
        if comp_flat:
            per_tool[label]["server-compression"] = {
                "status": "PASS", "metrics": _flat_to_metric_block(comp_flat)}
            per_subbench.setdefault(label, {})["server-compression"] = comp_slices

    # Cross-platform candidate record.
    sha = bh.git_sha()
    ended_utc = hr.iso_now()
    rec = {
        "commit":   sha,
        "comment":  comment,
        "date":     ended_utc,
        "started_utc": started_utc,
        "ended_utc": ended_utc,
        "duration_seconds": hr.duration_seconds(started_utc, ended_utc),
        "batch_id": batch_id,
        "benchmark": "benchmarkbotactions",
        "nodes":    {},
    }
    for node in nodes:
        label = node["label"]
        tools = per_tool.get(label)
        if not tools:
            continue
        m = {}
        for tool, blk in tools.items():
            for k, v in blk.get("metrics", {}).items():
                m[k] = {"median": v.get("median", v.get("value")),
                        "stddev": v.get("stddev", 0.0),
                        "unit":   v.get("unit"),
                        "better": v.get("better", "lower")}
        if m:
            rec["nodes"][label] = {"arch": node.get("arch", "?"),
                                    "libs": bh.LIBS_BY_NODE.get(label, {}),
                                    "metrics": m}

    # No candidate-<stamp>.json: nothing reads it (bh.candidate_path had
    # no reader), and every number in it is already in the per-platform
    # history records below + champion.json on promotion.

    # Cross-platform champion compare. SKIP on a --node run: decision +
    # champion promotion need the WHOLE fleet.
    partial_run = bh.node_filter_active() or truncated
    decision = None
    if not partial_run:
        champ = bh.load_champion("benchmarkbotactions")
        decision, summary = bh.decide_multi_node(champ, rec)
        bh.print_decision("benchmarkbotactions", decision, summary)

        if decision == "KEEP":
            ch_p = bh.champion_path("benchmarkbotactions")
            bh.write_record(ch_p, rec)
            print(_color(bh.C_GREEN, f"[champion] promoted -> {ch_p}"))

    # Per-platform history.
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
            cc_path, dp_path = SRV_RUN_DIR, DATAPACK_PATH
        else:
            wd = node.get("work_dir") or "/tmp/cc-bench-botactions"
            cc_path, dp_path = wd, os.path.join(wd, "datapack")
        pr = hr.PlatformRecord("benchmarkbotactions", batch_id,
                               node["label"], runner=runner,
                               arch_hint=node.get("arch")).collect(
                                   cc_binary_path=cc_path,
                                   datapack_path=dp_path,
                                   sensor_baseline=(sensor_pre
                                       if node["label"] == "local" else None))
        for tool, blk in per_tool[node["label"]].items():
            pr.add_result(tool, blk["metrics"], status=blk["status"])
        # Per-workload sub-benchmark slices (cpu_percent + wall_s + ...).
        for tool, slices in per_subbench.get(node["label"], {}).items():
            for slabel, smetrics in slices.items():
                pr.add_subbenchmark(tool, slabel, smetrics)
        out_p = pr.write(commit=sha, started_utc=started_utc,
                         ended_utc=ended_utc,
                         compile_flags=compile_flags
                             + list(br.exec_node_flag_defs(node).values()),
                         simd_tier="generic",
                         harness_version=hr.harness_version(),
                         comment=comment)
        if out_p is not None:
            bh.chatter(_color(bh.C_CYAN, f"[history] {out_p}"))

    if not partial_run:
        hr.attach_decision("benchmarkbotactions", batch_id, decision)
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
    bh.regenerate_charts("benchmarkbotactions")

    return 0


if __name__ == "__main__":
    sys.exit(main())
