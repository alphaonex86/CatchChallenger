# benchmark/ — CPU/memory profiling and AI-driven optimisation

**Always read `../CLAUDE.md` first.** When Claude is launched with
`benchmark/` as the working directory, the root CLAUDE.md is NOT auto-
loaded — open `../CLAUDE.md` before doing anything else and combine its
rules with the ones below. The root file owns project-wide conventions
(C++/Qt, CMake-per-binary layout, multi-arch support down to i486/MIPS2/
RISC-V, never-search-from-`/`) and they apply here too.
one command target: the IA not start every command, have to package in the corresponding benchmarkXXXXXX.py
* do you edit to optimise specific part
* just run this command without arguement with 1h timeout (fix the problem if timeout, you can't edit remote_nodes.json, escalate if needed), wait the finish of the command
* `--comment="..."` is the only flag every `benchmark*.py` takes. It is
  stored verbatim in this run's candidate / champion / per-platform
  history JSON (`comment` field) so a score is attributable to the
  change under test. When measuring an optimisation, pass the SAME
  comment to every benchmark, e.g. testing an "Invert loop" change →
  run all benchmarks with `--comment="Invert loop"` so every recorded
  score is tagged "Invert loop".
* compare the results
Generate helper to put in common the code it will mostly used by the majority

## Purpose

Workspace where the AI proposes performance optimisations, runs them
against a defined workload, compares metrics against the previous
champion, and decides keep / discard / escalate. The goal is to make
forward progress on perf without ever silently shipping a regression.

## Benchmarks detect regressions; profile to find the cause

`benchmark*.py` give only a coarse before/after regression signal —
*whether* a change is faster/slower across the fleet, not *where* the
time goes. To know where to optimise, profile the SAME command the
target benchmark runs:

* `--profile` on any `benchmark*.py` runs that benchmark's command once
  under a profiler, writes the profile under `/mnt/data/perso/tmpfs/`,
  and prints its path. Open that path and work the hot spots. Reach for
  perf / callgrind / heaptrack / whatever gives more CPU/memory detail
  than the benchmark itself records.
* Run WITHOUT `--profile` for the actual before/after numbers — the
  profiling build perturbs timing and must not be the regression datum.
* "more performance" with no qualifier means CPU.
* If the benchmark's own recorded metrics already pinpoint the cause,
  don't do the extra profiling pass.

## Accept / reject / escalate — strict decision matrix

After a candidate run completes on **every benchmark target**, compare
each metric against the current champion's record for that target:

| metric outcome (across all benchmarks × all nodes) | action |
|---|---|
| at least one strictly better of 30% AND none worse (rest equal-within-noise) | **KEEP** — promote to new champion |
| at least one strictly worse of 20% AND none better (rest equal-within-noise) | **DISCARD** — revert, do not commit |
| if each values is ±10% than current champion's | **DISCARD** — revert, do not commit |
| any other mix (some up, some down) | **ESCALATE** — leave artifacts in place, surface a summary, wait for human |

"Equal-within-noise" = within ±10 stddev across the warmup-dropped
samples on the same node. A single-run difference inside the noise
band IS NOT a metric movement. Don't tune for noise.

## Where benchmarks run — dynamic combination set

Every benchmark runs on:

* The current workstation (host) — compiled here, run here.
* remote_nodes.json (not execution_nodes) compiled here — , execution_nodes run here.
* Every `execution_nodes[]` entry in
  `/home/user/Desktop/CatchChallenger/remote_nodes.json` whose
  `benchmark: true` flag is set AND whose **runtime load average (1
  min) is < 1.0** at the moment the benchmark is about to start.

The combination set is **dynamic**: built right before each benchmark
batch, not precomputed at startup. A node that was idle when the
harness launched can become busy ten minutes later — not re-check load
on every node.

### Compile path — via the matching compile node, NOT locally

Each exec node is a child of one `nodes[]` compile entry (same arch,
same cross-toolchain, paired in `remote_nodes.json`). Build flow per
target:

1. Resolve the exec node's parent compile node from `remote_nodes.json`.
2. rsync the source tree to that compile node (same as
   `testingremote.py`'s remote build).
3. `cmake -S … && cmake --build` on the compile node — its toolchain
   produces the right binary for that arch / libc / ABI.
4. rsync the resulting binary + workload data from the compile node
   to the exec node.
5. Run the benchmark on the exec node.

Compiling on the host and pushing one binary to N arches doesn't work
— MIPS, RISC-V, ARM and i686 each need their own toolchain output,
and the cross-compile environments already live on the compile nodes
exactly for this reason. The host's role is orchestration + the
amd64 native baseline, nothing more.

### Load < 1 gate — sleep 60s after rsync, then check, else skip

Exact sequence on the exec node, right after the binary has been
rsync'd into place:

1. **`sleep 60`** on the exec node before doing anything else. The
   rsync itself spikes CPU + disk I/O — and the 1-minute load
   average is, by definition, a sliding window over the *past
   60 s*. Reading it immediately after rsync reflects the staging
   work, not the steady-state load we care about. Sleeping 60 s
   lets that spike fall out of the window.
2. ssh into the exec node and read `/proc/loadavg`. If the
   1-minute field is **≥ 1.0**, mark the run for that node
   `[SKIP load=X.Y]` and move on — measuring on a busy box gives
   garbage stddev that swamps the actual signal we're trying to
   detect.
3. If the field is `< 1.0`, run the benchmark. Re-check `loadavg`
   between repeats too, not just before the first iteration — a
   cron job firing every 5 min poisons only the iterations that
   straddle the spike, and you want to drop those samples (not
   the whole run).

A skipped node is NOT a failure; it just means the perf datum for
that arch is missing from this batch. The decision matrix treats
a missing metric as "unknown" — can't trigger KEEP (need at least
one strict improvement somewhere) and can't trigger DISCARD (need
at least one regression somewhere); if every node skipped, the
candidate stays in ESCALATE with reason "no measurement quorum".

