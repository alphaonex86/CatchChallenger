#!/usr/bin/env python3
"""benchmarkmapmanager2.py -- min_balanced() over a per-node generated replay.

Two stages, because the workload and the measurement want opposite things.

STAGE 1 runs HERE, on the orchestrating host, once per execution node
(benchmarkmapmanager2/stage1). It reads the datapack's real world with the
PRODUCTION loader -- every .tmx under map/main/generated, 647 real maps --
asks the node how much memory it has and decides how many players it can
hold, spreads them 60/30/10 over route/town/interior maps crowded to the
owner's 35/200/20 per map (253 hard guard), and then SIMULATES their walk:
every movement vector is truncated at the first obstacle by the production
predicate MoveOnTheMap::isWalkableWithDirection. What comes out is a C++
source: the map dimensions, where each player starts, and the vectors they
will replay.

STAGE 2 is what gets measured (benchmarkmapmanager2/stage2). It compiles
that file in and replays it -- no datapack, no map files, no collision test,
nothing read at runtime. Per player per tick it costs a countdown and a
coordinate store: a vector saying "3 cells" is walked one cell per tick for
3 ticks, and only then is the next vector fetched. So the run measures
min_balanced and the tick loop around it, not a client simulation. It also
means the benchmark fits a board with no filesystem, which is the only way
the ESP32 can ever run it.

Both stages are therefore specific to the execution node AND to the input
datapack, and the harness rebuilds stage 2 per node for exactly that reason.

The workload is FIXED: nothing about it can be changed from a command line.
Each run records what it replayed (world_maps, workload_players,
workload_cycle_ticks) so a regenerated map set, a different datapack or a
node whose RAM changed shows up in the timeline instead of quietly shifting
every timing.

When the replay reaches its end everything resets -- every player home,
every list rewound -- and the same window replays. Stage 1 sizes the list so
that stays rare (a reset is one tick where the whole world jumps, and enough
of them poison the tail) without baking megabytes into a small board's
binary; `resets` is recorded so the cost is never invisible. After a full
cycle stage 2 hashes its state and compares it with what stage 1 computed:
`replay_mismatch` must be 0, which also proves no replayed vector walked
into a wall.

Player counts per node: the generated population, a quarter of it and a
sixteenth -- the large/medium/small of THAT node, all out of the same
generated set, since a prefix of the players is a valid workload on its own.

One-command target -- per benchmark/CLAUDE.md, run with no args, 1h
timeout. Builds stage 1, generates a workload per node, builds stage 2 for
each (on its compile parent, the workload rsync'd there first), runs every
available profiler (rusage via /usr/bin/time -v, perf stat, callgrind,
binary-size), prints a one-line progress update per cell, records
per-platform history + charts, and applies the KEEP/DISCARD/ESCALATE matrix
against benchmark/results/benchmarkmapmanager2/champion.json.
"""
import re
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
STAGE1_SRC = os.path.join(BENCH_DIR, "benchmarkmapmanager2", "stage1")
STAGE2_SRC = os.path.join(BENCH_DIR, "benchmarkmapmanager2", "stage2")
STAGE1_BIN = "benchmark_world_stage1"
# Was benchmark_min_network* until 2026-08-21: it measures min_balanced(), and
# that method was itself called min_network() until the view-range algorithm
# took the name (mapVisibility/minimize "network"). Renaming the binary orphans
# nothing -- the history is keyed by SCRIPT name (benchmark/history/<script>/)
# and by metric, never by the binary file name.
BIN_NAME   = "benchmark_min_balanced_replay"
# A rusage cell replays the three mapVisibility/minimize strategies inside ONE
# process and reports them on ONE BENCH line, so they are measured on the same
# machine, in the same conditions, in the same run -- the only way their
# trade-off is a measurement and not an argument. One process matters for the
# fleet too: an exec node is sent a SINGLE command per cell, so a strategy that
# needed its own invocation would never be measured anywhere but locally.
# The plain metric names stay "balanced" (what every series recorded before the
# other two existed measured, so the history is comparable), the others are
# prefixed:
#   net_<metric>     min_network()  view range, border maps
#   mincpu_<metric>  min_CPU()      whole map, no state
# It TRIPLES the wall time of a rusage cell; empty the tuple to go back to
# balanced-only (the binary then still emits the three -- pass --algo balanced
# to make it replay one).
EXTRA_ALGOS = (("net_", "network"), ("mincpu_", "cpu"))

# Concurrency marker: pure in-process visibility loop, no port bind / no
# network (the binary name is historical). Safe to run in parallel.
NETWORK_EXCLUSIVE = False

# Build dir lives outside the source tree (root CLAUDE.md).
try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
except Exception:
    BUILD_ROOT = os.path.join("/tmp", "cc-build")
BUILD_DIR   = os.path.join(BUILD_ROOT, "benchmark", "benchmarkmapmanager2")
STAGE1_BUILD = os.path.join(BUILD_DIR, "stage1")
# Generated workloads: one per (execution node, datapack). They are build
# INPUTS, not sources -- they never enter the repo.
WORKLOAD_DIR = os.path.join(BUILD_DIR, "workload")

# The world stage 1 reads. Same constant as the other benchmarks here. Only
# stage 1 ever touches it: the measured binary carries its world compiled in,
# so no exec node needs a datapack at all.
DATAPACK_PATH = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
# Where a node's generated workload is parked on its compile node (out of the
# rsync'd source tree, which is mirrored with --delete).
REMOTE_WORKLOAD_SUBDIR = "bench-workloads"

# THE WORKLOAD IS FIXED and lives in the BINARY: stage 1 generates it (which
# maps exist, how many players, where they start, every movement vector) and
# stage 2 compiles it in, so there is nothing to read or decide at runtime and
# nothing a flag could change. Each node gets its own -- its RAM sets the
# population -- which is the point: 5000 players says nothing about a 52 MB
# Pentium MMX and nothing about a 128 GB host either.
# ---- How many players a node is given -------------------------------------
# Stage 1 decides the population from the node's RAM, because that is what
# actually limits it. MEASURED on stage 2 over this world: 1250 players ->
# 6.1 MB, 5000 -> 10.0 MB, 20000 -> 43.4 MB, 65530 -> 202 MB. The slope is not
# constant -- ~1.0 KB per player while the maps are thinly populated, ~3.0 KB
# once the population overflows the target crowds and the maps pack toward 253,
# because the packets each broadcast captures grow with the crowd. Size on the
# WORST slope: over-estimating costs a node some players, under-estimating
# OOM-kills it.
PER_PLAYER_KB   = 3.0
BASE_RSS_MB     = 10        # binary + world tables + allocator, with slack
# Only ever take half of a node's RAM: it has a kernel, an NFS root and a shell
# to keep alive, and a benchmark that OOM-kills its node measures nothing.
RAM_FRACTION    = 0.5
# The connected-player index on the wire is 16-bit (65535 is the empty marker).
MAX_PLAYERS     = 65530
MIN_PLAYERS     = 50


