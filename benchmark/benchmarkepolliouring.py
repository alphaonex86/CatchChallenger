#!/usr/bin/env python3
"""benchmarkepolliouring.py -- A/B the server's epoll event loop against its
io_uring event loop AT SATURATION, with the measuring client running ON the
exec node next to the server, and every node measured in PARALLEL.

The SERVER is the thing under test. server/cli is built TWICE from the SAME
source tree on the exec node's parent compile node -- once as-is (epoll) and
once with -DCATCHCHALLENGER_IO_URING=ON -- and both binaries are run on the
SAME constrained exec node against the SAME workload. The measuring client
(tools/bot-bench, Qt-free) is built on the same compile node and pushed to the
SAME exec node, so the whole workload is node-local over 127.0.0.1: no
orchestrating host in the loop, no physical link in the measurement path.
Loopback still goes through the real socket/syscall/kernel path, which is
exactly what an epoll-vs-io_uring comparison is about.

# HEADLESS: yes  (both binaries are CLI; no display server is needed)
Metric (per backend, per repetition):
  * req_per_s  -- requests/s the client sustained over its own fixed window,
                  read from bot-bench's "REQ_PER_S <float>" line. HIGHER is
                  better. This is the HEADLINE: the server is driven into
                  saturation, so both backends sit at the SAME operating point
                  (server CPU-bound) and their throughput is directly
                  comparable.
  * moves / survivors      -- work done and how many bots were still connected
                  at the end (load-parity check; see verdict()).
  * cpu_s / run_delay_s    -- the server's OWN /proc/<pid>/schedstat field 0
                  (ns actually on CPU) and field 1 (ns runnable but waiting
                  for a CPU), sampled ON the node once a second. Context, not
                  the headline. field 1 quantifies how much the node-local
                  client starved the server -- essential on a single-core
                  board, where it is reported instead of pretending there is
                  no contention.
  * rss_mb     -- server VmHWM over the run (lower better, context).
  * packets_in / lat p50/p99 -- from the server's own "BENCH <k>=<v>" dump.
                  Only with --counters: by default both binaries are built
                  exactly as they ship, because the event-loop probe does not
                  measure the same code on the two backends (see SERVER_DEFS).
Sampled, not deterministic: the verdict is the NON-OVERLAPPING min..max range
across repetitions, never a better mean alone (see "Verdict" below).

WHY THIS SHAPE -- THREE MEASURED FAILURES OF THE PREVIOUS DESIGN
---------------------------------------------------------------
The previous version ran the bot fleet on the ORCHESTRATING HOST and drove the
server over the physical network, reporting CPU-us per move. Three problems,
all measured on the reference board, and all fixed by moving the client onto
the node and measuring throughput at saturation:

 1. Server CPU was read over SSH: one handshake costs ~0.65 s idle and ~1.4 s
    under load, and the fleet kept moving during it -- a 2.8-3.5 % move-count
    error against a ~10 % effect. GONE by construction: the schedstat samples
    are taken by the same node-local shell that runs the client, so no SSH
    round trip is ever inside the measured window.
 2. The workload is closed-loop, so over a fixed window the two backends ran
    at DIFFERENT move rates (589/s vs 389/s measured) -- i.e. at different
    points on the load curve. CPU-per-move is load-dependent, so that was not
    a like-for-like comparison; under fixed WORK they instead ran for
    different durations. Neither is clean. FIXED: the client saturates the
    server (--spam), so both backends are pinned at the same operating point
    (server CPU-bound) and requests/s is then directly comparable.
 3. The last run of the old design produced OVERLAPPING ranges (epoll median
    309.4 us/move [306.0..321.6] vs io_uring 298.9 [296.0..312.1]) -- "no
    separation". The previously published -9.8 % does NOT reproduce, so it is
    not a baseline this file claims any continuity with.

CPU CONTENTION IS NOW PART OF THE RESULT
----------------------------------------
Client and server share the node's CPU, so the client necessarily takes cycles
away from the thing under test. That is reported, never hidden:
  * more than one core -> server pinned to CPU 0 and client to CPU 1..n-1 with
    taskset, so they do not fight for the same core;
  * exactly one core -> nothing to pin. The run says so and reports schedstat
    field 1 (run_delay) so the reader sees how long the server sat runnable
    waiting for the CPU;
  * contention alone does NOT void the verdict. The client is required to run
    next to the server, so on a single-core board it necessarily takes about
    half the CPU and both arms are starved equally (measured on the geode:
    epoll x1.03/1.01/1.02, io_uring x1.02/1.02/1.02). Equal starvation
    depresses both ABSOLUTE req/s figures and leaves their RATIO -- what is
    actually claimed -- intact. Each row prints its own [contended: ...] tag so
    the absolute number is never read as the board's ceiling.
  * what DOES void it is ASYMMETRY: if the two backends' run_delay/on-CPU
    differ by more than RUN_DELAY_SKEW_MAX they answered under different
    conditions, and the delta belongs to the scheduler rather than the event
    loop. Same for a survivor-count skew (SURVIVOR_SKEW_MAX).

PARALLEL FLEET
--------------
Two phases, following br.run_profiler_fleet() (benchmark_remote.py):
  phase 1 -- every UNIQUE (compile node, per-CPU-flag set) builds its three
             binaries (server epoll, server io_uring, bot-bench) in parallel;
  phase 2 -- every exec node measures in parallel, one worker per distinct
             exec host, so wall time is the SLOWEST node and not the sum.
benchmarkclientlatency.py deliberately keeps its measurement SERIAL because
concurrent load on the ORCHESTRATING HOST perturbs the latency it measures
there. That objection does not apply here and must not be re-applied: the
client no longer runs on the host, so the host does nothing during a
measurement but sleep on an ssh read. Do NOT re-serialise this loop.

VERDICT
-------
A better mean is not a result: these runs are noisy on constrained hardware.
A backend is declared faster only when its min..max range over the repetitions
does NOT overlap the other's. Overlapping ranges are reported as "no
separation" -- which is a real answer, not a failure.

WHY IT IS NOT IN all.sh
-----------------------
Deliberately NOT registered in benchmark/all.sh. It builds three binaries per
compile node and runs 2 x --reps saturating runs on slow reference hardware,
so a full A/B would dominate the suite's wall time while answering a question
that only comes up when the event loop itself changes. Run it on demand:

    ./benchmarkepolliouring.py [--node <label|arch>] [--reps 3] [--seconds N]

Every host/user/port/work_dir comes from remote_nodes.json via
bh.benchmark_exec_nodes(); this file contains no infrastructure. There is no
"local" row on purpose -- an amd64 workstation answers a bot fleet from its
cache and the two backends both idle, so its numbers say nothing about either.
"""
import argparse
import concurrent.futures as cf
import hashlib
import os
import re
import shlex
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh
import benchmark_remote as br

REPO_ROOT = bh.REPO_ROOT
BENCH = "benchmarkepolliouring"

# Binds a fixed server port: serialise against the OTHER network benchmarks
# through the shared machine-wide flock. It does NOT serialise the nodes of
# THIS run against each other (see main(): the lock is taken once, never
# per node) -- with every node talking to its own loopback there is no shared
# link and no port collision between nodes, only between benchmarks.
NETWORK_EXCLUSIVE = True

# Both are built ON the compile node, so these are paths inside the staged
# source tree, not local paths.
SRV_SRC_DIR  = "server/cli"
SRV_BIN_NAME = "catchchallenger-server-cli"
BOT_SRC_DIR  = "tools/bot-bench"
BOT_BIN_NAME = "bot-bench"

# The datapack source of truth sits next to the repo checkout; derive it rather
# than hard-code a path. CC_DATAPACK overrides for a differently laid-out host.
DATAPACK_PATH = os.environ.get(
    "CC_DATAPACK",
    os.path.join(os.path.dirname(REPO_ROOT), "CatchChallenger-datapack"))

try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
except Exception:
    # No test/config.json on this host: fall back to the system temp dir.
    BUILD_ROOT = os.path.join(os.environ.get("TMPDIR", "/tmp"), "cc-bench-build")

LOG_DIR = os.path.join(BUILD_ROOT, "benchmark", "epolliouring-logs")