The 60 s sleep is a hard floor, not a heuristic — don't lower it
because the box "looks idle". The Linux kernel computes loadavg
with an exponential decay tuned to 60 s / 5 min / 15 min, so a
shorter wait reads partially-decayed pre-rsync state and the
threshold loses meaning.

### Record the network link too

For every exec node, capture the link type used to reach it and any
characteristics that affect timing. Numbers measured over a wifi or
tunnelled link are NOT comparable to numbers measured over a 10 GbE
wired link; numbers from a qemu-emulated mips guest on an x86 host
are NOT comparable to numbers from a real mips board. Without these
fields in the JSON the operator has no way to spot that the
"regression" is just a different transport.

* **wired** → `eth_link_mbps` negotiated speed (`/sys/class/net/<i>/speed`,
  fallback `ethtool <i>`). A 100 Mbit port on an otherwise modern box
  silently caps throughput; the field exposes that.
* **wifi** → SSID, standard (wifi-4/5/6/6e/7), band (2.4/5/6 GHz),
  link rate (`iw dev <i> link` / `iw dev <i> station dump`, fallback
  `nmcli -t -f ACTIVE,SSID,FREQ,RATE dev wifi`).
* **ppp / slip / serial / ser2net** → `net_link` bucket + driver in
  `net_link_detail` (e.g. `ppp0`, `sl0`).
* **tunnel** (gre, ipip, sit, vxlan, geneve, bare tun/tap not owned by
  a VPN daemon) → `net_link: "tunnel"`, kind in `net_link_detail`.
* **vpn** (wireguard, openvpn, or tun/tap owned by a VPN process per
  `ss -tulpn`) → `net_link: "vpn"`, daemon in `net_link_detail`.
* **virtual** (bridge, bond, veth, vlan, macvlan, ipvlan, dummy) →
  `net_link: "virtual"`, kind in `net_link_detail`. Underlying physical
  speed is intentionally null — walk bridge members manually if needed.

Wifi and tunnels add jitter a wired link doesn't, so a wifi/VPN node
should not be the sole signal for KEEP/DISCARD on latency metrics —
escalate when only wifi/VPN/tunnel nodes show movement.

### Record bare-metal / container / VM and arch emulation

For every exec node, also capture:

* `virt_kind` — `bare-metal` / `container` / `vm`.
* `virt_type` — `none` / `lxc` / `docker` / `podman` / `systemd-nspawn`
  / `kubernetes` / `qemu` / `kvm` / `vmware` / `virtualbox` / `xen` /
  `wsl` / `unknown`. Read via `systemd-detect-virt --container` and
  `systemd-detect-virt --vm`; fall back to `/proc/1/cgroup`,
  `/.dockerenv`, `/proc/sys/kernel/osrelease` (WSL marker) and the
  `hypervisor` CPU flag.
* `arch_emulated` — `true` when `uname -m` disagrees with the CPU
  family implied by `/proc/cpuinfo`. qemu-user-static (e.g. an x86
  host running a `mips`-rootfs LXC container via binfmt_misc) returns
  the emulated arch from `uname -m` while `/proc/cpuinfo` still
  surfaces the host CPU — that mismatch IS the signal. An emulated
  arch is typically 5×–50× slower than native silicon; a benchmark
  unaware of this will conclude "MIPS is glacial" when in reality
  it's measuring qemu-user overhead.
* `host_arch` — the underlying CPU arch when `arch_emulated` is true,
  else null.

These fields are MANDATORY in every per-run JSON. The shared helper
`history_recorder.collect_virt()` writes them; benchmarks do not need
to (and must not) reimplement detection.

### Why every arch, every time

Local-only is never enough. An optimisation that helps amd64 but
regresses MIPS is NOT a win — the project targets constrained
hardware (i486 66 MHz, Geode LX800, MIPS2, RISC-V, RPI1) and amd64
results in isolation say nothing about those. Skipping a noisy node
is fine; skipping an arch is not — escalate so the human knows the
candidate hasn't actually been validated across the fleet.

## `benchmark` flag on execution_nodes