def _reference_players():
    """The lowest rung of stage 2's fleet ladder, read from stage 2 rather than
    duplicated here: it is the C++ that acts on it, and two copies of a number
    that MUST match is how a fleet ends up comparing two different loads.

    It is the smallest cell of any node's sweep, which makes it what callgrind
    (~30x slower) runs -- so instruction counts compare across machines too."""
    src = os.path.join(STAGE2_SRC, "main.cpp")
    try:
        with open(src, "r") as handle:
            found = re.search(r"^#define\s+REFERENCE_PLAYERS\s+(\d+)", handle.read(),
                              re.MULTILINE)
        if found is not None:
            return int(found.group(1))
        print("[warn] stage2 defines no REFERENCE_PLAYERS; the sweep still runs "
              "but no cell is comparable across nodes")
    except OSError as e:
        print(f"[warn] cannot read stage 2 to learn REFERENCE_PLAYERS: {e}")
    return 0


REFERENCE_PLAYERS = _reference_players()
# A node we cannot ask right now (unreachable, mid-reboot): fall back to what
# the fleet recorded about it, and failing that assume a small board rather
# than a workstation, so a first run cannot swamp it.
DEFAULT_RAM_MB  = 64
# The ESP32 has no /proc to ask: 520 KB of SRAM, of which roughly 300 KB is
# free heap once the firmware and its network stack are up. Its population is
# a constant for the same reason -- the per-player cost measured on Linux says
# nothing about a board whose whole world lives in flash.
ESP32_RAM_KB        = 300
ESP32_PLAYERS       = 60
ESP32_REPLAY_BYTES  = 32 * 1024     # flash, not RAM
# Byte budget for the replay table baked into stage 2. It decides how long the
# replay runs before it loops: too small and the run is mostly resets (every
# reset is a tick where the whole world jumps home, and enough of them poison
# the tail latency), too large and the board has to carry a table it does not
# need -- an ESP32 has little room for software. Scale it with the node: 2% of
# its RAM, floored so even a tiny board gets a usable replay and capped so a
# big one does not bake in megabytes it will never replay.
REPLAY_RAM_FRACTION = 0.02
REPLAY_BYTES_MIN    = 64 * 1024
REPLAY_BYTES_MAX    = 4 * 1024 * 1024
# Below this many ticks per replay loop, resets stop being a rounding error.
SHORT_CYCLE_TICKS   = 100

RUN_REPEATS   = 3        # warmup + 3 measured wall passes per cell

# Fixed-TIME model (benchmark/CLAUDE.md): the wall-time / throughput
# profilers (rusage, perf-stat) run each player-count for a fixed budget
# and report ticks completed (higher-is-better) -- NOT a fixed tick count
# timed to completion. Per-player-count budget in ms; total binary wall =
# MAP_BENCH_MS * (number of player counts).
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
# not startup). min_balanced is the measured hot path and is never inlined.
MAP_CALLGRIND_TOGGLE = "*min_balanced*"

# Timeout DERIVED from the budget (timeout = budget + margin), not picked
# independently. The binary self-stops at the budget; the timeout is only
# the hard backstop for a hung run, so it must exceed total budget wall.
# 3 cells per run (this node's small / medium / large), and each cell replays
# the 3 strategies back to back (one invocation, one BENCH line), so the budget
# wall is 3 x 3 x MAP_BENCH_MS.
_BUDGET_WALL_S = (MAP_BENCH_MS / 1000.0) * 3 * (1 + len(EXTRA_ALGOS))
RUN_TIMEOUT    = int(_BUDGET_WALL_S * 3) + 120   # margin: warmup+startup+repeats
# Callgrind inflates wall ~30x; scale its timeout by the same factor so it
# measures the same WORK (its tick count is fixed, see MAP_CALLGRIND_TICKS).
# It profiles ONE strategy (--algo, see run_one), so it keeps the single-pass
# budget as its base instead of the tripled one.
RUN_TIMEOUT_CALLGRIND = (int((MAP_BENCH_MS / 1000.0) * 3 * 3) + 120) * 30

def _color(c, s): return f"{c}{s}{bh.C_RESET}"


def _meminfo_mb(text):
    """MemTotal out of /proc/meminfo, in MB."""
    for line in (text or "").splitlines():
        if line.startswith("MemTotal:"):
            try:
                return int(line.split()[1]) // 1024
            except (IndexError, ValueError):
                return None
    return None


def _recorded_field(label, field):
    """A platform field the fleet recorded for this node in an earlier run."""
    root = os.path.join(BENCH_DIR, "history")
    if not os.path.isdir(root):
        return None
    for bench in sorted(os.listdir(root)):
        bdir = os.path.join(root, bench)
        if not os.path.isdir(bdir):
            continue
        for compile_node in sorted(os.listdir(bdir)):
            pf = os.path.join(bdir, compile_node, label, "platform.json")
            if not os.path.isfile(pf):
                continue
            try:
                with open(pf, "r", encoding="utf-8") as f:
                    d = json.load(f)
            except (OSError, ValueError):
                continue
            got = (d.get("platform") or d).get(field)
            if got:
                return got
    return None


def node_identity_ok(exec_node):
    """Is the machine answering this label's address still that machine?

    An address outlives the box behind it: a board goes down, DHCP hands its
    lease to another, and the fleet then measures the WRONG machine under the
    old label -- silently, since every number looks plausible. It happened on
    this very run: p1mmx (a 52 MB Pentium MMX) answered as a 1 GB Atom N270,
    which would have written Atom results into the Pentium's timeline and
    compared them against it forever after.

    Compares the CPU model the node reports now with the one the fleet
    recorded for that label. Unknown either side -> allowed (a node we have
    never recorded has nothing to contradict). Returns (ok, message)."""
    label = exec_node["label"]
    was = _recorded_field(label, "cpu_model")
    if not was:
        return True, "no recorded identity to compare"
    rc, out, _err = br.ssh_run(exec_node.get("user"), exec_node.get("host"),
                               exec_node.get("port", 22),
                               "grep -m1 -E '^(model name|Model|Processor)' /proc/cpuinfo",
                               timeout=30)
    if rc != 0 or not out.strip():
        return True, "node did not answer the identity probe"
    now = out.split(":", 1)[-1].strip() if ":" in out else out.strip()
    # Compare loosely: the exact string varies with kernel version.
    if now[:24].lower() == str(was)[:24].lower():
        return True, "identity matches"
    return False, (f"this address now answers as {now!r}, but {label!r} was "
                   f"recorded as {was!r} -- the label points at a different "
                   f"machine (an address outliving its board), so anything "
                   f"measured here would be filed under the wrong node")


def _recorded_ram_mb(label):
    """Last resort: the RAM the fleet recorded for this node in an earlier
    run. Only used when the node cannot be asked right now."""
    best = None
    root = os.path.join(BENCH_DIR, "history")
    if os.path.isdir(root):
        for bench in os.listdir(root):
            bdir = os.path.join(root, bench)
            if not os.path.isdir(bdir):
                continue
            for compile_node in os.listdir(bdir):
                pf = os.path.join(bdir, compile_node, label, "platform.json")
                if not os.path.isfile(pf):
                    continue
                try:
                    with open(pf, "r", encoding="utf-8") as f:
                        d = json.load(f)
                except (OSError, ValueError):
                    continue
                got = (d.get("platform") or d).get("ram_total_mb")
                if isinstance(got, int) and got > 0 and (best is None or got > best):
                    best = got
    return best