# Distinct from the sibling network benchmarks (61920 botactions,
# 61921 clientlatency) so a stale server of theirs can never be measured here.
SERVER_PORT = 61922

# One account, N character slots -- bot-bench derives its per-bot character
# from the slot index, so a single login/pass drives the whole fleet. The
# server runs with automatic_account_creation and a RAM DB, so the account is
# created on first use and gone again when the server exits.
BOT_LOGIN = "bench"
BOT_PASS  = "bench"

# Both builds: RAM DB (no disk I/O in the measured window).
# DB_INTERNAL_VARS implies DB_FILE in server/cli/CMakeLists.txt; both are
# passed explicitly so the configure line is self-describing.
#
# CATCHCHALLENGER_BENCHMARK is deliberately OFF, so the two measured binaries
# are the ones that actually ship -- and because its loop probe does not mean
# the same thing on the two backends.
#
# CC_BENCH_LOOP_IN/OUT bracket main-unix.cpp:888..1201, i.e. everything AFTER
# EventLoop::wait() returns. On epoll that window contains
# parseIncommingData() (main-unix.cpp:1113). On io_uring it contains almost
# none of the packet work: onAsyncRecv() runs INSIDE wait()
# (EventLoop.cpp:1214) and the batched broadcast sends are flushed by
# io_uring_submit_and_wait() inside wait() too (EventLoop.cpp:1134). So
# loop_busy_us is not a biased measure of the same thing on both backends --
# it measures a DIFFERENT SET OF CODE, and must never appear in an A/B
# headline. On top of that the probe itself costs two clock_gettime() per
# wakeup and the backends do not wake the same number of times per request.
#
# Nothing here needs any of it: the headline comes from the CLIENT's own
# req/s and the CPU context from /proc/<pid>/schedstat -- both external to the
# server binary. (packets_in and lat_hist_* ARE backend-comparable, one
# increment per parser dispatch on each path -- main-unix.cpp:1120 and
# EventLoop.cpp:1218 -- which is why --counters is still offered for context.)
#
# CATCHCHALLENGER_IO_URING is pinned OFF for the epoll build rather than left
# unset: build_on_compile_node reuses its build dir, and an unset option is
# read back from the existing CMakeCache.txt. A dir configured once with
# IO_URING=ON would otherwise make every later run build TWO io_uring
# binaries, measure the same backend twice, and publish the resulting overlap
# as a legitimate "no separation" result. deploy_node() additionally hard-fails
# if the two deployed binaries hash the same.
#
# CATCHCHALLENGER_CACHE_HPS is deliberately NOT enabled, and no pre-generated
# datapack cache is staged. When the binary cache is open the server loads its
# SETTINGS from that cache and never reads server-properties.xml
# (server/cli/main-unix.cpp -> loadSettingsFromBinaryCache), so the tuned
# max-players / mapVisibility Max / DDOS limits below would be silently
# replaced by whatever was baked in at cache-generation time. Paying a cold
# datapack parse per boot is the honest trade: it happens BEFORE the
# measurement window opens (the client onboards its bots first, and its window
# is the last --seconds of its own life).
#CATCHCHALLENGER_BENCHMARK=ON is REQUIRED here, and that needs explaining
#because it looks like the opposite of what the comment above argues.
#
#The anti-flood filter is not independently switchable: VariableServer.hpp:64
#defines CATCHCHALLENGER_DDOS_FILTER inside `#ifndef CATCHCHALLENGER_BENCHMARK`.
#With the filter compiled in, a saturating client is kicked after ~60 moves
#(measured: SURVIVORS 0/4, MOVES 256) and the run measures the filter instead
#of the event loop. Raising the runtime limits cannot rescue it either -- they
#are uint8_t (ServerStructures.hpp), so a value past 255 wraps to a STRICTER
#one. So there is no build that both saturates and leaves the filter in.
#
#Turning the probe on is safe for THIS design specifically: the headline is
#REQ_PER_S, measured by the CLIENT. The reason to fear the probe was that
#loop_busy_us brackets different code on the two backends -- which only
#matters for a metric derived from the probe, and none of ours is. What the
#probe still costs is two clock_gettime() per wakeup, and the backends do not
#wake the same number of times per move, so treat the ABSOLUTE req/s as
#carrying that overhead. The comparison stands because both binaries carry it;
#loop_busy_us must still never be quoted as an A/B result.
SERVER_DEFS = {
    "CATCHCHALLENGER_DB_FILE":          "ON",
    "CATCHCHALLENGER_DB_INTERNAL_VARS": "ON",
    "CATCHCHALLENGER_IO_URING":         "OFF",
    "CATCHCHALLENGER_BENCHMARK":        "ON",
}
IOURING_DEFS = dict(SERVER_DEFS)
IOURING_DEFS["CATCHCHALLENGER_IO_URING"] = "ON"

# --tuned: turn the io_uring sub-options ON for the io_uring arm. They exist
# precisely so this harness can compare the variants (server/cli/CMakeLists.txt).
# Not all of them are appropriate, and the reasons matter:
#
#  COOP_TASKRUN / TASKRUN_FLAG / NO_SQARRAY -- pure overhead reduction, no extra
#    threads and no extra CPU. Always safe to include.
#
#  SQPOLL -- real accelerator (it removes io_uring_enter entirely, which is
#    where the "80% fewer syscalls" figures in the literature come from) but it
#    SPINS A KERNEL THREAD. That is an extra core's worth of CPU that the epoll
#    arm does not get, so a win under SQPOLL is NOT like-for-like against
#    single-threaded epoll and must be read as "io_uring + a dedicated poller
#    beats epoll", a different claim. Added only where the node has a core to
#    spare; on a single-core board it would fight the server and the client for
#    the only CPU, so it is refused there rather than silently skewing the run.
#
#  IOPOLL -- deliberately NOT enabled. Per CMakeLists it applies to the FILE
#    ring (datapack load), NOT the socket ring, and is a no-op without O_DIRECT
#    (tmpfs rejects it). It cannot move a network benchmark, so switching it on
#    would only imply a tuning that is not doing anything.
IOURING_TUNED_EXTRA = {
    "CATCHCHALLENGER_IO_URING_COOP_TASKRUN":  "ON",
    "CATCHCHALLENGER_IO_URING_TASKRUN_FLAG":  "ON",
    "CATCHCHALLENGER_IO_URING_NO_SQARRAY":    "ON",
}
IOURING_SQPOLL_EXTRA = {"CATCHCHALLENGER_IO_URING_SQPOLL": "ON"}

BACKENDS = ("epoll", "iouring")

# 250 bots on ONE map is the protocol ceiling (8-bit per-map player index) and
# the load the event loop is meant to be measured under: every bot broadcasts
# its movement to all the others (benchmark/CLAUDE.md).
BOTS_DEFAULT  = 250
REPS_DEFAULT  = 3
MEASURE_SECS  = 40   # length of the client's own measured window

# One worker per distinct exec host, capped: 4 concurrent nodes is enough to
# collapse the fleet's wall time to the slowest node while keeping the ssh
# fan-out (and the local terminal) readable.
MAX_PARALLEL_NODES = 4

# Sampling period of the node-local schedstat sampler, in seconds. One `cat`
# per second on the node: identical for both backends and far below the load
# the client itself puts there, but it does mean the CPU window edges are
# quantised to ~1 s (fine for a CONTEXT metric -- the headline comes from the
# client and is not derived from these samples at all).
SAMPLE_S = 1

# Contention thresholds on run_delay/on-CPU (schedstat field 1 / field 0).
#   WARN -- the row is tagged; the server lost a visible slice of its time to
#           the scheduler but the comparison is still about the event loop.
#   VETO -- no winner is declared: the server spent as long WAITING for a CPU
#           as running on one, so what changed between the two rows could just
#           as well be the scheduler. On a single-core board with a saturating
#           client this is genuinely reachable -- which is the point: it is
#           better to report "the client starved the server" than to publish a
#           delta that means nothing.
RUN_DELAY_WARN_RATIO = 0.25
# Contention only invalidates the comparison when it is UNEQUAL between the two
# backends. The client is REQUIRED to run next to the server (that is the whole
# point of this design), so on a single-core board it necessarily takes about
# half the CPU and run_delay ~= on-CPU time for BOTH arms -- measured on the
# geode: epoll x1.03/1.01/1.02, io_uring x1.02/1.02/1.02. A level playing field
# that is level for both is not a confound: it depresses the ABSOLUTE req/s of
# both arms and leaves their RATIO meaningful, which is what is being claimed.
# Vetoing on the absolute ratio therefore threw away a valid measurement (the
# geode's ranges did separate). What must still veto is ASYMMETRY: if one
# backend was starved noticeably harder than the other, it answered under
# different conditions and the delta is the scheduler's, not the event loop's.
# Expressed as the allowed spread between the two backends' run_delay/on-CPU.
RUN_DELAY_SKEW_MAX = 0.25

