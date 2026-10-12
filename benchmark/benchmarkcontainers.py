#!/usr/bin/env python3
"""benchmarkcontainers.py -- std::vector vs std::list vs std::deque on real data.

Builds benchmark/benchmarkcontainers/ (benchmark_containers) and runs it
RUN_REPEATS times on a datapack read with the production loader: the files a
server of its BIGGEST maincode serves (base + that maincode), that maincode's
maps, the items, the monsters, and the small per-player FIFO the server Client
keeps (std::queue = std::deque by default). Per dataset and container:
build time, read-only scan rate, heap bytes and heap blocks of one built
container; per FIFO depth: ops/s, allocations per op and what an EMPTY
container already holds. The median of the runs is printed as a table.

  python3 benchmarkcontainers.py [--datapack DIR] [--ms WINDOW] [--comment ...]

Without --datapack it measures the default datapack and records the per-node
history like every benchmark. With --datapack the numbers are printed only:
nothing about that datapack (path, name, numbers) is written anywhere.
Runs on the host, ON every detected ESP32 board and on the T2100 i486 laptop:
those cannot load the datapack, so the host emits the files + items datasets as
C++ arrays compiled into the firmware / a small cross-built binary
(benchmarkcontainers/embedded), which also runs the FIFO pattern. No fleet yet.
"""
import argparse
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "test"))

import benchmark_helpers as bh
import benchmark_microbench as bmicro
import history_recorder as hr

NAME = "benchmarkcontainers"
DEFAULT_DATAPACK = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
RUN_REPEATS = 3
WINDOW_MS = 200
# Fixed time: every cell is one WINDOW_MS window (a pass is cut mid-way when the
# window ends), so each timeout is the run's budget (windows x window) + a margin
# for what is not windowed: loading the data, one full build per cell for its heap.
HOST_DATASETS = 4               # files, maps, items, monsters
EMBEDDED_DATASETS = 2           # files, items
# key -> value cells: files (flat, std_flat_map, unordered_map, map) + 3 x 5 by id
HOST_LOOKUP_WINDOWS = 4 + 3 * 5
EMBEDDED_LOOKUP_WINDOWS = 3 + 4  # no <flat_map> there: files 3, items 4
HOST_MARGIN_S = 300             # datapack + maps parse
EMBEDDED_MARGIN_S = 180         # data setup + one full build per cell on a 50 MHz 486
EMIT_TIMEOUT = 600

try:
    import test_config
    BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
except Exception:
    BUILD_ROOT = "/tmp/cc-build"
SRC_DIR = os.path.join(bh.REPO_ROOT, "benchmark", "benchmarkcontainers")
BUILD_DIR = os.path.join(BUILD_ROOT, "benchmark", NAME)
BIN = os.path.join(BUILD_DIR, "benchmark_containers")
ESP32_PROJECT = os.path.join(SRC_DIR, "esp32")
EMBEDDED_EMIT_MAX_FILES = 4000  # the ESP32 / i486 binary keeps what its memory holds
EMBEDDED_BIN = "benchmark_containers_embedded"
T2100_GATE_S = 60
ESP32_BOOT_S = 10               # USB re-enumeration wait in the firmware
CONTAINERS = ("vector", "vector_reserved", "list", "deque")
FIFO_CONTAINERS = ("vector", "list", "deque")


def budget_s(datasets, lookup_windows, window_ms):
    """Seconds of measurement of one run: a build, a scan and a random-access
    window per dataset and container, one per lookup structure, one per FIFO
    depth (1, 16) and container."""
    windows = datasets * len(CONTAINERS) * 3 + lookup_windows + 2 * len(FIFO_CONTAINERS)
    return windows * window_ms / 1000.0

# metric suffix -> (unit, better)
UNITS = {
    "build_ns_per_elem": ("ns", "lower"),
    "scan_elems_per_s":  ("1/s", "higher"),
    "heap_bytes":        ("B", "lower"),
    "heap_blocks":       ("blocks", "lower"),
    "cpu_percent":       ("%", "lower"),
    "ops_per_s":         ("1/s", "higher"),
    "allocs_per_op":     ("allocs", "lower"),
    "empty_heap_bytes":  ("B", "lower"),
    "random_ns":         ("ns", "lower"),
    "lookup_ns":         ("ns", "lower"),
}
LOOKUPS = ("dense_vector", "flat", "std_flat_map", "unordered_map", "map")


def check_values(values):
    """None, or why a run's output is unusable."""
    if values.get("done") != 1.0:
        return "no 'BENCH done' line"
    missed = [k for k in values if k.endswith(".lookup_missed")]
    if missed:
        return "lookups of existing keys missed: " + ", ".join(missed)
    return None