_RAM_MB_CACHE = {}


def node_ram_mb(node):
    """ASK the node how much memory it has. Every node in this fleet runs
    Linux, so /proc/meminfo is the answer everywhere -- reading it now beats
    trusting a number recorded weeks ago, and it is the only way a board that
    gained or lost RAM is noticed at all.

    `node` is either the exec-node dict or a bare label for the host. The
    diskless-LXC nodes are brought up for the question and put back after;
    that is a no-op for every other node. The ESP32 has no /proc at all, so
    its figure is a constant (see ESP32_RAM_KB)."""
    label = node["label"] if isinstance(node, dict) else node
    if label in _RAM_MB_CACHE:
        return _RAM_MB_CACHE[label]
    got = _probe_ram_mb(node, label)
    _RAM_MB_CACHE[label] = got
    return got


def _probe_ram_mb(node, label):
    if label == "local":
        try:
            with open("/proc/meminfo", "r", encoding="utf-8") as f:
                got = _meminfo_mb(f.read())
        except OSError:
            got = None
        return got if got else DEFAULT_RAM_MB
    if isinstance(node, dict) and node.get("esp32"):
        return ESP32_RAM_KB // 1024
    if isinstance(node, dict) and node.get("host"):
        runtime, msg = br.nfs_lxc_prepare(node, verbose=False)
        if runtime is not None:
            try:
                rc, out, err = br.ssh_run(runtime.get("user"), runtime.get("host"),
                                          runtime.get("port", 22),
                                          "cat /proc/meminfo", timeout=60)
                if rc == 0:
                    got = _meminfo_mb(out)
                    if got:
                        return got
            finally:
                br.nfs_lxc_teardown(node, verbose=False)
        else:
            br.nfs_lxc_teardown(node, verbose=False)
    got = _recorded_ram_mb(label)
    return got if got else DEFAULT_RAM_MB


def node_replay_bytes(node):
    """How many bytes of replay table this node may carry. On the ESP32 the
    table lives in FLASH, not in its 300 KB of RAM, so its budget is a
    constant of the board rather than a fraction of anything."""
    if isinstance(node, dict) and node.get("esp32"):
        return ESP32_REPLAY_BYTES
    n = int(node_ram_mb(node) * 1024 * 1024 * REPLAY_RAM_FRACTION)
    if n < REPLAY_BYTES_MIN: n = REPLAY_BYTES_MIN
    if n > REPLAY_BYTES_MAX: n = REPLAY_BYTES_MAX
    return n


def node_max_players(node):
    """How many players this node's RAM allows. Stage 1 clamps it further to
    what the world itself can hold at 253 players per map."""
    if isinstance(node, dict) and node.get("esp32"):
        return ESP32_PLAYERS
    ram = node_ram_mb(node)
    usable = ram * RAM_FRACTION - BASE_RSS_MB
    if usable <= 0:
        return MIN_PLAYERS
    n = int(usable * 1024 / PER_PLAYER_KB)
    if n > MAX_PLAYERS: n = MAX_PLAYERS
    if n < MIN_PLAYERS: n = MIN_PLAYERS
    return n


def build_stage1():
    """Build the generator. It runs HERE, on the orchestrating host: it needs
    the datapack, and nothing it does is measured."""
    os.makedirs(STAGE1_BUILD, exist_ok=True)
    print(_color(bh.C_CYAN, f"[stage1] build {STAGE1_SRC}"))
    bh.drop_stale_cmake_cache(STAGE1_BUILD, STAGE1_SRC)
    cfg = ["cmake", "-S", STAGE1_SRC, "-B", STAGE1_BUILD,
           "-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    cfg += bh.cmake_accel_defs()
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=300)
    if rc != 0:
        bh.print_local_build_error("stage1", "cmake configure", sout, serr)
        return None
    bld = ["cmake", "--build", STAGE1_BUILD, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=900)
    if rc != 0:
        bh.print_local_build_error("stage1", "cmake build", sout, serr)
        return None
    path = os.path.join(STAGE1_BUILD, STAGE1_BIN)
    if not os.path.isfile(path):
        print(_color(bh.C_RED, f"[stage1] missing binary: {path}"))
        return None
    return path


NODE_SMALL_CELL = {}     # label -> the small cell of that node's sweep