# The two backends must carry the SAME load to be comparable. If one of them
# lost noticeably more bots (kicks, disconnects), it was answering a smaller
# fleet. Tolerance as a fraction of --bots.
SURVIVOR_SKEW_MAX = 0.05

SERVER_READY_TIMEOUT = 900   # cold datapack parse on a slow board
# Hard backstop for the one ssh call that runs the client: its own window plus
# room for onboarding N bots (login + character + map load) and teardown. It
# must always exceed the client's window (benchmark/CLAUDE.md: the timeout is
# derived from the budget, never picked independently).
CLIENT_MARGIN_S = 1200

# bot-bench's fixed output contract (tools/bot-bench). Anchored to the line
# start so a message that merely mentions the token cannot be mistaken for it.
REQ_RE   = re.compile(r"^REQ_PER_S ([0-9.]+)$", re.M)
MOVES_RE = re.compile(r"^MOVES ([0-9]+)$", re.M)
SURV_RE  = re.compile(r"^SURVIVORS (\d+)/(\d+)$", re.M)


def _c(colour, s):
    return f"{colour}{s}{bh.C_RESET}"


def _say(label, msg, colour=None):
    """One line, always prefixed with the node it belongs to: with several
    nodes measuring in parallel, unprefixed output is unreadable."""
    line = f"[{label}] {msg}"
    print(_c(colour, line) if colour else line, flush=True)


def _num(v, fmt):
    """Format a metric that may be unavailable. 'n/a' -- never 0.0, which
    would read as a measured zero."""
    return format(v, fmt) if v is not None else "n/a".rjust(len(format(0, fmt)))


# ---- CLI -----------------------------------------------------------------

def parse_args():
    """Own knobs first, then the SHARED benchmark flags (--node/--comment/
    --maxtime/--profile) through bh.parse_bench_args() so they keep exactly
    the fleet-wide semantics. Returns (own, shared)."""
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument("--reps", type=int, default=REPS_DEFAULT,
                    help="measured runs per backend (default %d)" % REPS_DEFAULT)
    ap.add_argument("--bots", type=int, default=BOTS_DEFAULT,
                    help="concurrent bots on one map (default %d, the "
                         "protocol ceiling)" % BOTS_DEFAULT)
    ap.add_argument("--seconds", type=int, default=MEASURE_SECS,
                    help="length of the client's measured window in seconds "
                         "(default %d)" % MEASURE_SECS)
    ap.add_argument("--tuned", action="store_true",
                    help="build the io_uring arm with its sub-options on "
                         "(COOP_TASKRUN, TASKRUN_FLAG, NO_SQARRAY, plus SQPOLL "
                         "where a core can be spared). SQPOLL spins a kernel "
                         "thread, so a win under it is not like-for-like "
                         "against single-threaded epoll -- the verdict says so")
    ap.add_argument("--sqpoll", action="store_true",
                    help="implies --tuned and adds SQPOLL: a spinning kernel "
                         "poll thread, i.e. an extra core epoll does not get. "
                         "Not like-for-like; refused on single-core nodes")
    ap.add_argument("--counters", action="store_true",
                    help="build both backends with CATCHCHALLENGER_BENCHMARK=ON "
                         "to also collect the server's BENCH dump. NOT the "
                         "default: the loop probe is not backend-neutral, so "
                         "it biases anything derived from it (see SERVER_DEFS)")
    argv = sys.argv[1:]
    if "-h" in argv or "--help" in argv:
        print(__doc__)
        print("Own flags: --reps N  --bots N  --seconds N  --tuned  --sqpoll  "
              "--counters\n")
    own, rest = ap.parse_known_args(argv)
    shared = bh.parse_bench_args(rest)
    if own.reps < 1 or own.bots < 1 or own.seconds < 5:
        print(_c(bh.C_RED, "[fatal] --reps/--bots must be >= 1, "
                           "--seconds >= 5"))
        sys.exit(2)
    if own.tuned or own.sqpoll:
        IOURING_DEFS.update(IOURING_TUNED_EXTRA)
        print(_c(bh.C_CYAN, "[tuned] io_uring arm gets COOP_TASKRUN + "
                            "TASKRUN_FLAG + NO_SQARRAY. IOPOLL stays off: it "
                            "drives the FILE ring, not the socket ring, so it "
                            "cannot move this benchmark."))
    if own.sqpoll:
        # Deliberately a SEPARATE opt-in rather than part of --tuned, and not
        # auto-enabled per node: it changes what is being compared, so the
        # operator has to ask for it. Refused on a single-core node, where the
        # poller would fight the server and the client for the only CPU.
        IOURING_DEFS.update(IOURING_SQPOLL_EXTRA)
        print(_c(bh.C_YELLOW, "[warn] --sqpoll: io_uring also gets a SPINNING "
                              "KERNEL POLL THREAD, i.e. roughly an extra core "
                              "of CPU that the epoll arm does not get. A win "
                              "here means 'io_uring plus a dedicated poller "
                              "beats epoll', which is a DIFFERENT claim from "
                              "'io_uring beats epoll'. Single-core nodes are "
                              "skipped."))
    if own.counters:
        # CATCHCHALLENGER_BENCHMARK is already ON in SERVER_DEFS -- it has to
        # be, or the anti-flood filter kicks the saturating client (see the
        # SERVER_DEFS comment). So this flag no longer changes the BUILD; it
        # only asks for the server's BENCH dump to be parsed and reported as
        # context alongside the client-measured headline.
        print(_c(bh.C_YELLOW, "[warn] --counters reports the server's own "
                              "BENCH dump. packets_in and lat_hist_* are "
                              "backend-comparable; loop_busy_us is NOT -- it "
                              "brackets different code on the two backends, so "
                              "never quote it as an A/B result."))
    return own, shared


# ---- server configuration -------------------------------------------------

def detect_maincode(datapack_src):
    """Prefer the small 'test' map set (fast parse); else the first dir."""
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