Added to every `execution_nodes[]` entry in `remote_nodes.json`
(MANDATORY field — see `../test/CLAUDE.md` "All remote_nodes.json
fields are mandatory"). Boolean.

* `true` — node opts into the benchmark harness. Operator has
  confirmed the box is quiet enough for stable measurements (no
  competing tenants, CPU governor pinned to `performance`, turbo
  disabled if you want stricter reproducibility).
* `false` — node is excluded from benchmark runs. Default on every
  new node.

Independent of `sanitizer_*` flags: a node can build+run for
correctness every commit and still set `benchmark: false` because
its CPU is noisy.

## Zero impact on production binaries — non-negotiable

Benchmark code MUST NOT add overhead to a non-benchmark build. Two
acceptable mechanisms, in this priority order:

1. **Class override.** Define the hot code as a virtual base + ship a
   plain non-virtual production implementation; the benchmark
   subclass overrides the hooks. Production code links against the
   non-virtual concrete class — the v-table never enters the picture.
   When the hot path is already non-virtual (and bumping it to virtual
   would itself be the regression), this option doesn't apply — fall
   through to #2.
2. **`#ifdef CATCHCHALLENGER_BENCHMARK`.** Define is only set on the
   benchmark build. Inside the guard you may add counters, RDTSC reads,
   perf-event setup, allocator hooks, anything. Outside, the code path
   compiles to nothing.

Forbidden: `if (g_benchmark_enabled) { … }` runtime checks on a hot
path — the branch alone is a regression on the constrained targets.

## Multi-profiler — cross-product, not pick-one

A single profiler gives one perspective. Use several and combine
their verdicts, because each tool has blind spots:

* `perf stat` — cycles, instructions, IPC, branch misses, LLC misses.
  Lightweight, always-on baseline.
* `perf record` + `perf report` — top hot symbols and call chains
  (sampling). Reveals where the cycles actually land.
* `callgrind` / `cachegrind` — instruction-accurate, slow (~30×) but
  noise-free. Best for "did the change move instruction count?"
* `heaptrack` — allocation profile. Allocator churn doesn't show up
  in `perf stat` cycles.
* `/usr/bin/time -v` — peak RSS, peak VSZ, page faults, ctx switches.
  Free, runs everywhere, doesn't need root.
* `rusage()` snapshot at end of binary — same fields, no external
  tool, robust on minimal-rootfs nodes.
* Static metric — stripped binary size (`size`, `wc -c`). Targets
  with 32 MiB RAM care; an optimisation that swells the binary 2×
  is a regression on those nodes even if the runtime is faster.

**Every benchmark must run under every profiler that is meaningful for
its workload.** If a candidate is better under `perf` but worse under
`callgrind` (= same cycle count, more instructions), that's the
ESCALATE row in the decision matrix.

## SIMD across architectures

Two hard rules:

1. **Generic stays generic.** Never compile SIMD intrinsics into the
   generic build. The generic binary is what ships when the host
   doesn't advertise the feature — pollute it and you break the
   targets we exist for.
2. **Runtime detection, not build-time gating.** A single fat binary
   reads `cpuid` (x86_64), `getauxval(AT_HWCAP)` (arm/aarch64/mips/
   riscv), `__builtin_cpu_supports` (clang/gcc helpers) and dispatches
   to the matching code path. The generic implementation stays the
   tail-of-jump-table fallback. This way the same binary serves
   amd64-v1 → amd64-v4 / armv7 → arm-sve / riscv-rv64gc → riscv-rvv,
   and downstream packagers don't have to ship N binaries.

SIMD have to be detected at runtime at startup and not impact performance of generic code path.
SIMD is architecture-specific: x86 has SSE2/SSSE3/SSE4.1/AVX/AVX2/
AVX-512; arm has NEON/SVE/SVE2; mips has MSA (mips32r5+); riscv has
the V extension. A candidate that adds x86 AVX2 says nothing about
arm/mips/riscv — it must be measured on each architecture in the
fleet that has a SIMD tier of its own, with the runtime-dispatch
table extended for that arch.

**Test matrix is the full product, assembled dynamically per batch:**

```
{profiler} × {generic, SIMD-tier-1, SIMD-tier-2, …} ×
    {arch} × {workload-size} ×
    {exec_node where benchmark=true AND loadavg(1m) < 1.0}
```

Rows are generated per-batch, not at startup — a busy node drops
out of that batch's matrix, its parent compile node still builds
the variant for *other* exec nodes downstream. A candidate is
"kept" only if it passes the decision matrix across the entire
realised product, not on the host's preferred row.

### compilation
* Fix util it compile on all arch (then generic code path always have to work), if you put SSE intrasic into ARM/MIPS it fail compile, then you have to #ifdef x86 the SSE code

### Progress display — mandatory live counter

The harness MUST emit a one-line progress update for every cell of
the matrix as it starts, exactly this format:

```
[bench] 99/200 done — profiler: callgrind, SIMD: no, execution_node: mips-lxc
```

Fields:
* `done/total` — `total` is the realised matrix size *after* dropping
  load-gated skips for this batch (not the theoretical max). `done`
  increments after each cell finishes (PASS, SKIP, or FAIL).
* `profiler: <name>` — `perf-stat` / `perf-record` / `callgrind` /
  `cachegrind` / `heaptrack` / `time-v` / `rusage` / `binary-size`.
* `SIMD: no` for the generic-build row, otherwise the tier name
  (`SIMD: avx2`, `SIMD: neon`, `SIMD: rvv`, `SIMD: msa`, …).
* `execution_node: <label>` from `remote_nodes.json` (or `local` for
  the host row).

A skipped cell still prints, with reason appended:

```
[bench] 100/200 done — profiler: heaptrack, SIMD: no, execution_node: atom-n455  [SKIP load=1.7]
```

Unattended batches run for hours; without this line the operator
can't tell whether the harness is wedged or just slow. Match the
field labels exactly — the line is also `grep`-able for post-mortem
("which cells skipped on this run?"). Keep it one line, no ANSI
colour codes inside the data fields (colour the prefix only —
green for PASS, yellow for SKIP, red for FAIL — so the line stays
greppable when piped through `tee` to a logfile).

## Per-benchmark metadata — headless flag MANDATORY in the source

Every benchmark source file MUST carry a leading comment block stating:

* **`// HEADLESS: yes | no`** — `yes` for benchmarks that run without
  any display server (CLI servers, libcatchchallenger, datapack
  parse, …); `no` for benchmarks that need an X11/Wayland display
  (Qt widgets paint, OpenGL frame, MapVisualiser render).
  Non-headless benchmarks are skipped automatically on
  `client_run_mode == "none"` / `has_gui == false` execution nodes.
* What is being measured (one sentence).
* The metric(s) it emits — name + unit + lower-is-better/higher-is-
  better — so the comparator doesn't have to infer.
* Whether the benchmark is deterministic (same input → same output)
  or sampled (noise band given explicitly).

## Champion baseline + history — per-benchmark, not per-node

The champion is **per-benchmark**, not per execution node. The decision
is whether a change is better for the whole fleet — a regression on any
platform triggers DISCARD, an improvement on any platform can trigger
KEEP (per the decision matrix above). One champion per benchmark lives
at `benchmark/results/<benchmark-name>/champion.json`. Schema:

```
{
  "commit":  "<sha>",
  "comment": "...",
  "date":    "<ISO-8601>",
  "batch_id": "...",
  "benchmark": "<benchmark-name>",
  "nodes": {
    "<execution_node label>": {
      "arch":    "<x86_64 | armv6 | mips | ...>",
      "metrics": {
        "<metric-name>": { "median": <n>, "stddev": <n>, "unit": "<u>",
                           "better": "lower" | "higher" }
      }
    },
    "rpi-zero-w": { "arch": "armv6", "metrics": { ... } },
    ...
  }
}
```

History is git-tracked; champion.json is overwritten in-place when a
new champion is promoted (the `git log` of the file IS the history).
Don't keep a parallel JSON list — it desync's.

When proposing an optimisation, the agent reads the per-benchmark
champion.json, compares the candidate's metrics from **every** node
against the champion's records for those nodes, and writes the decision
(KEEP / DISCARD / ESCALATE) + per-metric deltas back to
`benchmark/results/<benchmark-name>/candidate-<stamp>.json`
(`<stamp>` = the run's started_utc, `:`→`-`). The candidate file is
git-ignored except for the rare ESCALATE case the operator wants to
preserve for discussion.

## Per-run history — append-only, one JSON per run

In addition to `champion.json` (which only tracks the current winner),
every `benchmark*.py` (or the shared helper they all call) MUST drop a
full snapshot of each run under:

```
benchmark/history/<benchmark-name>/<compile-node>/<exec-node>/<ISO-8601-timestamp>.json
```

* `<compile-node>`/`<exec-node>` — the `nodes[].label` /
  `execution_nodes[].label` pair from remote_nodes.json (local host =
  `local/local`). The cpu-model-slug is no longer in the path — it is
  inside the JSON; the node pair is the stable identity.
* `<ISO-8601-timestamp>` — UTC, second resolution, `:` replaced with
  `-` so the path is portable (e.g. `2026-05-14T13-42-07Z`).

**One file per (benchmark, run, platform)** — each execution_node
exercised in a batch gets its own JSON file. Never overwritten,
never rotated. History grows append-only; the directory IS the
timeline.

A single batch that touches N platforms drops N files under
`benchmark/history/<benchmark-name>/` — same timestamp, different
`<compile-node>/<exec-node>` dir. Don't bundle platforms into
one file: it forces readers to parse the whole batch to inspect a
single arch, and makes per-platform diffs (`git log -p <file>`)
useless.

### Mandatory JSON fields (human-readable: 2-space indent, sorted keys)

Schema (one platform per file):

```
{
  "benchmark":   "<benchmark-name>",
  "commit":      "<git sha, full 40-char>",
  "commit_short":"<7-char>",
  "started_utc": "<ISO-8601>",
  "ended_utc":   "<ISO-8601>",
  "harness_version": "<git sha of benchmark/ helper at run time>",
  "batch_id":    "<shared across every per-platform file of the same run>",
  "node":       "<execution_node label from remote_nodes.json>",
  "arch":       "<x86_64 | aarch64 | mips | riscv64 | i686 | ...>",
  "cpu_model":  "<verbatim /proc/cpuinfo model name>",
  "cpu_cores":  <int>,
  "cpu_mhz":    <float>,
  "ram_total_mb": <int>,
  "ram_type":   "<DDR3-1600 | LPDDR4-3200 | unknown>",
  "disk_root":  "<vendor + model of the device backing /, from lsblk/smartctl>",
  "disk_kind":  "<nvme | ssd-sata | hdd | emmc | sd | tmpfs>",
  "net_card":   "<lspci/lshw model string for the primary NIC>",
  "net_link":   "<wired | wifi | ppp | slip | tunnel | vpn | serial | virtual | loopback | unknown>",
  "net_link_detail": "<driver/protocol token: wireguard | openvpn | gre | vxlan | tun | tap | bridge | ppp0 | ser2net | ... | null>",
  "wifi_ssid":  "<SSID if net_link==wifi, else null>",
  "wifi_standard": "<wifi-4 (802.11n) | wifi-5 (802.11ac) | wifi-6 (802.11ax) | wifi-6e | wifi-7 (802.11be) | null>",
  "wifi_band":  "<2.4GHz | 5GHz | 6GHz | null>",
  "wifi_link_mbps": <int negotiated link rate, or null>,
  "eth_link_mbps": <int negotiated wired link rate Mbps (100/1000/2500/10000/...), or null>,
  "virt_kind":  "<bare-metal | container | vm>",
  "virt_type":  "<none | lxc | docker | podman | systemd-nspawn | openvz | kubernetes | qemu | kvm | vmware | virtualbox | xen | microsoft | wsl | unknown>",
  "arch_emulated": <bool — true when uname -m disagrees with /proc/cpuinfo (qemu-user-static binfmt, etc.)>,
  "host_arch":  "<underlying CPU arch when arch_emulated==true (e.g. 'x86_64' inside a mips-emulated container), else null>",
  "kernel":     "<uname -r>",
  "libc":       "<glibc 2.39 | musl 1.2.5 | ...>",
  "compiler":   "<gcc 13.2 | clang 18 | ...>",
  "compile_flags": ["-O3", "-DCATCHCHALLENGER_EPOLL", "-DCATCHCHALLENGER_IO_URING", "..."],
  "simd_tier":  "<generic | sse4.2 | avx2 | neon | sve | msa | rvv | ...>",
  "loadavg_1min_at_start": <float>,
  "results": {
    "<profiler-or-tool-name>": {
      "metrics": { "<metric>": { "value": <n>, "unit": "<u>",
                                 "better": "lower" | "higher",
                                 "samples": [<n>, <n>, ...],
                                 "median": <n>, "stddev": <n> } },
      "subbenchmarks": {
        "<label e.g. '10-players' | '50-players' | '200-players' | 'small' | 'medium' | 'large'>": {
          "cpu_percent": { "value": <float, 0..100 — server is single-threaded, 100 = one core saturated>,
                           "unit": "%", "better": "lower",
                           "samples": [<n>,...], "median": <n>, "stddev": <n> },
          "<other-metric>": { ... same shape ... }
        }
      },
      "artifact": "<relative path under benchmark/history/.../artifacts/ if any>",
      "status":   "PASS | SKIP | FAIL",
      "skip_reason": "<only if status==SKIP, e.g. 'load=1.7'>"
    }
  }
}
```

`batch_id` (e.g. a UUID or the run's start-timestamp) is the only
way to re-stitch per-platform files back into a single batch view —
required because the filename alone can't disambiguate two batches
that hit the same node in the same second on different commits.

Every profiler / tool that ran on that platform gets its own entry
under `results` (`perf-stat`, `perf-record`, `callgrind`,
`cachegrind`, `heaptrack`, `time-v`, `rusage`, `binary-size`, …).
Missing fields are written as `null`, not omitted — readers should
not have to guess between "unknown" and "the harness forgot".

Writers MUST emit with `json.dumps(..., indent=2, sort_keys=True)`
(or equivalent) so a human can diff two runs by eye and `git diff`
stays line-oriented.

### CPU% per sub-benchmark — MANDATORY

When a benchmark sweeps a workload axis (player count 10/50/200, map
size small/medium/large, packet rate 1k/10k/100k, …), every slice
MUST record a `cpu_percent` metric (unit `%`, `better: "lower"`)
alongside its primary metric inside that slice's `subbenchmarks`
entry. Without it, a throughput "win" is ambiguous: did the patch
do more work per cycle, or did it just burn more CPU to push the
same packets? The constrained targets care about both — a 30 %
throughput gain that takes the i486 from 70 %→95 % CPU is NOT a
win; it eats the headroom that keeps tick latency stable.

**The CatchChallenger server is intentionally single-threaded
(epoll event loop, simple + cache-friendly + portable to i486
class hardware), so the server's `cpu_percent` is bounded at
100 %.** 100 % means one core fully saturated — the server cannot
go faster on that node, only on a faster core. A reading above
100 % indicates the metric was taken from the *wrong* process
(measurement bug: probably timed the whole bot-client tree
including N worker bots instead of the single server PID) — treat
it as FAIL, not as "uses more cores". Capture from
`getrusage((ru_utime+ru_stime)/wall)*100` of the server PID, or
`/usr/bin/time -v` wrapping ONLY the server binary, not the bot
fleet. Reaching 100 % at the largest workload is the expected
saturation signal — that's the point where added load starts
costing latency.

Capture CPU% from:
* `/usr/bin/time -v` → "Percent of CPU this job got: 99%" — easiest,
  no extra dependency, works on every node.
* `getrusage(RUSAGE_SELF)` at end of the binary → `(ru_utime +
  ru_stime) / wall_clock * 100` — when the binary is benchmark-aware
  and already prints rusage.
* `pidstat -p <pid> 1` for long-running servers — sample throughout
  the slice, record median + p95.

Don't synthesize the field from aggregate rusage — every slice gets
its own measurement, not a divided global. The helper provides
`PlatformRecord.add_subbenchmark(tool, label, metrics)` exactly for
this; benchmark scripts call it once per workload point.

### Helper, not copy-paste

This collection logic lives in **one shared helper** under
`benchmark/` (e.g. `benchmark/history_recorder.py`) imported by every
`benchmark*.py`. The list of fields above is long enough that
duplicating it per benchmark guarantees drift — and a missing field
in one file silently breaks any future analysis script that joins
across the timeline. Add new fields to the helper, not the benchmark.

## Progression charts — per-node visualisations of per-benchmark history

After each batch, the harness MUST regenerate a chart per
`(benchmark-name, compile-node, exec-node)` tuple plotting every
metric over commits (x-axis = commit date or commit index in
chronological order, y-axis = metric value, one line per metric, dual
axis when units differ). Source data is the append-only per-run JSONs
under `benchmark/history/<benchmark-name>/<compile>/<exec>/`; group by
the `<compile>/<exec>` node pair (remote nodes included, not just the
local host). The champion commit marker comes from the per-benchmark
`champion.json` at `benchmark/results/<benchmark-name>/champion.json`.

Output path — charts live per-node (they visualise per-node history):

```
benchmark/results/<benchmark-name>/<compile>/<exec>/champion.svg
benchmark/results/<benchmark-name>/<compile>/<exec>/candidate-<stamp>.svg
```

In addition to per-node charts, the harness also generates a
**cross-node session chart** that groups data by `batch_id` (one
benchmark run across all platforms = one session), showing one line
per node within each metric panel:

```
benchmark/results/<benchmark-name>/champion.svg
benchmark/results/<benchmark-name>/candidate-<stamp>.svg
```

This lets the reviewer see at a glance whether an optimisation helps or
regresses every platform in a single chart, without flipping through
per-node SVGs.

`champion.svg` is the always-current progression chart; a benchmark
run also freezes the same chart as `candidate-<stamp>.svg`
(`<stamp>` = run started_utc, `:`→`-`).

Rules:
* SVG only (text, diff-able, no binary churn in git). PNG is forbidden.
* Regenerate from scratch each batch — never append to an existing
  SVG. The history JSONs are the source of truth; charts are derived.
* Annotate the current champion commit with a marker so a reviewer can
  see at a glance which point is the baseline. The champion commit is
  read from the per-benchmark `champion.json` (same across all nodes).
* Mark KEEP / DISCARD / ESCALATE decisions on the corresponding commit
  with distinct glyphs (green ▲ / red ▼ / yellow ◆).
* Chart generation lives in the shared helper (next to
  `history_recorder.py`), not duplicated per `benchmark*.py`.
* No external chart service — render locally (matplotlib SVG backend
  or hand-rolled SVG). Don't add a new pip dep without asking.
* `champion.svg` is git-tracked.
  `candidate-<stamp>.svg` is git-ignored like
  `candidate-<stamp>.json` (transient; regenerable from the history
  JSONs, which remain the source of truth).

### Generating charts manually

After a run, charts are regenerated automatically by every
`benchmark*.py`. To rebuild them out-of-band (e.g. after pulling new
history JSONs from another machine, or after editing
`chart_generator.py` itself), invoke the helper directly:

```
# every benchmark that has history JSONs
python3 benchmark/chart_generator.py

# one specific benchmark (matches benchmark/history/<name>/)
python3 benchmark/chart_generator.py benchmarkmapmanager

# multiple benchmarks in one call
python3 benchmark/chart_generator.py benchmarkmapmanager benchmarkserversave
```

Exits non-zero only when no history was found at all — a benchmark
with an empty history dir is silently skipped (a future run will
populate it).

## Workload variety — small / medium / large

A single workload size hides allocation-pattern regressions. Each
benchmark should expose at least:

* **small** — fits in L1, no allocator interaction. Tests the pure
  instruction pipeline.
* **medium** — touches L2/L3, hits allocator. Closest to typical
  per-tick server load.
* **large** — exceeds L3 / spills to RAM-bandwidth-limited regime.
  Tests memory-traffic optimisations, prefetching, SIMD wins on
  bulk loops.

An optimisation that's a big win on `large` but a regression on
`small` is the textbook ESCALATE case — usually a sign of a
constant-factor tradeoff that may or may not be acceptable
depending on actual call-site mix.

## Latency vs throughput

Both are first-class metrics. Don't collapse to "ms/op".

* **Throughput** — ops/sec under sustained load. The headline number
  for batch workloads (datapack parse, map preload).
* **Latency** — p50 / p95 / p99 / p999 distribution. The headline
  number for hot tick paths (`EventLoop::wait` callback, packet
  parse). A p99 regression at unchanged p50 is a real regression
  even if throughput is flat.

Most network/server hot paths care more about p99 tail than mean
throughput. Most preload/once-per-boot paths care only about
throughput.

## Fixed-time, not fixed-iteration

Every benchmark runs for a fixed wall-clock budget (e.g. 10 min) and
reports the work done in that window (iterations/ops completed,
higher-is-better) — NEVER a fixed iteration count timed to completion.
A fixed 1000-iteration loop just takes longer on a slow arch (same
count), hiding throughput regressions and making cross-arch numbers
incomparable; the time budget makes "ops in 10 min" directly comparable
across i486 → amd64 and surfaces a regression as fewer ops.

The per-cell/per-run TIMEOUT is DERIVED from that budget, not picked
independently: `timeout = budget + margin` (margin covers startup +
teardown + the final in-flight iteration). The workload itself stops at
the budget (the binary self-times, or the harness sends SIGTERM/SIGINT
at the budget and counts completed iterations) — the timeout is only the
hard backstop for a hung run, so it must always exceed the budget. A
profiler that inflates wall-time (callgrind ~30×) scales its budget AND
its timeout by the same factor so it still measures the same amount of
work.

## Auto-revert on REGRESSION

When the decision matrix says DISCARD, revert the patch (then each changes have to be small, under 200 lines). so the next iteration starts from
the champion again. Otherwise small regressions stack up across
iterations and the agent ends up "optimising" away from where it
started without anyone noticing. ESCALATE leaves the patch in
place since the human is going to look at it.

## Don't optimise into the noise floor

If the median delta is within ±10 stddev of the noise band on every
node, the change is statistically meaningless. The decision matrix
treats it as "all equal" — neither KEEP nor DISCARD on its own
merits. If the change is otherwise neutral (code clarity, smaller
binary, removed dependency), promote it as a non-perf clean-up via
the normal commit path, not via the benchmark champion mechanism.

## What NOT to put here

* New testing*.py scripts — those live in `test/`. Benchmarks are
  perf measurement, not correctness verification.
* Production code that happens to be perf-sensitive. The benchmark
  workspace exists to *measure* prod code from outside; it doesn't
  ship.