def generate_workload(stage1_bin, node):
    """Run stage 1 for one node: read the world, size the population from that
    node's RAM, and write the .cpp stage 2 will compile in.

    Returns (workload_path, info_dict) or (None, error_string)."""
    label = node["label"] if isinstance(node, dict) else node
    os.makedirs(WORKLOAD_DIR, exist_ok=True)
    out = os.path.join(WORKLOAD_DIR, f"workload-{label}.cpp")
    cmd = [stage1_bin, "--datapack", DATAPACK_PATH, "--node", label,
           "--max-players", str(node_max_players(node)),
           "--replay-bytes", str(node_replay_bytes(node)), "--out", out]
    rc, sout, serr, _ = bh.run_capture(cmd, timeout=900)
    if rc != 0 or not os.path.isfile(out):
        return None, f"stage1 rc={rc}: {(serr or sout or '').strip()[:400]}"
    info = {}
    for line in sout.splitlines():
        if line.startswith("STAGE1 "):
            for kv in line[len("STAGE1 "):].split():
                k, _, v = kv.partition("=")
                try: info[k] = int(v)
                except ValueError: info[k] = v
    print(_color(bh.C_CYAN,
          f"[stage1] {label}: {info.get('players', '?')} players "
          f"({node_ram_mb(node)} MB RAM), "
          f"{info.get('maps', '?')} maps, cycle {info.get('cycle_ticks', '?')} ticks, "
          f"{info.get('entries_per_player', '?')} vectors each, "
          f"{info.get('workload_bytes', 0) // 1024} KiB"))
    # A short cycle means the world jumps home often, and enough of that shows
    # up in the tail. It happens when the population is so large that the byte
    # budget only buys the minimum list; say so rather than let the p95 drift.
    if info.get("players"):
        # callgrind (~30x slower) runs the smallest cell of the sweep, which is
        # now the fleet-wide reference: its instruction count then compares
        # across machines directly, since every node counted the same work.
        players = int(info["players"])
        NODE_SMALL_CELL[label] = min(REFERENCE_PLAYERS, players) \
                                 if REFERENCE_PLAYERS else max(1, players // 16)
    cycle = info.get("cycle_ticks") or 0
    if cycle and cycle < SHORT_CYCLE_TICKS:
        print(_color(bh.C_YELLOW,
              f"[stage1] {label}: the replay loops every {cycle} ticks -- that "
              f"is {info.get('players', '?')} players against a "
              f"{node_replay_bytes(node) // 1024} KiB budget. Resets will be a "
              f"visible share of the run; check `resets` before reading p95."))
    return out, info


def build_stage2(workload_cpp, build_dir):
    """Build the measured binary with that workload compiled in."""
    os.makedirs(build_dir, exist_ok=True)
    bh.drop_stale_cmake_cache(build_dir, STAGE2_SRC)
    cfg = ["cmake", "-S", STAGE2_SRC, "-B", build_dir,
           "-DCMAKE_BUILD_TYPE=Release", f"-DCC_WORKLOAD_CPP={workload_cpp}"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    cfg += bh.cmake_accel_defs()
    rc, sout, serr, _ = bh.run_capture(cfg, timeout=300)
    if rc != 0:
        bh.print_local_build_error("stage2", "cmake configure", sout, serr)
        return None
    bh.record_libs("local", sout)
    bld = ["cmake", "--build", build_dir, "--", "-j", str(os.cpu_count() or 1)]
    rc, sout, serr, _ = bh.run_capture(bld, timeout=1800)
    if rc != 0:
        bh.print_local_build_error("stage2", "cmake build", sout, serr)
        return None
    path = os.path.join(build_dir, BIN_NAME)
    if not os.path.isfile(path):
        print(_color(bh.C_RED, f"[stage2] missing binary: {path}"))
        return None
    print(_color(bh.C_GREEN, f"[stage2] OK ({bh.binary_size(path)} bytes)"))
    return path


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


def parse_workload_line(stdout):
    """Parse stage 2's one-off `WORKLOAD node=... players=...` line.

    The workload is what stage 1 generated for this node and this datapack, so
    its shape is recorded with every run: a regenerated map set, a different
    datapack or a node whose RAM changed then shows up as a changed
    world_maps / workload_players in the timeline instead of quietly shifting
    every timing below it."""
    for line in stdout.splitlines():
        if line.startswith("WORKLOAD "):
            f = {}
            for kv in line[len("WORKLOAD "):].split():
                k, _, v = kv.partition("=")
                try: f[k] = int(v)
                except ValueError: f[k] = v
            return f
    return {}


def _bench_to_cell(bench, cell):
    """Map one parsed BENCH block into the {(player, metric): value} cell.

    ONE line carries the three strategies: the unprefixed fields are
    min_balanced (so every series recorded before the other two existed keeps
    its name), net_* is min_network and mincpu_* is min_CPU. The world facts
    (maps, changed_pct) describe the replay itself and are written once.

    Returns an error string when the binary's own oracle failed, else None:
    replay_mismatch != 0 means stage 2 did not end a cycle where stage 1 said
    it would, so the two stages disagree about the workload -- bad data, which
    is the one thing that IS a FAIL (benchmark/CLAUDE.md)."""
    for p, fields in bench.items():
        for pre in ("",) + tuple(_p for _p, _a in EXTRA_ALGOS):
            if fields.get(pre + "ticks") is None:
                # Strategy not in this line (a --algo-pinned run measures one).
                continue
            bad = fields.get(pre + "replay_mismatch")
            if bad:
                return (f"{pre}replay_mismatch={bad} at players={p}: after a "
                        f"full cycle the replayed state does not match what "
                        f"stage 1 computed, so the two stages disagree and the "
                        f"data is invalid")
            # Fixed-time headline: throughput (ticks/s, higher-is-better) + the
            # work done (ticks completed). Latency stays per-tick.
            cell[(p, pre + "ticks_per_s")]    = fields.get(pre + "ticks_per_s")
            cell[(p, pre + "ticks")]          = fields.get(pre + "ticks")
            # Measured by the binary itself (getrusage over the timed window),
            # so no extra run per slice just to wrap it in /usr/bin/time.
            cell[(p, pre + "cpu_percent")]    = fields.get(pre + "cpu_percent")
            cell[(p, pre + "median_tick_ns")] = fields.get(pre + "median_tick_ns")
            cell[(p, pre + "p95_tick_ns")]    = fields.get(pre + "p95_tick_ns")
            # Bytes PER TICK, not the run's total: the total is proportional to
            # the ticks completed, so a FASTER binary sends more of them and a
            # real speed-up would read as a byte regression (and turn a KEEP
            # into an ESCALATE). What a strategy is judged on is how much it
            # puts on the wire per broadcast.
            ticks = fields.get(pre + "ticks")
            sent  = fields.get(pre + "bytes_sent")
            if ticks and sent is not None:
                cell[(p, pre + "bytes_per_tick")] = round(float(sent) / ticks, 2)
            cell[(p, pre + "bytes_sent")] = sent
            # Resident state the algorithm keeps between ticks to know what it
            # last broadcast -- the memory side of the same trade-off.
            cell[(p, pre + "visibility_state_bytes")] = \
                fields.get(pre + "visibility_state_bytes")
            # How often the replay looped back to its first vector. A reset is
            # one tick where every player jumps home, so it must stay a small
            # fraction of the run -- and it is only invisible if nobody looks.
            cell[(p, pre + "resets")]         = fields.get(pre + "resets")
            # Harness cost per tick. NOT server work and NOT inside the latency
            # window: recorded so a reviewer can check it stayed a fraction of
            # median_tick_ns instead of taking that on trust.
            cell[(p, pre + "median_prep_ns")] = fields.get(pre + "median_prep_ns")
            # ---- normalised, so two machines compare even at different sizes
            # Each node's population comes from its own RAM, so its p<N>_ cells
            # carry a different N and the raw times cannot be put side by side:
            # a 56 ms tick over 5461 players is faster work than a 20 ms tick
            # over 1000. These three divide the load back out, and they are what
            # answers "which machine is quicker" across the fleet without first
            # picking a common population.
            #
            # (The REFERENCE cell -- 1000 players on every node -- answers the
            # same question the other way, by making the load identical instead
            # of dividing it out. Both are recorded: the reference cell is exact
            # but covers one size only, these cover every cell but assume the
            # cost is roughly linear in the population, which the fixed 647-map
            # floor makes only approximately true at small N.)
            if p:
                if fields.get(pre + "ticks_per_s") is not None:
                    # Player-broadcasts per second: the work rate itself. This
                    # is the one to rank hardware on -- it rises both with the
                    # machine's speed and with the population it can hold,
                    # which is exactly what a bigger box buys.
                    cell[(p, pre + "player_ticks_per_s")] = round(
                        float(fields[pre + "ticks_per_s"]) * p, 1)
                if fields.get(pre + "median_tick_ns") is not None:
                    # Cost of one player in one broadcast. Falls as the
                    # population grows (the per-map floor amortises), so read it
                    # between machines at similar sizes, or on the reference
                    # cell.
                    cell[(p, pre + "ns_per_player")] = round(
                        float(fields[pre + "median_tick_ns"]) / p, 1)
                per_tick = cell.get((p, pre + "bytes_per_tick"))
                if per_tick is not None:
                    cell[(p, pre + "bytes_per_player_tick")] = round(
                        float(per_tick) / p, 2)
        # ---- world facts: one replay, so they are the same for the three ----
        # How many maps this population spread over -- the world shape is
        # derived from the population, so record it with the numbers it
        # explains rather than leaving a reader to recompute it.
        cell[(p, "maps")] = fields.get("maps")
        # Share of slots that differ from the previous broadcast. This is the
        # workload's defining property: 100 - changed_pct is what min_balanced's
        # stateful diff gets to SKIP, and an optimisation of that path is only
        # measurable while this stays well under 100.
        slots   = fields.get("sampled_slots")
        changed = fields.get("sampled_changed")
        if slots:
            cell[(p, "changed_pct")] = round(100.0 * changed / slots, 2)
    return None


def _mode_args(profiler):
    """Per-profiler workload-size args. Throughput profilers use the
    fixed-TIME budget (--ms); callgrind uses a fixed deterministic tick
    count (--ticks)."""
    if profiler == "callgrind":
        return ["--ticks", str(MAP_CALLGRIND_TICKS)]
    return ["--ms", str(MAP_BENCH_MS)]


def run_one(bin_path, profiler="rusage", players_arg=None, algo=None):
    """algo=None replays the three strategies in one process (rusage: the
    BENCH line then carries all of them). Every other profiler measures the
    PROCESS, so it has to see exactly one strategy or the counters would be a
    blend of the three -- they pin min_balanced, the historical series."""
    cmd = [bin_path, *_mode_args(profiler)]
    if algo is None and profiler != "rusage":
        algo = "balanced"
    if algo is not None:
        cmd += ["--algo", algo]
    if profiler == "callgrind" and players_arg is None and NODE_SMALL_CELL.get("local"):
        players_arg = [NODE_SMALL_CELL["local"]]
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
        # One warmup pass (dropped: cold page cache / cold branch predictors)
        # then RUN_REPEATS measured passes, and every measured pass is KEPT.
        # The decision matrix calls a move real only when it clears the noise
        # band, and a noise band needs more than one sample -- with a single
        # pass stddev is 0 and every delta looks significant.
        for i in range(RUN_REPEATS + 1):
            rc, sout, serr, dt = bh.run_capture(cmd, timeout=timeout,
                                                cwd=run_cwd,
                                                preexec_fn=bh._drop_core_rlimit)
            if rc != 0:
                return None, f"benchmark binary exited with code {rc}"
            if i == 0:
                continue
            one = {}
            wl = parse_workload_line(sout)
            if wl.get("maps"):
                one[(0, "world_maps")]            = wl["maps"]
                one[(0, "workload_players")]      = wl.get("players")
                one[(0, "workload_cycle_ticks")]  = wl.get("cycle_ticks")
                one[(0, "workload_entries")]      = wl.get("entries_per_player")
                one[(0, "workload_replay_bytes")] = wl.get("replay_bytes")
            bad = _bench_to_cell(parse_bench_lines(sout), one)
            if bad is not None:
                return None, bad
            if not one:
                return None, "no BENCH line parsed from the benchmark binary"
            for key, value in one.items():
                if value is not None:
                    metrics.setdefault(key, []).append(value)
        # Peak RSS + wall come from one extra pass under /usr/bin/time -v
        # (it swallows the child's stdout, so it cannot double as a sample).
        t = bh.measure_time_v(run_one(bin_path, profiler, algo="balanced"),
                              timeout=timeout, cwd=run_cwd)
        if t["max_rss_kb"] is not None:
            metrics[(0, "max_rss_kb")] = [t["max_rss_kb"]]
        if t["wall_s"] is not None:
            metrics[(0, "wall_s")] = [t["wall_s"]]
        if not metrics:
            return None, "no metrics captured"
        return metrics, None
    if profiler == "perf-stat":
        out = bh.measure_perf_stat(cmd, timeout=timeout, cwd=run_cwd)
        if not out:
            return None, "SKIP:" + bh.perf_no_hw_skip("local")
        for evt, val in out.items():
            metrics[(0, f"perf_{evt}")] = [val]
        return metrics, None
    if profiler == "callgrind":
        # toggle_collect: count only min_balanced (+ callees) so the IR
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
        metrics[(0, "callgrind_ir")] = [ic]
        return metrics, None
    if profiler == "binary-size":
        sz = bh.binary_size(bin_path)
        if sz is None:
            return None, "binary-size failed"
        metrics[(0, "binary_size_bytes")] = [sz]
        return metrics, None
    return None, f"unknown profiler: {profiler}"


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


def print_sweep_table(cell_metrics, counts):
    """Compact per-player-count summary on stdout. The recorded metrics go to
    history; this is what an operator actually reads after a run."""
    rows = []
    for p in counts:
        row = [p]
        for name in ("maps", "median_tick_ns", "p95_tick_ns", "median_prep_ns",
                     "changed_pct", "resets", "ticks",
                     "ns_per_player", "player_ticks_per_s"):
            blk = cell_metrics.get(f"p{p}_{name}")
            row.append(blk["value"] if blk else None)
        rows.append(tuple(row))
    if not any(r[2] for r in rows):
        return
    print(_color(bh.C_CYAN, "[sweep] this node's replay, per player count "
                            "(every map is broadcast every tick)"))
    # The last two columns are the ones that survive a change of population:
    # every node runs a different one, so its raw median_ns compares with
    # nothing until the load is divided back out.
    print("  players   maps   median_ns    p95_ns   prep%   changed%"
          "   resets     ticks    ns/plyr   plyr-tk/s")
    for (p, maps, med, p95, prep, chg, resets, ticks, nspp, ptps) in rows:
        pct = (100.0 * prep / med) if (prep and med) else float("nan")
        print(f"  {p:>7}  {maps if maps is not None else '-':>5}"
              f"  {med if med is not None else '-':>10}"
              f"  {p95 if p95 is not None else '-':>8}"
              f"  {pct:>5.1f}%  {chg if chg is not None else float('nan'):>8.1f}%"
              f"  {resets if resets is not None else '-':>7}"
              f"  {ticks if ticks is not None else '-':>8}"
              f"  {nspp if nspp is not None else float('nan'):>9.0f}"
              f"  {ptps if ptps is not None else float('nan'):>10.0f}")
    if REFERENCE_PLAYERS in counts:
        print(f"  rows from {REFERENCE_PLAYERS} players up are the fleet ladder: "
              f"fixed counts every node runs if it can hold them, so they "
              f"compare as-is between machines")


def _metric_unit_better(metric_name):
    # net_<x> / mincpu_<x> is <x> measured with another visibility strategy:
    # same unit, same direction, its own series. Classify on the name it
    # mirrors, else net_ticks_per_s would be read as lower-is-better and
    # net_bytes_per_tick would lose its unit.
    for _prefix, _algo in EXTRA_ALGOS:
        if metric_name.startswith(_prefix):
            metric_name = metric_name[len(_prefix):]
            break
    # Throughput is higher-is-better; everything else lower-is-better.
    better = "higher" if metric_name in ("ticks_per_s", "ticks",
                                         "player_ticks_per_s") else "lower"
    # The cross-hardware trio is named first: player_ticks_per_s would
    # otherwise fall into the generic "_s means seconds" rule and be labelled
    # a duration, and the two per-player costs carry no unit suffix at all.
    unit = "player-ticks/s" if metric_name == "player_ticks_per_s" else \
           "ns" if metric_name == "ns_per_player" else \
           "bytes" if metric_name == "bytes_per_player_tick" else \
           "ticks/s" if metric_name == "ticks_per_s" else \
           "bytes" if metric_name == "bytes_per_tick" else \
           "ns" if metric_name.endswith("_ns") else \
           "bytes" if metric_name.endswith("_bytes") or metric_name == "bytes_sent" or metric_name == "binary_size_bytes" else \
           "kb" if metric_name.endswith("_kb") else \
           "s" if metric_name.endswith("_s") else \
           "%" if metric_name == "cpu_percent" or metric_name.endswith("_cpu_percent") \
                  or metric_name.endswith("_pct") else \
           "count"
    return unit, better


def _samples_of(value):
    """Accept either a bare value or a list of samples -- the local rusage
    cell collects several passes, every other cell has exactly one."""
    if isinstance(value, (list, tuple)):
        return [v for v in value if v is not None]
    return [] if value is None else [value]


def _median_stddev(samples):
    """Median + population stddev of a sample list (0.0 for a single one)."""
    ordered = sorted(samples)
    n = len(ordered)
    median = ordered[n // 2] if n % 2 else (ordered[n // 2 - 1] + ordered[n // 2]) / 2.0
    if n < 2:
        return median, 0.0
    mean = sum(ordered) / float(n)
    var = sum((v - mean) ** 2 for v in ordered) / float(n)
    return median, var ** 0.5


def _cell_to_metric_block(per_cell):
    """Convert the {(player, metric): value-or-samples} cell dict into the
    per-tool `metrics` block expected by history_recorder.PlatformRecord."""
    out = {}
    for (player, metric_name), value in per_cell.items():
        samples = _samples_of(value)
        if not samples: continue
        key = metric_name if player == 0 else f"p{player}_{metric_name}"
        unit, better = _metric_unit_better(metric_name)
        median, stddev = _median_stddev(samples)
        out[key] = {"value": median, "unit": unit, "better": better,
                    "samples": samples, "median": median, "stddev": stddev}
    return out


# Recorded in history and charts, but kept OUT of the champion/candidate
# metric set: they describe the RUN, not the server's performance. `maps`,
# `changed_pct` and `walk_pct` are properties of the fixed workload (they
# cannot move unless the workload changes -- if they DO move, the workload
# drifted and that is what needs looking at), and median_prep_ns is the
# harness's own cost. Letting any of them into the decision matrix would let
# harness noise veto a real server improvement.
# NOTE binary_size_bytes now includes the workload baked into stage 2, so it
# reads as "what this node has to carry", not as pure code size. It is stable
# run to run for a given node (the generated workload is deterministic) and a
# code-size regression still shows in it, so it stays in the decision set.
DIAGNOSTIC_METRICS = ("maps", "resets", "median_prep_ns", "changed_pct",
                      "bytes_sent",
                      "world_maps", "workload_players", "workload_cycle_ticks",
                      "workload_entries", "workload_replay_bytes",
                      # Derived: exact functions of metrics the champion already
                      # judges (ticks_per_s, median_tick_ns, bytes_per_tick), so
                      # they are recorded for cross-hardware reading and kept OUT
                      # of KEEP/DISCARD -- judging both weighs one movement twice.
                      "player_ticks_per_s", "ns_per_player", "bytes_per_player_tick")


def aggregate_metrics(per_cell):
    """Flatten nested {(player,metric): value-or-samples} into a single
    metric-name -> {median, stddev, unit, better} dict suitable for
    champion/candidate JSON. The local rusage cell carries several passes
    (real stddev -> a real noise band); perf/callgrind/binary-size are
    deterministic single values and keep stddev 0."""
    out = {}
    for (player, metric_name), value in per_cell.items():
        samples = _samples_of(value)
        if not samples: continue
        if metric_name in DIAGNOSTIC_METRICS: continue
        key = metric_name if player == 0 else f"p{player}_{metric_name}"
        unit, better = _metric_unit_better(metric_name)
        median, stddev = _median_stddev(samples)
        out[key] = {"median": median, "stddev": stddev,
                    "unit": unit, "better": better}
    return out


def _runtime_cmd_string(profiler="rusage", small_players=None):
    """Build the bench binary's argv as a single shell-quoted string for
    use on the exec node. The binary lives in the exec node's work_dir
    after push_binary_to_exec; we invoke it as ./BIN_NAME so cwd is the
    work_dir. Per-profiler workload mode: fixed-time (--ms) for throughput
    profilers, fixed-iteration (--ticks) for callgrind."""
    parts = [f"./{BIN_NAME}", *_mode_args(profiler)]
    # Same rule as run_one(): rusage replays the three strategies in one
    # process (the exec node runs ONE command per cell, so this is how the
    # fleet gets all three), every other profiler measures the process itself
    # and must see exactly one.
    if profiler != "rusage":
        parts += ["--algo", "balanced"]
    # Callgrind is ~30x, and a node's largest cell is now whatever its RAM
    # allows -- tens of thousands of players. Give it the SMALL cell only: its
    # metric is a deterministic instruction count, which does not need the
    # biggest workload to be comparable, and the big one would run for hours.
    if profiler == "callgrind" and small_players:
        parts += ["--players", str(small_players)]
    return " ".join(parts)


def _push_workload(compile_node, workload_path, label):
    """Park this node's generated workload on its COMPILE node, outside the
    rsync'd source tree (that one is mirrored with --delete), and return the
    remote path for -DCC_WORKLOAD_CPP.

    It cannot travel with the sources: it is generated per exec node and per
    datapack, and a generated file has no business in the repo."""
    work = compile_node["work_dir"]
    remote_dir = f"{work}/{REMOTE_WORKLOAD_SUBDIR}"
    u = compile_node["ssh"]["user"]
    h = compile_node["ssh"]["host"]
    port = compile_node["ssh"].get("port", 22)
    import shlex
    rc, _, _ = br.ssh_run(u, h, port, f"mkdir -p {shlex.quote(remote_dir)}",
                          timeout=60)
    if rc != 0:
        return None, f"cannot create {remote_dir} on {compile_node['label']}"
    remote_path = f"{remote_dir}/workload-{label}.cpp"
    rc, msg = br.rsync_to(u, h, port, workload_path, remote_path, delete=False)
    if rc != 0:
        return None, f"workload rsync failed: {msg}"
    return remote_path, "ok"


def _remote_spec(node, avail_profilers, skips, all_profilers,
                 progress, per_tool, stage1_bin):
    """Build the run_profiler_fleet spec for one remote exec node: generate
    ITS workload (its RAM sets the population), push that to its compile node,
    and point stage 2's build at it. Emits the SKIP progress lines
    (no-compile-node / tool-missing / generation failed) up front so the
    counter stays in lock-step, then returns the spec dict -- or None when
    there is nothing to run."""
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
    # Is this still the machine this label names? An address outlives its
    # board, and measuring the wrong one is worse than not measuring at all.
    ok, why = node_identity_ok(exec_node)
    if not ok:
        bh.print_node_error("benchmarkmapmanager2", label, "SKIP", why)
        for prof in runnable:
            progress.emit(prof, "no", label, status="SKIP", extra="wrong-machine")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        return None
    # STAGE 1 for this node: its own RAM decides how many players it is given,
    # so the generator gets the exec-node dict -- that is what gets asked.
    workload, info = generate_workload(stage1_bin, exec_node)
    if workload is None:
        bh.print_node_error("benchmarkmapmanager2", label, "SKIP", str(info))
        for prof in runnable:
            progress.emit(prof, "no", label, status="SKIP",
                          extra="stage1-generate-failed")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        return None
    remote_workload, msg = _push_workload(compile_node, workload, label)
    if remote_workload is None:
        bh.print_node_error("benchmarkmapmanager2", label, "SKIP", msg)
        for prof in runnable:
            progress.emit(prof, "no", label, status="SKIP",
                          extra="workload-push-failed")
            per_tool[label][prof] = {"status": "SKIP", "metrics": {}}
        return None
    return {
        "exec_node":         exec_node,
        "compile_node":      compile_node,
        "cmake_src_subdir":  "benchmark/benchmarkmapmanager2/stage2",
        "build_subdir_base": "benchmarkmapmanager2",
        "bin_name":          BIN_NAME,
        # Per-profiler workload mode: fixed-time (--ms) for rusage/perf-stat,
        # fixed-iteration (--ticks) for callgrind.
        "runtime_cmd":       {p: _runtime_cmd_string(p, NODE_SMALL_CELL.get(label))
                              for p in runnable},
        "profilers":         runnable,
        # The generated workload is a build INPUT: run_profiler_fleet keys its
        # build dirs on these defs, so each node compiles its own binary.
        "cmake_defs":        {"CMAKE_BUILD_TYPE": "Release",
                              "CC_WORKLOAD_CPP": remote_workload},
        # Remote callgrind counts only min_balanced via --toggle-collect
        # (excludes startup). Resolved against symbols at runtime, so no
        # build define / valgrind header needed on the cross compile node.
        "callgrind_toggle":  MAP_CALLGRIND_TOGGLE,
        "run_timeout":       RUN_TIMEOUT,
    }


def run_fleet_serially(remote_specs, progress, per_tool, all_metrics):
    """Build and measure ONE NODE AT A TIME.

    Not run_profiler_fleet(): that one drives the fleet from a thread pool, and
    its workers spawn subprocesses with a preexec_fn. Forking a threaded
    process runs Python code in the child between fork and exec, where any
    lock another thread happened to hold is locked forever with no owner --
    CPython documents preexec_fn as unsafe with threads for exactly this. It
    deadlocked this benchmark's first fleet run: a child stuck in futex before
    exec, the parent stuck in wait4, an hour with no output. A serial loop has
    no threads, so the fork is safe, and there is little to lose here anyway --
    every node carries its OWN generated workload, so no two nodes could have
    shared a build.
    """
    for spec in remote_specs:
        exec_node = spec["exec_node"]
        label = exec_node["label"]
        compile_node = spec["compile_node"]
        # One build dir per exec node: each has its own workload compiled in.
        build_subdir = f"{spec['build_subdir_base']}-{label}"
        print(_color(bh.C_CYAN, f"[fleet] {label}: building on "
                                f"{compile_node['label']}"))
        rc, msg, bld = br.build_for_fleet(
            compile_node, spec["cmake_src_subdir"], build_subdir,
            cmake_defs=spec.get("cmake_defs"), use_ninja=exec_node.get("ninja"),
            verbose=True, exec_node=exec_node)
        if rc != 0:
            # A compile-node build failure is NOT a regression: the exec node
            # is never touched, so its metric is unknown -- SKIP, not FAIL.
            _record_remote_result(label, spec["profilers"],
                                  {p: None for p in spec["profilers"]},
                                  f"SKIP:compile-failed: {msg}",
                                  progress, per_tool, all_metrics)
            continue
        print(_color(bh.C_CYAN, f"[fleet] {label}: running"))
        out, m = br.push_and_run_profilers(
            compile_node, exec_node, bld, spec["bin_name"],
            spec["runtime_cmd"], spec["profilers"],
            extras=spec.get("extras"),
            run_timeout=spec.get("run_timeout", 600),
            verbose=True, callgrind_toggle=spec.get("callgrind_toggle"),
            stage_fn=spec.get("stage_fn"))
        _record_remote_result(label, spec["profilers"], out, m,
                              progress, per_tool, all_metrics)


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
            # The live counter truncates; the banner carries the whole cause
            # and the command to reproduce just this benchmark.
            detail = err_msg
            for key in ("stdout", "stderr"):
                tail = (res.get(key) or "").strip()
                if tail:
                    detail += f"\n--- {key} (tail) ---\n" + tail[-1200:]
            bh.print_node_error("benchmarkmapmanager2", label, "FAIL", detail)
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
            wl = parse_workload_line(res.get("stdout") or "")
            if wl.get("maps"):
                cell[(0, "world_maps")]            = wl["maps"]
                cell[(0, "workload_players")]      = wl.get("players")
                cell[(0, "workload_cycle_ticks")]  = wl.get("cycle_ticks")
                cell[(0, "workload_entries")]      = wl.get("entries_per_player")
                cell[(0, "workload_replay_bytes")] = wl.get("replay_bytes")
            bad = _bench_to_cell(parse_bench_lines(res.get("stdout") or ""), cell)
            if bad is not None:
                progress.emit(prof, "no", label, status="FAIL", extra=bad)
                bh.print_node_error("benchmarkmapmanager2", label, "FAIL", bad)
                per_tool[label][prof] = {"status": "FAIL", "metrics": {},
                                         "error": bad}
                continue
            if not cell:
                err_msg = res.get("error", "no-data")
                detail = err_msg + "\n--- stdout (tail) ---\n" + \
                         (res.get("stdout") or "")[-1200:]
                bh.print_node_error("benchmarkmapmanager2", label, "FAIL", detail)
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
                bh.print_node_error("benchmarkmapmanager2", label, "FAIL", err_msg)
                progress.emit(prof, "no", label, status="FAIL", extra=err_msg)
                per_tool[label][prof] = {"status": "FAIL", "metrics": {}, "error": err_msg}
        elif prof == "binary-size":
            if res is None:
                bh.print_node_error("benchmarkmapmanager2", label, "FAIL",
                                    "the exec node reported no binary size "
                                    "(the push may have landed nothing)")
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
    # The world is not optional: without the datapack there is no workload, and
    # inventing a synthetic one would silently measure something else.
    world_dir = os.path.join(DATAPACK_PATH, "map", "main", "generated")
    if not os.path.isdir(world_dir):
        bh.print_local_build_error(
            "benchmarkmapmanager2", "datapack check", "",
            f"the generated map set is missing: {world_dir}\n"
            f"This benchmark's workload IS that map set, loaded with the "
            f"production Map_loader. Point DATAPACK_PATH at a datapack that "
            f"has map/main/generated/, or generate it with "
            f"tools/map-procedural-generation/.")
        return 2
    # STAGE 1 -- here, once: it reads the datapack, which no other machine has.
    stage1_bin = build_stage1()
    if stage1_bin is None:
        return 2
    # The local baseline gets its own workload too: this host's RAM decides its
    # population exactly like any other node's.
    local_workload, local_info = generate_workload(stage1_bin, "local")
    if local_workload is None:
        bh.print_local_build_error("benchmarkmapmanager2", "stage1 generate",
                                   "", str(local_info))
        return 2
    # Independent check of the generated file before anything is built on it.
    # Stage 1 writes the vectors and stage 2 replays them, so the two agree by
    # construction -- an encoding bug would be invisible to both. verify_workload
    # is a third implementation, written from the header contract: it replays
    # the file in Python and must reach the state stage 1 predicted, with no
    # step leaving its map. Run on the local workload only (~5 s); the fleet's
    # own copies are covered at runtime by replay_mismatch.
    rc, sout, serr, _ = bh.run_capture(
        [sys.executable, os.path.join(STAGE2_SRC, "..", "verify_workload.py"),
         local_workload], timeout=600)
    if rc != 0:
        bh.print_local_build_error(
            "benchmarkmapmanager2", "workload verification", sout,
            "the generated workload does not replay to the state stage 1 "
            "computed, or a step leaves its map -- the two stages disagree "
            "about the workload, so nothing measured from it would mean "
            "anything.\n" + (serr or ""))
        return 2
    print(_color(bh.C_GREEN, "[verify] local workload replays to the state "
                             "stage 1 predicted, no step leaves its map"))
    bin_path = build_stage2(local_workload, os.path.join(BUILD_DIR, "stage2-local"))
    if bin_path is None:
        return 2

    if args.profile:
        tools = bh.profile_tools(args.profile)
        # Each node profiles ITS OWN workload, so the build input is resolved
        # per node: stage 1 generates it here, it is pushed to that node's
        # compile parent, and the path comes back as the define stage 2 needs.
        def _profile_defs(exec_node, compile_node):
            workload, _why = generate_workload(stage1_bin, exec_node)
            if workload is None:
                return None
            remote_workload, msg = _push_workload(compile_node, workload,
                                                  exec_node["label"])
            if remote_workload is None:
                return None
            return {"CMAKE_BUILD_TYPE": "Release",
                    "CC_WORKLOAD_CPP": remote_workload}
        remote_spec = {
            "cmake_src_subdir": "benchmark/benchmarkmapmanager2/stage2",
            "build_subdir_base": "benchmarkmapmanager2",
            "bin_name": BIN_NAME,
            "cmake_defs_fn": _profile_defs,
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
    # stage 2's CMakeLists appends -O2, which lands after the Release -O3 and
    # wins; record what the compiler was actually given.
    compile_flags = ["-O2", "-DNDEBUG", "-DCMAKE_BUILD_TYPE=Release"]

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
                                progress, per_tool, stage1_bin)
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

        # Per-CLAUDE.md "CPU% per sub-benchmark": for the rusage profile only
        # (perf/callgrind/binary-size are deterministic single values and have
        # no workload axis), probe each player count individually and emit one
        # sub-bench slice per count carrying cpu_percent + its tick metrics.
        if per_tool[label].get("rusage", {}).get("status") == "PASS":
            # This node's own counts, learned from the run it just did.
            # EXACT match on the metric name, not endswith(): the prefixed
            # net_/mincpu_ series also end in _median_tick_ns and would list
            # every player count three times.
            counts = sorted(int(k[1:].split("_", 1)[0])
                            for k in per_tool[label]["rusage"]["metrics"]
                            if k.startswith("p") and
                            k.split("_", 1)[-1] == "median_tick_ns")
            slices = {}
            cell_metrics = per_tool[label]["rusage"]["metrics"]
            for p in counts:
                slice_metrics = {}
                # pull parsed tick/byte values out of the existing flat
                # metric dict (keys are p<count>_<metric_name>).
                prefix = f"p{p}_"
                for key, blk in cell_metrics.items():
                    if key.startswith(prefix):
                        slice_metrics[key[len(prefix):]] = blk
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
            for p in counts:
                blk = cell_metrics.get(f"p{p}_median_tick_ns")
                if blk is not None:
                    fit.append((p, blk["value"]))
            print_sweep_table(cell_metrics, counts)
            exp = _fit_exponent(fit)
            if exp is not None:
                slices["scaling"] = {"scaling_exponent": {
                    "value": exp, "median": exp, "stddev": 0.0,
                    "unit": "count", "better": "lower", "samples": [exp]}}
                print(_color(bh.C_CYAN,
                             f"  scaling exponent = {exp:.2f}  "
                             f"(2.0 = quadratic in players, 1.0 = linear, "
                             f"<1.0 = the constant of ticking the whole world "
                             f"still dominates at the small end)"))
            per_subbench[label]["rusage"] = slices

    # Remote fleet, in PARALLEL: phase 1 builds every unique compile node
    # at once; phase 2 pushes the binary + runs the profilers on each exec
    # node (corresponding to its compile node) concurrently. Results are
    # recorded serially here afterwards so Progress stays single-threaded.
    if remote_specs:
        print(_color(bh.C_CYAN,
              f"[bench] remote fleet: {len(remote_specs)} exec node(s), one at "
              f"a time (each carries its own generated workload, so no two "
              f"could share a build anyway)"))
        run_fleet_serially(remote_specs, progress, per_tool, all_metrics)

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
        # No datapack anywhere but on this host: the measured binary carries
        # its workload compiled in.
        if node["label"] == "local":
            cc_path = os.path.join(BUILD_DIR, "stage2-local")
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
        # A node's metric NAMES carry its player counts, and those come from
        # its RAM. If a node gained or lost memory, stage 1 gives it a
        # different population and NOTHING lines up with the champion any more
        # -- decide_multi_node quietly skips a metric it cannot pair, so that
        # node would drop out of the verdict without a word. Say it out loud:
        # the node has re-baselined and its next run is its new reference.
        if champ:
            for label, blk in rec["nodes"].items():
                ch_metrics = (champ.get("nodes", {}).get(label) or {}).get("metrics", {})
                if not ch_metrics:
                    continue
                common = set(ch_metrics) & set(blk.get("metrics", {}))
                if not common:
                    print(_color(bh.C_YELLOW,
                          f"[champion] {label}: nothing comparable with the "
                          f"champion -- its workload changed (its RAM decides "
                          f"the player count, and the metric names carry it). "
                          f"This node is re-baselining, not regressing."))
                elif len(common) < len(ch_metrics) / 2:
                    print(_color(bh.C_YELLOW,
                          f"[champion] {label}: only {len(common)} of "
                          f"{len(ch_metrics)} champion metrics could be paired "
                          f"-- part of its workload changed."))
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