def build():
    cfg = ["cmake", "-S", SRC_DIR, "-B", BUILD_DIR, "-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    bh.drop_stale_cmake_cache(BUILD_DIR, SRC_DIR)
    for stage, cmd in (("cmake configure", cfg),
                       ("cmake build", ["cmake", "--build", BUILD_DIR, "-j", str(os.cpu_count() or 1)])):
        rc, sout, serr, _ = bh.run_capture(cmd, timeout=1800)
        if rc != 0:
            bh.print_local_build_error(NAME, stage, sout, serr)
            return False
    return os.path.isfile(BIN)


def run_once(datapack, window_ms):
    """One run -> ({key: value}, error)."""
    timeout = budget_s(HOST_DATASETS, HOST_LOOKUP_WINDOWS, window_ms) + HOST_MARGIN_S
    try:
        proc = subprocess.run([BIN, "--datapack", datapack, "--ms", str(window_ms)],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, f"timeout after {timeout:.0f}s"
    values = parse_bench(proc.stdout.decode(errors="replace"))
    problem = check_values(values)
    if proc.returncode != 0 or problem is not None:
        return None, f"rc={proc.returncode}, {problem}:\n" + proc.stderr.decode(errors="replace")[-2000:]
    return values, None


def metric_block(samples, suffix):
    median, stddev = bh.stats_of(samples)
    unit, better = UNITS[suffix]
    return {"value": median, "median": median, "stddev": stddev, "samples": samples,
            "unit": unit, "better": better}


def parse_bench(text):
    values = {}
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("BENCH ") and "=" in line:
            key, value = line[6:].split("=", 1)
            try:
                values[key] = float(value)
            except ValueError:
                pass
    return values


def print_tables(median):
    def fmt(value):
        if value is None:
            return "-"
        if value >= 1e6:
            return f"{value / 1e6:.1f}M"
        if value >= 1e4:
            return f"{value / 1e3:.1f}k"
        return f"{value:.3g}"
    if "maincode_maps" in median:
        print(f"\nbiggest maincode: {int(median['maincode_maps'])} maps "
              f"({int(median.get('maps_failed', 0))} failed to load)")
    for dataset in ("files", "maps", "items", "monsters"):
        if f"{dataset}.elements" not in median:
            continue
        print(f"\n{dataset}: {int(median.get(dataset + '.elements', 0))} elements "
              f"of {int(median.get(dataset + '.sizeof_elem', 0))} B")
        columns = ("build_ns_per_elem", "scan_elems_per_s", "random_ns", "heap_bytes", "heap_blocks")
        print(f"  {'container':16s}" + "".join(f"{c:>20s}" for c in columns) + f"{'scan vs vector':>16s}")
        base = median.get(f"{dataset}.vector_reserved.scan_elems_per_s")
        for container in CONTAINERS:
            row = [median.get(f"{dataset}.{container}.{c}") for c in columns]
            scan = median.get(f"{dataset}.{container}.scan_elems_per_s")
            ratio = f"{scan / base:.2f}x" if scan and base else "-"
            print(f"  {container:16s}" + "".join(f"{fmt(v):>20s}" for v in row) + f"{ratio:>16s}")
        if any(f"{dataset}.{lookup}.lookup_ns" in median for lookup in LOOKUPS):
            print(f"  lookup by key (random, read-only):")
            columns = ("lookup_ns", "heap_bytes", "heap_blocks")
            print(f"  {'structure':16s}" + "".join(f"{c:>20s}" for c in columns))
            for lookup in LOOKUPS:
                if f"{dataset}.{lookup}.lookup_ns" in median:
                    row = [median.get(f"{dataset}.{lookup}.{c}") for c in columns]
                    print(f"  {lookup:16s}" + "".join(f"{fmt(v):>20s}" for v in row))
    for depth in (1, 16):
        print(f"\nfifo depth {depth} (the server Client's per-player queues):")
        columns = ("ops_per_s", "allocs_per_op", "empty_heap_bytes")
        print(f"  {'container':16s}" + "".join(f"{c:>20s}" for c in columns))
        for container in FIFO_CONTAINERS:
            row = [median.get(f"fifo{depth}.{container}.{c}") for c in columns]
            print(f"  {container:16s}" + "".join(f"{fmt(v):>20s}" for v in row))


def emit_data(datapack):
    """The files + items datasets as C++ arrays, for the targets that cannot load
    the datapack. -> (path, error)"""
    data_dir = os.path.join(BUILD_DIR, "embedded-data")
    os.makedirs(data_dir, exist_ok=True)
    data_cpp = os.path.join(data_dir, "containers_data.cpp")
    rc, sout, serr, _ = bh.run_capture([BIN, "--datapack", datapack, "--emit-esp32", data_cpp,
                                        str(EMBEDDED_EMIT_MAX_FILES)], timeout=EMIT_TIMEOUT)
    if rc != 0:
        return None, "emit failed: " + serr[-500:]
    return data_cpp, None


def measure_t2100(datapack, external, window_ms):
    """The i486 laptop: cross-build the embedded suite with the data compiled in,
    push it over the SLIP link, run it behind the 60 s + load < 1 gate.
    -> (values, error)"""
    import benchmark_t2100 as t2100
    data_cpp, err = emit_data(datapack)
    if data_cpp is None:
        return None, "FAIL:" + err
    bin_path, err = t2100.cross_build("benchmark/benchmarkcontainers/embedded", "build-containers",
                                      EMBEDDED_BIN, {"CC_CONTAINERS_DATA_CPP": data_cpp})
    if external:
        os.remove(data_cpp)   # it holds that datapack's file names
    if bin_path is None:
        return None, "SKIP:cross-build: " + err
    ok, detail = t2100.push([bin_path])
    if not ok:
        return None, "SKIP:push: " + detail
    rc, sout, serr = t2100.ssh(f"cd {t2100.T2100_NODE['work_dir']} && sleep {T2100_GATE_S} && cat /proc/loadavg"
                               f" && ./{EMBEDDED_BIN} --ms {window_ms}",
                               timeout=T2100_GATE_S + budget_s(EMBEDDED_DATASETS, EMBEDDED_LOOKUP_WINDOWS, window_ms) + EMBEDDED_MARGIN_S)
    if rc != 0:
        return None, f"FAIL:rc={rc}: {(serr or sout)[-1500:]}"
    lines = sout.splitlines()
    try:
        load = float(lines[0].split()[0])
    except (IndexError, ValueError):
        return None, "FAIL:no /proc/loadavg line: " + sout[:300]
    if load >= 1.0:
        return None, f"SKIP:load={load}"
    values = parse_bench(sout)
    problem = check_values(values)
    if problem is not None:
        return None, f"FAIL:{problem}:\n" + sout[-1500:]
    return values, None


def measure_esp32(node, datapack, external, window_ms):
    """One ESP32 board: emit the datasets, build + flash + read. -> (values, error)."""
    import benchmark_esp32 as esp
    if node["tty"] is None:
        return None, "SKIP:" + node["skip_reason"]
    data_cpp, err = emit_data(datapack)
    if data_cpp is None:
        return None, "FAIL:" + err
    os.environ["CC_CONTAINERS_DATA_CPP"] = data_cpp
    status, detail, text = esp.run_ondevice_benchmark(
        ESP32_PROJECT, os.path.join(esp.ESP32_PREFIX, "build-containers"),
        read_secs=ESP32_BOOT_S + budget_s(EMBEDDED_DATASETS, EMBEDDED_LOOKUP_WINDOWS, window_ms) + EMBEDDED_MARGIN_S, verbose=False, target=node["target"], serial=node["tty"])
    if external:
        os.remove(data_cpp)   # it holds that datapack's file names
    if status != "PASS":
        return None, "SKIP:" + detail
    values = parse_bench(text)
    problem = check_values(values)
    if problem is not None:
        tail = "\n".join(line for line in text.splitlines() if line.strip())[-1500:]
        return None, f"FAIL:{problem}; console tail:\n" + tail
    return values, None


def main():
    pre = argparse.ArgumentParser(add_help=False)
    pre.add_argument("--datapack", default=None)
    pre.add_argument("--ms", type=int, default=WINDOW_MS)
    own, rest = pre.parse_known_args()
    args = bh.parse_bench_args(rest)
    external = own.datapack is not None
    datapack = os.path.abspath(own.datapack) if external else DEFAULT_DATAPACK
    if not os.path.isdir(datapack):
        print("datapack directory not found" + ("" if external else f": {datapack}"))
        return 2
    if bh.acquire_singleton_lock(NAME) is None:
        return 3
    bmicro.ensure_missing()
    if not build():
        return 2

    import benchmark_esp32 as esp
    import benchmark_t2100 as t2100
    esp32_nodes = esp.benchmark_nodes(verbose=False)
    with_t2100 = bh.node_allowed(t2100.T2100_NODE["label"], t2100.T2100_NODE["arch"])
    progress = bh.Progress(RUN_REPEATS + len(esp32_nodes) + (1 if with_t2100 else 0), NAME)
    batch_id = hr.new_batch_id()
    started_utc = hr.iso_now()
    runs = []
    while len(runs) < RUN_REPEATS:
        values, err = run_once(datapack, own.ms)
        if values is None:
            progress.emit("rusage", "no", "local", "FAIL", err.splitlines()[0][:120])
            bh.print_node_error(NAME, "local", "FAIL", err)
            return 1
        runs.append(values)
        progress.emit("rusage", "no", "local", "PASS")
    ended_utc = hr.iso_now()

    keys = sorted(set(k for run in runs for k in run))
    median = {k: bh.stats_of([run.get(k) for run in runs])[0] for k in keys}
    print(f"\n==== local ({bh.host_arch()})")
    print_tables(median)

    boards = {}
    for node in esp32_nodes:
        values, err = measure_esp32(node, datapack, external, own.ms)
        if values is None:
            status, reason = err.split(":", 1)
            progress.emit("rusage", "no", node["label"], status, reason.splitlines()[0][:120])
            bh.print_node_error(NAME, node["label"], status, reason)
            continue
        progress.emit("rusage", "no", node["label"], "PASS")
        boards[node["label"]] = (node, values)
        print(f"\n==== {node['label']} (on the board, files cut to "
              f"{int(values.get('files.elements', 0))}/{int(values.get('files.available', 0))} by its heap)")
        print_tables(values)

    t2100_values = None
    if with_t2100:
        label = t2100.T2100_NODE["label"]
        t2100_values, err = measure_t2100(datapack, external, own.ms)
        if t2100_values is None:
            status, reason = err.split(":", 1)
            progress.emit("rusage", "no", label, status, reason.splitlines()[0][:120])
            bh.print_node_error(NAME, label, status, reason)
        else:
            progress.emit("rusage", "no", label, "PASS")
            print(f"\n==== {label} (i486, files cut to {int(t2100_values.get('files.elements', 0))}/"
                  f"{int(t2100_values.get('files.available', 0))} by its "
                  f"{int(t2100_values.get('mem_available_kb', 0))} KiB available)")
            print_tables(t2100_values)

    if external:
        print("\n(--datapack given: numbers printed only, nothing recorded)")
        return 0

    metrics = {}
    slices = {}
    for key in keys:
        parts = key.split(".")
        if len(parts) == 3 and parts[2] in UNITS:
            block = metric_block([run.get(key) for run in runs], parts[2])
            metrics[key] = block
            slices.setdefault(f"{parts[0]}-{parts[1]}", {})[parts[2]] = block
    pr = hr.PlatformRecord(NAME, batch_id, "local", runner=hr.local_runner,
                           arch_hint=bh.host_arch()).collect(cc_binary_path=BUILD_DIR,
                                                             datapack_path=datapack)
    pr.add_result("rusage", metrics, status="PASS")
    for label, block in sorted(slices.items()):
        pr.add_subbenchmark("rusage", label, block)
    out = pr.write(commit=bh.git_sha(), started_utc=started_utc, ended_utc=ended_utc,
                   compile_flags=["-O2", "-DCATCHCHALLENGER_CLIENT", "-DCATCHCHALLENGER_DB_FILE"],
                   simd_tier="generic", harness_version=hr.harness_version(),
                   comment=args.comment)
    if out is not None:
        bh.chatter(f"[history] {out}")
    targets = [(label, node, values) for label, (node, values) in boards.items()]
    if t2100_values is not None:
        targets.append((t2100.T2100_NODE["label"], t2100.T2100_NODE, t2100_values))
    for label, node, values in targets:
        if node.get("t2100"):
            pr = hr.PlatformRecord(NAME, batch_id, label, runner=t2100.runner,
                                   arch_hint=node.get("arch")).collect(
                                       cc_binary_path=node["work_dir"], datapack_path=node["work_dir"])
            pr.platform.update(t2100.platform_fields())
        else:
            pr = hr.PlatformRecord(NAME, batch_id, label, runner=None, arch_hint=node.get("arch"))
            pr.platform.update(esp.esp32_platform_fields(node["target"]))
        block = {}
        slices = {}
        for key, value in values.items():
            parts = key.split(".")
            if len(parts) == 3 and parts[2] in UNITS:
                unit, better = UNITS[parts[2]]
                metric = {"value": value, "median": value, "stddev": 0.0, "samples": [value],
                          "unit": unit, "better": better}
                block[key] = metric
                slices.setdefault(f"{parts[0]}-{parts[1]}", {})[parts[2]] = metric
        pr.add_result("rusage", block, status="PASS")
        for slice_label, slice_block in sorted(slices.items()):
            pr.add_subbenchmark("rusage", slice_label, slice_block)
        out = pr.write(commit=bh.git_sha(), started_utc=started_utc, ended_utc=hr.iso_now(),
                       compile_flags=(t2100.COMPILE_FLAGS if node.get("t2100")
                                      else ["-Os", "-DCC_TARGET_ESP32"]), simd_tier="generic",
                       harness_version=hr.harness_version(), comment=args.comment)
        if out is not None:
            bh.chatter(f"[history] {out}")
    import history_series
    history_series.main()
    bh.regenerate_charts(NAME)
    return 0


if __name__ == "__main__":
    sys.exit(main())