def server_properties_xml(maincode, port, bots):
    """server-properties.xml for the A/B. Not a production config:

    * max-players / mapVisibility Max are raised to the bot count so every bot
      is seated AND sees every other one -- the "everybody broadcasts to
      everybody" load the event loop is being compared under. A Max below the
      bot count silently shrinks the broadcast and the run stops measuring
      what it claims to. Max is a uint8_t that must stay under 254 (above it
      the simplified per-map id saturates), hence the clamp. Reshow is left at
      its default: it is a different knob (Minimize_Network only) and the
      server clamps it to Max anyway.
    * compression is off: it applies to the character-load protocol, not the
      per-tick packets, so it only adds a startup-time confound.
    * the DDOS kick limits are far above default because a saturating client
      would otherwise trip the flood kicker; we measure the event loop, not
      the anti-DDOS.
    Identical for BOTH backends, so nothing here can favour one of them."""
    vis = min(250, max(50, bots))
    return (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        f'    <server-port value="{port}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        f'    <max-players value="{max(50, bots) + 10}"/>\n'
        '    <httpDatapackMirror value=""/>\n'
        '    <compression value="none"/>\n'
        f'    <master><external-server-port value="{port}"/></master>\n'
        f'    <content><mainDatapackCode value="{maincode}"/>'
        '<subDatapackCode value=""/></content>\n'
        '    <mapVisibility>\n'
        '        <enable value="true"/>\n'
        # Without this the server picks MapVisibilityAlgorithmSelection_Simple
        # and the run silently measures a DIFFERENT broadcast path than the one
        # this A/B exists to compare. Confirmed from the server log line
        # "Visibility: MapVisibilityAlgorithmSelection_Simple" on a run where
        # the key was missing.
        '        <minimize value="network"/>\n'
        f'        <Max value="{vis}"/>\n'
        '    </mapVisibility>\n'
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


def write_properties(exec_node, xml):
    """Write server-properties.xml in the exec node's work_dir. Returns
    (ok, message)."""
    escaped = xml.replace("'", "'\\''")
    path = shlex.quote(exec_node["work_dir"] + "/server-properties.xml")
    rc, _o, err = br.run_remote_cmd(
        exec_node,
        f"mkdir -p {shlex.quote(exec_node['work_dir'])} && "
        f"printf '%s' '{escaped}' > {path}", timeout=60)
    if rc != 0:
        return False, f"server-properties.xml write failed rc={rc}: {err[:160]}"
    return True, "ok"


# ---- node capabilities ----------------------------------------------------

def node_capabilities(exec_node):
    """One round trip for the three facts the measurement shape depends on:
    core count, taskset availability, and whether the kernel exposes
    /proc/<pid>/schedstat (CONFIG_SCHEDSTATS). Returns (caps, error)."""
    rc, out, err = br.run_remote_cmd(
        exec_node,
        "echo CORES=$(nproc 2>/dev/null); "
        "if command -v taskset >/dev/null 2>&1; then echo TASKSET=yes; "
        "else echo TASKSET=no; fi; "
        "if [ -r /proc/self/schedstat ]; then echo SCHEDSTAT=yes; "
        "else echo SCHEDSTAT=no; fi", timeout=60)
    if rc != 0:
        return None, f"capability probe failed rc={rc}: {err[:160]}"
    caps = {"cores": None, "taskset": False, "schedstat": False}
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("CORES="):
            try:
                caps["cores"] = int(line.split("=", 1)[1])
            except ValueError:
                caps["cores"] = None
        elif line == "TASKSET=yes":
            caps["taskset"] = True
        elif line == "SCHEDSTAT=yes":
            caps["schedstat"] = True
    if not caps["cores"] or caps["cores"] < 1:
        caps["cores"] = 1   # nproc unavailable: assume the worst (contention)
    return caps, None


def cpu_sets(caps):
    """(server_cpu, client_cpus) taskset arguments, or (None, None) when the
    node cannot separate them. On a single core there is nothing to pin: the
    two processes MUST share it, and run_delay is what quantifies the damage."""
    if not caps["taskset"] or caps["cores"] < 2:
        return None, None
    return "0", ("1" if caps["cores"] == 2 else f"1-{caps['cores'] - 1}")


# ---- sampling / parsing ---------------------------------------------------

def parse_samples(text):
    """[(uptime_s, on_cpu_ns, run_delay_ns), ...] from the '#S' blocks the
    node-local sampler emits. Each block is `cat /proc/uptime
    /proc/<pid>/schedstat`, i.e. one line of two floats then one line of three
    integers. A block whose schedstat read failed (server already gone) is
    skipped rather than guessed."""
    out = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        if lines[i].strip() != "#S":
            i += 1
        else:
            blk = []
            j = i + 1
            while j < len(lines) and lines[j].strip() != "#S":
                s = lines[j].strip()
                if s and not s.startswith(("BOT_RC=", "VmHWM", "#CLIENTLOG")):
                    blk.append(s.split())
                elif s.startswith(("BOT_RC=", "VmHWM", "#CLIENTLOG")):
                    break
                j += 1
            if len(blk) >= 2 and len(blk[0]) >= 1 and len(blk[1]) >= 2:
                try:
                    out.append((float(blk[0][0]), int(blk[1][0]),
                                int(blk[1][1])))
                except ValueError:
                    pass
            i = j
    return out


def window_delta(samples, seconds):
    """(wall_s, cpu_ns, run_delay_ns) over the LAST `seconds` of the sampled
    run, or None when there is not enough of it.

    The client's own measured window is the last `--seconds` of its life, so
    attributing the server's CPU to the same interval means walking back from
    the final sample. It matches the client's window only to within the
    client's teardown time and the 1 s sampling period -- acceptable, because
    these numbers are CONTEXT: the headline req/s comes from the client itself
    and is not derived from them."""
    if len(samples) < 2:
        return None
    last = samples[-1]
    target = last[0] - seconds
    start = samples[0]
    for s in samples:
        if s[0] <= target:
            start = s
        else:
            break
    wall = last[0] - start[0]
    if wall <= 0:
        return None
    return (wall, last[1] - start[1], last[2] - start[2])


def parse_vmhwm(text):
    """Peak RSS in kB from a 'VmHWM: <n> kB' line, else None."""
    m = re.search(r"^VmHWM:\s+(\d+)", text, re.M)
    return int(m.group(1)) if m else None


def bench_values(text):
    """Parse the server's SIGTERM 'BENCH <k>=<v>' dump into a dict."""
    out = {}
    for k, v in re.findall(r"BENCH (\w+)=(\d+)", text):
        out[k] = int(v)
    return out


def bench_summary(text):
    """Context metrics from the BENCH dump: read events, event-loop busy us,
    and the server-side per-read-event latency tail. {} when absent."""
    v = bench_values(text)
    out = {}
    for k in ("packets_in", "loop_busy_us", "loop_wall_us", "loop_iterations"):
        if k in v:
            out[k] = v[k]
    buckets = {}
    for k, c in v.items():
        if k.startswith("lat_hist_"):
            try:
                buckets[int(k.split("_")[-1])] = c
            except ValueError:
                pass
    stats = bh.bench_hist_percentiles(buckets)
    if stats:
        out["lat_p50_ns"] = stats["p50_ns"]
        out["lat_p99_ns"] = stats["p99_ns"]
    return out


# ---- one measured run -----------------------------------------------------

def wait_for_bind(exec_node, pid):
    """Poll the exec node until the server logs 'correctly bind:'. Returns
    (ok, reason)."""
    log_q = shlex.quote(exec_node["work_dir"] + "/server.log")
    deadline = time.monotonic() + SERVER_READY_TIMEOUT
    while time.monotonic() < deadline:
        rc, out, err = br.run_remote_cmd(
            exec_node,
            f"if grep -q 'correctly bind:' {log_q} 2>/dev/null; then echo BIND; "
            f"elif kill -0 {pid} 2>/dev/null; then echo WAIT; else echo DEAD; fi",
            timeout=60)
        if rc != 0:
            return False, f"bind probe ssh rc={rc}: {err[:120]}"
        if "BIND" in out:
            return True, "ok"
        if "DEAD" in out:
            _rc, tail, _e = br.run_remote_cmd(
                exec_node, f"tail -n 5 {log_q} 2>/dev/null | tr '\\n' '|'",
                timeout=60)
            return False, f"server exited before bind: {tail.strip()[:200]}"
        time.sleep(2)
    return False, f"server did not bind within {SERVER_READY_TIMEOUT}s"


def pin_server(exec_node, pid, server_cpu):
    """Confine the already-running server to `server_cpu`. Affinity is set
    after launch rather than by wrapping the binary, so br.start_server_popen
    stays the single way this harness starts a server (and its pid file stays
    the server's own pid). Everything before the window -- the cold datapack
    parse -- runs unpinned, which does not matter: the window opens later.
    Returns (ok, message)."""
    rc, _o, err = br.run_remote_cmd(
        exec_node, f"taskset -pc {shlex.quote(server_cpu)} {pid}", timeout=60)
    if rc != 0:
        return False, f"taskset -pc {server_cpu} {pid} rc={rc}: {err[:120]}"
    return True, "ok"


def stop_and_collect(exec_node, ssh_proc, pid):
    """SIGTERM the server (NEVER SIGKILL: with --counters the benchmark
    counters are dumped from the SIGTERM handler as 'BENCH <k>=<v>' on stdout,
    i.e. into server.log -- a -9 loses them), wait for it to actually exit,
    then return the server log text."""
    rc, _o, err = br.run_remote_cmd(exec_node, f"kill -TERM {pid} 2>/dev/null",
                                    timeout=60)
    if rc != 0:
        _say(exec_node["label"], f"SIGTERM to pid {pid} rc={rc}: {err[:120]}",
             bh.C_YELLOW)
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline:
        rc, out, _e = br.run_remote_cmd(
            exec_node, f"if kill -0 {pid} 2>/dev/null; then echo UP; "
                       f"else echo GONE; fi", timeout=60)
        if rc != 0 or "GONE" in out:
            break
        time.sleep(1)
    log_q = shlex.quote(exec_node["work_dir"] + "/server.log")
    rc, text, _e = br.run_remote_cmd(
        exec_node, f"tail -c 400000 {log_q} 2>/dev/null", timeout=120)
    if rc != 0:
        text = ""
    # Close the launch SSH + make sure nothing survives into the next run.
    br.stop_server_popen(ssh_proc, exec_node, pid)
    return text


def server_log_hint(exec_node, msg, lines=12):
    """Append the tail of the server's own log to a failure message.

    Every way a measured run can die -- the client exiting, no bot reaching
    the map, no REQ_PER_S line -- looks identical from the outside, and the
    reason is almost always in the SERVER log ("Kicked by: ...", a bind
    failure, a crash). Fetching it on the failure path is what turns "no
    output" into a diagnosis; without it the log is destroyed by the next
    run's `rm -f server.log` before anyone can look."""
    rc, out, _err = br.run_remote_cmd(
        exec_node,
        f"tail -{int(lines)} {shlex.quote(exec_node['work_dir'] + '/server.log')}"
        " 2>/dev/null", timeout=60)
    if rc != 0 or not out.strip():
        return f"{msg} [server log unavailable]"
    return msg + "\n  --- server.log tail ---\n  " + \
        "\n  ".join(out.strip().splitlines())


def client_command(work_dir, pid, bots, seconds, client_cpus):
    """The ONE remote shell command that runs a measured window.

    Everything happens on the node: the client is launched in the background,
    the server's /proc/<pid>/schedstat is sampled once a second by the same
    shell while it runs, and the client's own stdout is kept in client.log and
    tailed back at the end. No ssh round trip is inside the window, so the
    ~0.65-1.4 s handshake that biased the previous design cannot appear here.

    `taskset` is only used when the node has a core to spare; on a single-core
    board the two processes share the CPU and schedstat field 1 reports it."""
    pfx = f"taskset -c {shlex.quote(client_cpus)} " if client_cpus else ""
    sample = f'echo "#S"; cat /proc/uptime /proc/{pid}/schedstat 2>/dev/null'
    return (
        f"cd {shlex.quote(work_dir)} && {{ "
        f"{pfx}./{BOT_BIN_NAME} --host 127.0.0.1 --port {SERVER_PORT} "
        f"--login {shlex.quote(BOT_LOGIN)} --pass {shlex.quote(BOT_PASS)} "
        f"--bots {int(bots)} --spam --seconds {int(seconds)} "
        f">client.log 2>&1 & "
        f"CPID=$!; "
        f"{sample}; "
        f"while kill -0 $CPID 2>/dev/null; do sleep {int(SAMPLE_S)}; "
        f"{sample}; done; "
        f"wait $CPID; echo \"BOT_RC=$?\"; "
        f"{sample}; "
        f"grep VmHWM /proc/{pid}/status 2>/dev/null; "
        f"echo '#CLIENTLOG'; tail -c 200000 client.log; }}")


def run_once(exec_node, backend, own, caps, tag):
    """One measured run of ONE backend on ONE node. Returns (row, error);
    exactly one of the two is None."""
    ewd   = exec_node["work_dir"]
    label = exec_node["label"]

    # Kill BEFORE swapping the binary in: overwriting a file that is still
    # executing gives "Text file busy" and loses the whole repetition.
    #
    # The pattern is "[.]/<bin>", not "./<bin>". `pkill -f` matches whole
    # COMMAND LINES, and the remote `bash -c "..."` running this very command
    # has "./<bin>" inside its own argv -- so the plain pattern makes pkill
    # signal its own parent shell, ssh returns 255, and the cp below never
    # runs. The bracket makes the regex still match the real server's
    # "./<bin>" while the shell's literal "[.]/<bin>" no longer contains the
    # "./" the regex needs. (pkill -x cannot be used instead: it matches comm,
    # which the kernel truncates to 15 chars, and the binary name is longer.)
    rc, _o, err = br.run_remote_cmd(
        exec_node,
        f"cd {shlex.quote(ewd)} && "
        f"pkill -f {shlex.quote('[.]/' + SRV_BIN_NAME)} 2>/dev/null; "
        f"pkill -f {shlex.quote('[.]/' + BOT_BIN_NAME)} 2>/dev/null; sleep 2; "
        f"cp -f {shlex.quote('server-' + backend)} {shlex.quote(SRV_BIN_NAME)} && "
        f"chmod +x {shlex.quote(SRV_BIN_NAME)} {shlex.quote(BOT_BIN_NAME)} && "
        f"rm -f server.log client.log", timeout=180)
    if rc != 0:
        return None, f"staging the {backend} binary failed rc={rc}: {err[:160]}"

    server_cpu, client_cpus = cpu_sets(caps)
    ssh_proc, pid, reason = br.start_server_popen(exec_node, SRV_BIN_NAME)
    if ssh_proc is None:
        return None, f"server start failed: {reason}"
    try:
        ok, reason = wait_for_bind(exec_node, pid)
        if not ok:
            return None, reason
        if server_cpu is not None:
            ok, msg = pin_server(exec_node, pid, server_cpu)
            if not ok:
                # Not fatal: the run continues UNPINNED and run_delay reports
                # the contention, exactly as on a single-core node.
                _say(label, f"could not pin the server: {msg}", bh.C_YELLOW)
                server_cpu, client_cpus = None, None

        rc, out, err = br.run_remote_cmd(
            exec_node, client_command(ewd, pid, own.bots, own.seconds,
                                      client_cpus),
            timeout=own.seconds + CLIENT_MARGIN_S)
        if rc != 0:
            return None, server_log_hint(
                exec_node, f"the client run failed on the node rc={rc}: "
                           f"{err.strip()[:200]}")

        log_text = stop_and_collect(exec_node, ssh_proc, pid)
        ssh_proc = None
    finally:
        if ssh_proc is not None:
            br.stop_server_popen(ssh_proc, exec_node, pid)

    head, _sep, client_log = out.partition("#CLIENTLOG")
    # Keep the client's own output for post-mortem: with 250 bots the reason a
    # run produced no REQ_PER_S line is in there, and the next run overwrites
    # client.log on the node.
    os.makedirs(LOG_DIR, exist_ok=True)
    local_log = os.path.join(LOG_DIR, f"{label}-{tag}.log")
    try:
        with open(local_log, "w") as fh:
            fh.write(client_log)
    except OSError as e:
        _say(label, f"could not save the client log to {local_log}: {e}",
             bh.C_YELLOW)

    m = REQ_RE.search(client_log)
    if m is None:
        rcm = re.search(r"^BOT_RC=(\d+)", head, re.M)
        return None, server_log_hint(
            exec_node,
            f"no REQ_PER_S line from {BOT_BIN_NAME} (client rc="
            f"{rcm.group(1) if rcm else '?'}, log: {local_log}). Either the "
            f"bots never reached the map, or this build has no --spam yet")
    req_per_s = float(m.group(1))
    mm = MOVES_RE.search(client_log)
    ms = SURV_RE.search(client_log)
    if mm is None or ms is None:
        return None, (f"{BOT_BIN_NAME} printed REQ_PER_S but not MOVES/"
                      f"SURVIVORS (log: {local_log})")
    moves     = int(mm.group(1))
    survivors = int(ms.group(1))
    asked     = int(ms.group(2))
    if survivors <= 0:
        return None, server_log_hint(
            exec_node, f"every bot was gone by the end of the window "
                       f"(SURVIVORS 0/{asked}, log: {local_log})")

    # Only FLOOD kicks invalidate a run: they mean the anti-DDOS, not the
    # event loop, shaped the load. A bot kicked for a bot-AI defect is
    # unrelated to the backend, so it is reported and the run is kept.
    flood_kicks = len(re.findall(r"Too many", log_text))
    other_kicks = max(0, len(re.findall(r"Kicked by", log_text)) - flood_kicks)
    if flood_kicks:
        return None, (f"{flood_kicks} FLOOD kick(s): the anti-DDOS shaped this "
                      f"run, the numbers are not comparable -- raise the DDOS "
                      f"limits in server_properties_xml()")

    samples = parse_samples(head)
    win = window_delta(samples, own.seconds)
    row = {"backend": backend, "req_per_s": req_per_s, "moves": moves,
           "survivors": survivors, "asked": asked,
           "bots": own.bots, "seconds": own.seconds,
           "cores": caps["cores"],
           "pinned": (f"server=cpu{server_cpu} client=cpu{client_cpus}"
                      if server_cpu is not None else None),
           "rss_mb": (parse_vmhwm(head) or 0) / 1024.0,
           "other_kicks": other_kicks,
           "cpu_s": None, "run_delay_s": None, "run_delay_ratio": None,
           "cpu_pct": None, "window_wall_s": None}
    if win is not None:
        wall, cpu_ns, delay_ns = win
        row["window_wall_s"] = wall
        row["cpu_s"] = cpu_ns / 1e9
        row["run_delay_s"] = delay_ns / 1e9
        row["cpu_pct"] = 100.0 * (cpu_ns / 1e9) / wall
        row["run_delay_ratio"] = (delay_ns / cpu_ns) if cpu_ns > 0 else None
    elif caps["schedstat"]:
        # schedstat exists but no usable pair of samples: say so rather than
        # silently reporting no CPU context.
        _say(label, f"{backend}: no usable schedstat sample pair "
                    f"({len(samples)} sample(s)); CPU context unavailable",
             bh.C_YELLOW)
    row.update(bench_summary(log_text))
    return row, None


# ---- deploy ---------------------------------------------------------------

_locks_guard = threading.Lock()
_locks = {}


def named_lock(key):
    """One lock per key, created on demand. Used for (a) per-compile-node
    source staging and (b) per-exec-host measurement exclusion."""
    with _locks_guard:
        lk = _locks.get(key)
        if lk is None:
            lk = threading.Lock()
            _locks[key] = lk
        return lk


_staged = {}


def stage_once(compile_node):
    """rsync the source tree to a compile node AT MOST ONCE per run, even
    when several exec nodes (or several flag sets) share that node: two
    concurrent rsyncs into the same sources/ dir would fight over the same
    files. Returns (rc, msg)."""
    label = compile_node["label"]
    with named_lock("stage:" + label):
        got = _staged.get(label)
        if got is None:
            got = br.stage_source_on_compile_node(compile_node, verbose=True)
            _staged[label] = got
        return got


def cleanup_node(exec_node):
    """Best-effort: leave no server and no client behind on the node.

    Normally both are already gone (the client is waited for, the server is
    SIGTERMed), but an ssh that timed out mid-window leaves the remote shell's
    background client orphaned -- and a leftover client would silently load the
    NEXT benchmark's node. `; true` keeps a no-match pkill (rc=1) from looking
    like a failure."""
    rc, _o, err = br.run_remote_cmd(
        exec_node,
        f"pkill -f {shlex.quote('[.]/' + SRV_BIN_NAME)} 2>/dev/null; "
        f"pkill -f {shlex.quote('[.]/' + BOT_BIN_NAME)} 2>/dev/null; true",
        timeout=60)
    if rc != 0:
        _say(exec_node["label"], f"final cleanup rc={rc}: {err[:120]}",
             bh.C_YELLOW)


def build_key(node):
    """Two exec nodes share one build only when they agree on BOTH the compile
    node AND the per-CPU compile flags -- otherwise one board's -march= would
    leak into another's binary (br.run_profiler_fleet uses the same key)."""
    flags = tuple(sorted(br.exec_node_flag_defs(br.exec_node_dict(node)).items()))
    return (node["compile_node"]["label"], flags)


def build_subdir(key, what):
    """Build dir name for one (build key, binary). The flag set is folded in
    as a short hash so two flag sets on the same compile node cannot land in
    the same dir (which is also how a stale CMakeCache would leak the wrong
    CATCHCHALLENGER_IO_URING value into the other backend)."""
    compile_label, flags = key
    base = f"{BENCH}-{what}-{compile_label}"
    if not flags:
        return base
    return f"{base}-{hashlib.sha1(repr(flags).encode()).hexdigest()[:8]}"


def build_all(key, node):
    """Phase 1 for one build key: the two server backends plus the client, all
    on the same compile node. Returns (ok, msg, {what: remote_build_dir})."""
    compile_node = node["compile_node"]
    exec_node    = br.exec_node_dict(node)
    rc, msg = stage_once(compile_node)
    if rc != 0:
        return False, f"source rsync to the compile node failed: {msg[:200]}", {}
    out = {}
    targets = (("epoll",  SRV_SRC_DIR, SERVER_DEFS),
               ("iouring", SRV_SRC_DIR, IOURING_DEFS),
               ("bot",     BOT_SRC_DIR, None))
    for what, src, defs in targets:
        print(_c(bh.C_CYAN, f"[build:{what}] on {compile_node['label']} "
                            f"({src})"), flush=True)
        rc, msg, rbuild = br.build_on_compile_node(
            compile_node, cmake_src_subdir=src,
            build_subdir=build_subdir(key, what),
            cmake_defs=defs, exec_node=exec_node, verbose=True)
        if rc != 0:
            return False, f"{what} build failed:\n{msg}", out
        out[what] = rbuild
    return True, "ok", out


def deploy_node(node, exec_node, builds):
    """Push the three binaries built for this node onto it, side by side as
    server-epoll / server-iouring / bot-bench. Returns (ok, message)."""
    compile_node = node["compile_node"]
    for backend in BACKENDS:
        rc, _bin, msg = br.push_binary_to_exec(
            compile_node, exec_node, builds[backend], SRV_BIN_NAME,
            verbose=True)
        if rc != 0:
            return False, f"{backend} push failed: {msg}"
        rc, _o, err = br.run_remote_cmd(
            exec_node,
            f"cd {shlex.quote(exec_node['work_dir'])} && "
            f"mv -f {shlex.quote(SRV_BIN_NAME)} "
            f"{shlex.quote('server-' + backend)} && "
            f"chmod +x {shlex.quote('server-' + backend)}", timeout=120)
        if rc != 0:
            return False, f"{backend} rename on the exec node failed: {err[:160]}"
    rc, _bin, msg = br.push_binary_to_exec(
        compile_node, exec_node, builds["bot"], BOT_BIN_NAME, verbose=True)
    if rc != 0:
        return False, f"{BOT_BIN_NAME} push failed: {msg}"
    rc, _o, err = br.run_remote_cmd(
        exec_node,
        f"cd {shlex.quote(exec_node['work_dir'])} && "
        f"chmod +x {shlex.quote(BOT_BIN_NAME)}", timeout=60)
    if rc != 0:
        return False, f"{BOT_BIN_NAME} chmod failed: {err[:160]}"

    # The whole comparison is void if the two server binaries are the same
    # build. build_on_compile_node reuses its build dir, so a CMakeCache left
    # over from an earlier configure can silently win over an unset option --
    # see the SERVER_DEFS comment. Cheap to check, and it fails LOUD instead
    # of publishing an overlap as "no separation between the backends".
    rc, out, err = br.run_remote_cmd(
        exec_node,
        f"cd {shlex.quote(exec_node['work_dir'])} && "
        f"sha256sum server-epoll server-iouring", timeout=120)
    if rc != 0:
        return False, f"cannot hash the deployed binaries: {err[:160]}"
    hashes = [ln.split()[0] for ln in out.strip().splitlines() if ln.strip()]
    if len(hashes) != 2:
        return False, f"expected 2 binary hashes, got: {out.strip()[:160]}"
    if hashes[0] == hashes[1]:
        return False, ("server-epoll and server-iouring are byte-identical "
                       f"({hashes[0][:16]}...): both backends built the same "
                       "way, so any comparison would be meaningless. Wipe the "
                       "build dirs on the compile node and re-run.")
    return True, "ok"


# ---- per-node A/B ---------------------------------------------------------

def measure_node(node, builds, own, deadline):
    """Full A/B on one exec node. Returns (rows, error, kind) where kind is
    "PASS", "SKIP" (infra: nothing measured, metric unknown) or "FAIL" (the
    benchmark ran and produced bad data). benchmark/CLAUDE.md: an unmeasured
    node is never a regression, so infra problems must not be reported FAIL."""
    label = node["label"]
    orig_exec_node = br.exec_node_dict(node)
    # NFS-LXC bring-up (a no-op for ordinary nodes) before any exec-node I/O.
    exec_node, prep = br.nfs_lxc_prepare(orig_exec_node, verbose=True)
    if exec_node is None:
        br.nfs_lxc_teardown(orig_exec_node, verbose=True)
        return [], f"nfs-lxc bring-up failed: {prep}", "SKIP"
    try:
        rc, msg = br.rsync_datapack_to_exec(exec_node, DATAPACK_PATH,
                                            remote_subdir="datapack",
                                            timeout=1800, server_mode=True,
                                            keep_skins=True)
        if rc != 0:
            return [], f"datapack rsync failed: {msg[:200]}", "SKIP"
        maincode = detect_maincode(DATAPACK_PATH)
        ok, msg = write_properties(
            exec_node, server_properties_xml(maincode, SERVER_PORT, own.bots))
        if not ok:
            return [], msg, "SKIP"
        ok, msg = deploy_node(node, exec_node, builds)
        if not ok:
            # A byte-identical pair is a harness/config defect, not infra:
            # FAIL it loudly so nobody reads the resulting overlap as a result.
            kind = "FAIL" if "byte-identical" in msg else "SKIP"
            return [], msg, kind
        caps, err = node_capabilities(exec_node)
        if caps is None:
            return [], err, "SKIP"

        server_cpu, client_cpus = cpu_sets(caps)
        if server_cpu is None:
            _say(label, f"{caps['cores']} core(s)"
                        + ("" if caps["taskset"] else ", no taskset")
                        + ": server and client MUST share the CPU. "
                          "run_delay (schedstat field 1) is reported per run "
                          "instead of pretending there is no contention.",
                 bh.C_YELLOW)
        else:
            _say(label, f"{caps['cores']} cores: server pinned to cpu"
                        f"{server_cpu}, client to cpu{client_cpus}", bh.C_CYAN)
        if not caps["schedstat"]:
            _say(label, "kernel has no /proc/<pid>/schedstat "
                        "(CONFIG_SCHEDSTATS off): no CPU or contention "
                        "context will be recorded", bh.C_YELLOW)
        _say(label, f"{own.bots} bots, {own.seconds}s window, {own.reps} "
                    f"rep(s) per backend, saturating (--spam)", bh.C_CYAN)

        rows = []
        rep = 1
        while rep <= own.reps:
            for backend in BACKENDS:
                if deadline is not None and time.monotonic() > deadline:
                    return rows, (f"--maxtime reached before {backend} rep{rep} "
                                  f"(partial: {len(rows)} run(s))"), "SKIP"
                row, err = run_once(exec_node, backend, own, caps,
                                    f"{backend}{rep}")
                if err is not None:
                    return rows, f"{backend} rep{rep}: {err}", "FAIL"
                row["rep"] = rep
                rows.append(row)
                pkts = row.get("packets_in")
                delay = row["run_delay_ratio"]
                tag = ""
                if delay is not None and delay >= RUN_DELAY_WARN_RATIO:
                    tag = f"  [contended: run_delay/cpu={delay:.2f}]"
                _say(label,
                     f"{backend:8s} rep{rep} "
                     f"req/s={row['req_per_s']:9.1f} "
                     f"moves={row['moves']:8d} "
                     f"surv={row['survivors']}/{row['asked']} "
                     f"cpu={_num(row['cpu_s'], '7.3f')}s "
                     f"({_num(row['cpu_pct'], '5.1f')}%) "
                     f"delay={_num(row['run_delay_s'], '7.3f')}s "
                     f"rss={row['rss_mb']:6.1f}MB "
                     f"packets_in={pkts if pkts is not None else 'n/a'}{tag}")
            rep += 1
        return rows, None, "PASS"
    finally:
        cleanup_node(exec_node)
        br.nfs_lxc_teardown(orig_exec_node, verbose=True)


# ---- verdict --------------------------------------------------------------

def summarise(rows):
    """Per-backend min/max/median of req_per_s + the contention and load
    context the verdict needs."""
    per = {}
    for backend in BACKENDS:
        mine = [r for r in rows if r["backend"] == backend]
        vals = [r["req_per_s"] for r in mine]
        if vals:
            med, std = bh.stats_of(vals)
            ratios = [r["run_delay_ratio"] for r in mine
                      if r["run_delay_ratio"] is not None]
            rmed, _rstd = bh.stats_of(ratios)
            per[backend] = {
                "n": len(vals), "min": min(vals), "max": max(vals),
                "median": med, "stddev": std, "samples": vals,
                "unit": "req/s", "better": "higher",
                "cpu_s_median": bh.stats_of([r["cpu_s"] for r in mine])[0],
                "cpu_pct_median": bh.stats_of([r["cpu_pct"] for r in mine])[0],
                "run_delay_s_median": bh.stats_of(
                    [r["run_delay_s"] for r in mine])[0],
                "run_delay_ratio_median": rmed,
                "rss_mb_median": bh.stats_of([r["rss_mb"] for r in mine])[0],
                "survivors_min": min(r["survivors"] for r in mine),
                "moves_median": bh.stats_of([r["moves"] for r in mine])[0],
                "pinned": mine[0]["pinned"],
            }
    return per


def verdict(per, bots):
    """(text, winner) from the per-backend summary. A winner needs THREE
    things, in this order:

      1. comparable load  -- both backends answered the same fleet;
      2. attributable CPU -- the node-local client did not starve the server
         so badly that the number describes the scheduler (schedstat field 1
         vs field 0), and when that cannot even be checked (no schedstat) the
         node must at least have pinned the two apart;
      3. separation       -- non-overlapping min..max ranges over the
         repetitions. A better median alone is never a result on this
         hardware."""
    if len(per) < 2:
        return "no verdict: one of the backends produced no run", None
    a, b = BACKENDS
    ranges = (f"{a} [{per[a]['min']:.1f}..{per[a]['max']:.1f}] vs "
              f"{b} [{per[b]['min']:.1f}..{per[b]['max']:.1f}] req/s")
    if len(per[a]["samples"]) < 2 or len(per[b]["samples"]) < 2:
        return ("no verdict: a single repetition per backend has no range "
                "-- re-run with --reps >= 2"), None

    caveats = []
    if abs(per[a]["survivors_min"] - per[b]["survivors_min"]) > \
            SURVIVOR_SKEW_MAX * bots:
        caveats.append(f"the backends did not carry the same load "
                       f"(survivors {per[a]['survivors_min']} vs "
                       f"{per[b]['survivors_min']} of {bots})")
    ratios = {}
    for backend in BACKENDS:
        ratio = per[backend]["run_delay_ratio_median"]
        if ratio is None:
            if per[backend]["pinned"] is None:
                caveats.append(f"{backend}: no schedstat AND no core to pin, "
                               f"so the client's interference with the server "
                               f"cannot be quantified at all")
        else:
            ratios[backend] = ratio
    # ASYMMETRIC contention is the disqualifier, not contention itself: see
    # RUN_DELAY_SKEW_MAX. Equal starvation depresses both arms alike and leaves
    # the ratio -- the thing being claimed -- intact.
    if len(ratios) == 2:
        skew = abs(ratios[a] - ratios[b])
        if skew > RUN_DELAY_SKEW_MAX:
            caveats.append(f"the client starved the two backends UNEQUALLY "
                           f"({a} x{ratios[a]:.2f} vs {b} x{ratios[b]:.2f}, "
                           f"spread {skew:.2f} > {RUN_DELAY_SKEW_MAX:.2f}), so "
                           f"they answered under different conditions and the "
                           f"delta is the scheduler's, not the event loop's")
    if caveats:
        return "no verdict: " + "; ".join(caveats) + f" ({ranges})", None

    if per[a]["min"] > per[b]["max"]:
        return (f"{a} is faster: its whole range "
                f"[{per[a]['min']:.1f}..{per[a]['max']:.1f}] req/s sits above "
                f"{b}'s [{per[b]['min']:.1f}..{per[b]['max']:.1f}]"), a
    if per[b]["min"] > per[a]["max"]:
        return (f"{b} is faster: its whole range "
                f"[{per[b]['min']:.1f}..{per[b]['max']:.1f}] req/s sits above "
                f"{a}'s [{per[a]['min']:.1f}..{per[a]['max']:.1f}]"), b
    return (f"no separation: the ranges overlap ({ranges}) -- a better median "
            f"alone is not a result, add repetitions or a longer --seconds"), None


# ---- fleet ----------------------------------------------------------------

def run_fleet(nodes, own, deadline):
    """Phase 1 (parallel builds, one per unique compile node + flag set) then
    phase 2 (parallel measurement, one worker per distinct exec host).

    Returns {label: (rows, err, kind)}. Nothing is printed through Progress in
    here: the counter is emitted serially by the caller once every node is
    done, so it needs no lock and stays in remote_nodes.json order."""
    by_key = {}
    for node in nodes:
        by_key.setdefault(build_key(node), []).append(node)

    # ---- phase 1: build --------------------------------------------------
    builds = {}

    def _build(key):
        # One node's build must never take the whole fleet down with a
        # traceback: an unexpected helper failure is reported as a build
        # failure for that key and the other keys keep going.
        try:
            return key, build_all(key, by_key[key][0])
        except Exception as e:      # noqa: BLE001 - reported, never swallowed
            return key, (False, f"build raised {type(e).__name__}: {e}", {})

    keys = list(by_key.keys())
    # Builds stay capped like br.run_profiler_fleet: several builds on ONE
    # compile node only fight for its cores, which costs build time and buys
    # no measurement. Phase 2 is the one that widens to the whole fleet.
    with cf.ThreadPoolExecutor(max_workers=min(MAX_PARALLEL_NODES,
                                               len(keys))) as ex:
        for key, res in ex.map(_build, keys):
            builds[key] = res

    # ---- phase 2: measure ------------------------------------------------
    # Distinct exec HOSTS measure concurrently; two exec entries that share
    # one physical host must not, since they would fight for the same CPU and
    # (worse) kill each other's server through the shared pkill/port. A
    # per-host lock serialises only those.
    results = {}
    results_lock = threading.Lock()

    def _measure(node):
        label = node["label"]
        ok, msg, bld = builds.get(build_key(node),
                                  (False, "no build result", {}))
        if not ok:
            # A compile-node build failure never touches the exec node: the
            # metric is UNKNOWN, which the decision matrix must not read as a
            # regression -> SKIP, not FAIL.
            with results_lock:
                results[label] = ([], f"compile-node build failed: {msg}",
                                  "SKIP")
            return
        if deadline is not None and time.monotonic() > deadline:
            with results_lock:
                results[label] = ([], "not run: --maxtime reached", "SKIP")
            return
        host = node.get("ssh_host") or label
        try:
            with named_lock("host:" + str(host)):
                rows, err, kind = measure_node(node, bld, own, deadline)
        except Exception as e:      # noqa: BLE001 - reported, never swallowed
            rows, err, kind = [], (f"the node measurement raised "
                                   f"{type(e).__name__}: {e}"), "SKIP"
        with results_lock:
            results[label] = (rows, err, kind)

    with cf.ThreadPoolExecutor(max_workers=min(MAX_PARALLEL_NODES,
                                               len(nodes))) as ex:
        list(ex.map(_measure, nodes))
    return results


# ---- main -----------------------------------------------------------------

def main():
    own, args = parse_args()
    bh.set_node_filter(args.node)
    br.set_benchmark_label(BENCH)

    if args.profile:
        print(_c(bh.C_RED, "[fatal] --profile is not supported here: a "
                           "profiler multiplies the very CPU time the two "
                           "backends are compared on. Profile one backend "
                           "with the other benchmarks instead."))
        return 2
    if not os.path.isdir(DATAPACK_PATH):
        print(_c(bh.C_RED, f"[fatal] datapack not found: {DATAPACK_PATH} "
                           f"(set CC_DATAPACK)"))
        return 2

    nodes = [n for n in bh.benchmark_exec_nodes() if n.get("compile_node")]
    if not nodes:
        print(_c(bh.C_RED,
                 "[fatal] no benchmark execution node selected or available.\n"
                 "  This A/B only runs on a constrained exec node from "
                 "remote_nodes.json (out of the repo root): the server AND "
                 "the measuring client both run ON that node, and a\n"
                 "  workstation answers a bot fleet from cache with both "
                 "backends idle -- so its numbers say nothing about either. "
                 "There is deliberately no built-in fallback host.\n"
                 "  Select one with --node <label|arch>, and check that the "
                 "node is enabled, benchmark:true, has a parent compile node, "
                 "and is under the loadavg gate."))
        return 2

    # ONE machine-wide flock for the whole run, taken once: it serialises this
    # benchmark against the OTHER network benchmarks. It is NOT re-acquired per
    # node -- it would deadlock nothing but would also serialise nothing
    # useful, since each node now drives its own loopback.
    lock = bh.acquire_network_lock(BENCH)
    try:
        deadline = (time.monotonic() + args.maxtime) if args.maxtime else None
        print(_c(bh.C_CYAN, f"[fleet] {len(nodes)} node(s), up to "
                            f"{MAX_PARALLEL_NODES} in parallel"))
        results = run_fleet(nodes, own, deadline)

        progress = bh.Progress(len(nodes), BENCH)
        stamp = time.strftime("%Y-%m-%dT%H-%M-%S", time.gmtime())
        failures = []
        measured = 0
        print()
        for node in nodes:
            label = node["label"]
            rows, err, kind = results.get(
                label, ([], "no result recorded for this node", "SKIP"))
            if err is not None:
                bh.print_node_error(BENCH, label, kind, err)
                progress.emit("ab", "no", label, status=kind,
                              extra=err.splitlines()[0][:80])
                failures.append((label, kind, err))
            else:
                progress.emit("ab", "no", label, status="PASS")
            if not rows:
                continue
            measured += 1
            per = summarise(rows)
            text, winner = verdict(per, own.bots)
            print(_c(bh.C_BOLD,
                     f"[{label}] epoll vs io_uring at saturation "
                     f"({own.bots} bots, {own.seconds}s window, "
                     f"{rows[0]['cores']} core(s), "
                     f"pinned: {rows[0]['pinned'] or 'no'})"))
            for backend in BACKENDS:
                s = per.get(backend)
                if s:
                    print(f"  {backend:8s} n={s['n']} "
                          f"median={s['median']:9.1f} "
                          f"min={s['min']:9.1f} max={s['max']:9.1f} "
                          f"stddev={s['stddev']:8.1f} req/s | "
                          f"cpu={_num(s['cpu_s_median'], '7.3f')}s "
                          f"({_num(s['cpu_pct_median'], '5.1f')}%) "
                          f"run_delay={_num(s['run_delay_s_median'], '7.3f')}s "
                          f"(x{_num(s['run_delay_ratio_median'], '4.2f')}) "
                          f"rss={_num(s['rss_mb_median'], '6.1f')}MB")
            print(_c(bh.C_GREEN if winner else bh.C_YELLOW, f"  => {text}"))
            compile_label, exec_label = bh.node_path_parts(label)
            out_path = os.path.join(bh.RESULTS, BENCH, compile_label,
                                    exec_label, f"ab-{stamp}.json")
            bh.write_record(out_path, {
                "benchmark": BENCH, "commit": bh.git_sha(),
                "comment": args.comment, "date": stamp, "node": label,
                "arch": node.get("arch", "?"),
                "metric": "req_per_s", "better": "higher",
                "bots": own.bots, "seconds": own.seconds, "reps": own.reps,
                "cores": rows[0]["cores"], "pinned": rows[0]["pinned"],
                "rows": rows, "summary": per,
                "verdict": text, "winner": winner,
            })
            print(_c(bh.C_CYAN, f"  wrote {out_path}"))

        if failures:
            print()
            for label, kind, err in failures:
                print(_c(bh.C_RED if kind == "FAIL" else bh.C_YELLOW,
                         f"[{label}] {kind}: {err.splitlines()[0][:200]}"))
        if not measured:
            print(_c(bh.C_RED, "[fatal] no node produced a measurement"))
            return 1
        # A SKIPped node is an unknown metric, not a regression: only a node
        # that ran and produced bad data makes the whole run non-zero.
        if any(kind == "FAIL" for _l, kind, _e in failures):
            return 1
        return 0
    finally:
        del lock


if __name__ == "__main__":
    sys.exit(main())
