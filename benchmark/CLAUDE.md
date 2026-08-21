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
* **Every benchmark*.py that builds the server uses the RAM DB**
  (`-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON`): the file-DB tree lives in
  /dev/shm (tmpfs), never disk, so no score is polluted by real disk I/O.
  DB_INTERNAL_VARS implies DB_FILE in CMake — pass only DB_INTERNAL_VARS,
  not DB_FILE. `CATCHCHALLENGER_DB_FILE` only governs player/character
  persistence; the datapack HPS cache path is independent of it, so the
  serversave datapack-cache measurement is unaffected by the DB backend.
  serversave builds at -O2 (not the Release default -O3).
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

* `--profile` on any `benchmark*.py` runs that benchmark's command under
  a profiler on the LOCAL host AND on every benchmark exec node
  (`benchmark: true`, loadavg<1.0 — built on the matching compile node,
  artefact rsync'd back). Bare `--profile` (= `--profile all`) runs BOTH
  callgrind and perf; pass a single tool to narrow. Each artefact is
  written under `/mnt/data/perso/tmpfs/cc-bench-profile/` and filename-
  tagged `<node>-CPU-<cores>` (e.g. `perf.data.rpi-zero-w-CPU-4.<stamp>`)
  so per-node profiles never collide. A node missing the profiler tool
  is SKIPped, not failed. Server+bot benchmarks whose remote dispatch
  isn't wired (botactions) profile local-only and say so. Open the path
  and work the hot spots; reach for heaptrack/cachegrind for more detail
  than the benchmark records.
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
* NEVER assume a board compiles: `execution_nodes[]` only RUN. Builds happen on the parent compile node (e.g. the armv6 one is a 32-core ARMv8) or a local cross toolchain (`/mnt/data/perso/progs/catchchallenger-arm-softFPU/` for rpi-zero-w).
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

### Post-run sensors / thermal-throttle detection

Every per-run JSON carries a `sensors` object written post-workload by
`history_recorder.collect_sensors()` (benchmarks must not reimplement it).
It answers "are these numbers heat-tainted?" from sysfs (works with no
tool installed): temps + trip points, CPU clock cur/max + scaling cap,
x86 `thermal_throttle` counters, and Pi `vcgencmd get_throttled`
live+sticky bits (also catches under-voltage). Verdict keys:
`thermal_throttling_active` (throttling at sample time),
`throttled_since_boot` (cumulative context), `delta.throttled_during_run`
(when a pre-workload `sensor_baseline()` was captured — the strong
per-run signal), `trend` (sensor-free: a work metric decaying across the
fixed-time window), and `thermal_throttling_suspected` (OR of the per-run
signals). A suspected run's metrics are not a clean KEEP/DISCARD signal —
treat like a contended node and re-measure. setup.py provisions `sensors`
+ `vcgencmd` + `cpupower` (availability-gated; sysfs is the fallback).

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

**Prefer NOT changing prod code at all.** A benchmark lands in `benchmark/`
(the harness) and drives/observes the prod binary EXTERNALLY (network, serial,
the bot CLI) — editing `general/`/`client/`/`server/`/`tools/` risks stability
and grows the codebase. Only when a measurement is impossible from outside add
prod code, and then ONLY behind `#ifdef CATCHCHALLENGER_BENCHMARK` / a class
override (below). A real prod BUG fix is still allowed.

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
* A benchmark must NEVER report FAIL on every cell. Local compile failure aborts the whole `benchmark*.py` (non-zero exit, before any cell). A remote compile-node build failure skips ONLY its child execution nodes (recorded SKIP — an unmeasured node is unknown, never a regression); the fleet continues on other nodes. Infra failures (push/unreachable node, bring-up, datapack rsync, cache stage, server won't launch) are SKIP, not FAIL. FAIL is reserved for a benchmark that actually ran and produced bad/garbled data.
* EVERY error (local compile, remote compile, push, bring-up, server-start, bad result) MUST print the full captured cause under a clear banner AND the exact command to re-run JUST this benchmark (so the operator reproduces one benchmark, not the whole fleet). The live one-line progress counter keeps a truncated reason; the banner carries the full text. Use the shared helpers `bh.print_local_build_error()` (aborting local builds) and `bh.print_node_error()` (per-node remote/infra), both of which append `bh.rerun_command()` — never hand-roll the banner per benchmark.

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

`benchmark/history/` and `benchmark/results/` are MOSTLY local. `.gitignore`
is the authority; today it tracks exactly three distilled files per location —
`results/**/champion.json`, `history/**/series.json` and
`history/**/platform.json` — so the performance evolution and the hardware
comparison live in git without the bulk, and untracks the per-run timeline and
every chart (regenerable, and the SVGs alone would be hundreds of MB).
champion.json is overwritten in-place when a new champion is promoted; the
history JSONs are the source of truth and charts are regenerated from them.
Don't keep a parallel JSON list — it desync's, and don't widen what is
tracked.

When proposing an optimisation, the agent reads the per-benchmark
champion.json, compares the candidate's metrics from **every** node
against the champion's records for those nodes, and writes the decision
(KEEP / DISCARD / ESCALATE) + per-metric deltas back to
`benchmark/results/<benchmark-name>/candidate-<stamp>.json`
(`<stamp>` = the run's started_utc, `:`→`-`). Only `champion.json` is tracked
under `results/`; everything else there is local, so an ESCALATE the operator
wants to keep is copied out of the tree by hand.

## Per-run history — one distilled pair per node

In addition to `champion.json` (which only tracks the current winner),
every `benchmark*.py` (or the shared helper they all call) records each run
under:

```
benchmark/history/<benchmark-name>/<compile-node>/<exec-node>/series.json
benchmark/history/<benchmark-name>/<compile-node>/<exec-node>/platform.json
```

`series.json` carries that node's WHOLE timeline as parallel arrays (a runs
axis + one column per metric, ~2.3 bytes per datapoint); `platform.json` is
its machine description, rewritten only when it changes. Written by
`history_series.py`, which every benchmark calls at the end of its run. This
REPLACED an earlier one-JSON-per-run-per-platform layout
(`<ISO-8601-timestamp>.json`): the fields below are what the pair carries, but
do not expect a file per run — there are none, and reading the evolution needs
no `git log -p` archaeology.

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
  "cpu_flags":  "<first /proc/cpuinfo flags (x86) / Features (arm,riscv) line>",
  "cpu_cache":  [{"type":"Data|Instruction|Unified","level":<int>,"size":"<e.g. 32K>","line_size_bytes":<int>}, ...],
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
  "libs":       {"zlib|zstd|blake3|xxhash|tinyxml2": {"source": "system | vendored", "version": "<x.y.z | null>"}},
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
* `champion.svg` and `candidate-<stamp>.svg` are BOTH untracked:
  regenerable from the history JSONs, which remain the source of truth.
  `champion.svg` is rewritten by every run but only when the bytes actually
  change; the `candidate-<stamp>.svg` freeze is opt-in
  (`CC_BENCH_WRITE_CHARTS=1`), and `python3 benchmark/svg.py` renders any
  chart on demand without persisting it.
* Skip charts that can't drive a cross-node decision (they only waste
  space); regenerate must delete a now-skipped stale SVG:
  * cross-node `champion.svg` + `champion-by-execution-node.svg`:
    skip entirely when <2 distinct nodes have history.
  * `champion-by-execution-node.svg`: drop any metric panel whose data
    covers <50% of the nodes (ceil(n/2)); skip the chart if no panel left.
  * cross-node `champion.svg`: drop any per-node series with <3 points
    (a lone dot is no trend); skip the chart if no series left.
* **A/B benchmark = both arms in ONE panel, never one panel per arm.**
  Auto-detected (`_ab_arms`): 2..4 sub-benchmark slices all reporting the
  SAME metrics are arms (epoll/iouring), not workload points. Then
  `champion-by-execution-node.svg` draws one metric panel with a bar GROUP
  per node — one bar per arm, arm-coloured with a legend, both medians
  printed above the group plus the arm-B-vs-arm-A delta — and the cross-node
  `champion.svg` plots the arm slices it otherwise skips (for an A/B they
  ARE the measurement, so skipping them left that chart empty).

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

## CPU%/RSS are never a conclusion on their own

A CPU% or memory delta is meaningless without the work-done metric it
rode on. Always contrast resource usage against the throughput/latency
result for the SAME slice — requests/s, time to process X players, time
to save, ticks/s, ops in the budget. "Variant X uses less CPU" says
nothing until paired with "…at the same or better requests/s"; less CPU
because it did less work is a regression, not a win. Every conclusion
states both: the work metric AND its resource cost.

The server-side work + tail metrics come from the `CATCHCHALLENGER_BENCHMARK`
build (server/cli, `-DCATCHCHALLENGER_BENCHMARK=ON`): a per-read-event
counter + log2-ns latency histogram dumped as `BENCH <k>=<v>` lines on
SIGINT (async-signal-safe writes). benchmark*.py derives requests/s,
p50/p95/p99/p999 and jitter from it. The whole block is `#ifdef`'d out
of production — zero cost when the define is off. Never report req/s
without its latency tail + jitter: a throughput win that inflates p99 or
jitter is a tick-stability regression on the constrained targets.

The same build carries the event-loop self-probe
(`benchmark/BenchProbe.hpp`): `BENCH loop_busy_us/loop_wall_us/
loop_iterations`, TIME units only (us — cross-arch comparable, never raw
cycles; ESP32 CCOUNT is converted at dump time). `bh.parse_loop_selfprobe()`
derives `loop_busy_pct` + `loop_us_per_wakeup`. This is the profiler tier
for install-nothing platforms (ESP32, OpenWrt): ESP32 has no signals, so
the firmware dumps to UART every 60 s — counters are cumulative, the LAST
dump covers the run.

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

## Target the long-lived server, not startup

The CatchChallenger server runs for hours/days; the metric that matters
is steady-state per-tick cost, NOT one-time process startup. Profiles
must reflect that:

* **Exclude startup from the profile.** A short callgrind cell (small
  `--ticks`) is swamped by dynamic linking — `do_lookup_x`, `strcmp`
  on symbol names, `_dl_relocate_object`, `_dl_lookup_symbol_x` can be
  ~70% of a single-core i386/armv6 run. That is the loader, not the
  server. Use `--collect-atstart=no` + `CALLGRIND_TOGGLE_COLLECT`
  around the work loop (or warm/long enough that startup amortises to
  noise) so the numbers are the tick path. The same applies to the
  HPS-cache warm-up: profile a WARM boot, never cold XML parse.
* **One-time costs are not regressions.** Startup dynamic-linking,
  datapack load, relocation are paid once over a multi-hour run —
  weight them ~0 in KEEP/DISCARD. A change that adds startup work but
  cuts per-tick cost is a WIN for the long-lived server even though a
  startup-dominated micro-profile shows it "slower". `-Wl,-z,now`
  (CATCHCHALLENGER_PERF_LINK) is exactly this: front-loads relocation
  to load time to make every later call a direct GOT hit.
* Steady-state hot paths (e.g. `MapVisibilityAlgorithm::min_network`,
  packet parse) are the optimisation target; boot-path symbols
  (tinyXML2 parse, `listFolderNotRecursive`, dl-* loader) are noise
  unless the benchmark is explicitly a boot-time benchmark.

## 250 bots per map is the protocol ceiling

The per-map player index on the wire is 8-bit with reserved values
(`SIMPLIFIED_PLAYER_ID_FOR_MAP`), so **250 connected bots is the safe number to
compute with**, whatever `mapVisibility/Max` says. At 250 on ONE map every bot
still broadcasts its movement to all the others, which is the load the event
loop is being measured on. Above 250, split them so no single map holds more
than 250 — otherwise the run breaks instead of scaling.

Spreading over several maps changes what is being measured: the broadcast stops
being "everyone sees everyone", so the work per bot drops and the number is no
longer comparable with a single-map run. Generate those maps with
`tools/map-procedural-generation/` — that is what it is for — and treat the MAP
COUNT as a calibration input, recorded alongside the bot count, not as a detail.

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

## The load client runs ON the node under test — always

The load generator and the server share the hardware being measured. Only
`tools/bot-bench` can do this fleet-wide: it is Qt-FREE, while `tools/bot-actions`
links Qt6 Widgets and 12 of the 16 benchmark exec nodes are headless boards with
no Qt6 runtime. `benchmarkbotactions.py` (`--spam` throughput) and
`benchmarkclientlatency.py` (`--latency` tail) both build it on the exec node's
compile parent, push it beside the server and run it there over 127.0.0.1.
Driving from the host instead records the HOST process's rusage under the NODE's
label, cannot produce a server CPU%, and measures the host↔node link. Shared
recipe: `br.build_bot_bench_on_compile_node()` / `push_bot_bench_to_exec()` /
`run_client_on_exec()` (one ssh carries the whole window; the server's
`/proc/<pid>/stat` + `/proc/uptime` are sampled on the node either side of it).

Traps that cost a debugging cycle each — all fixed, don't reintroduce:

* **The HPS datapack cache carries the SETTINGS, not just the datapack**
  (`loadSettingsFromBinaryCache`). An exec-node server OBEYS the cache and
  IGNORES the `server-properties.xml` next to it. So the cache slot key must
  include the port AND a hash of the XML (`pregenerate_datapack_cache(...,
  properties_xml=)`), and a caller needing a non-default setting must pass its
  XML. Symptoms seen: a server binding another benchmark's port
  (ECONNREFUSED on every cell), and a raised chat limit silently not applying.
* **`Client::sendLocalChatText` drops silently past
  `dropGlobalChatMessageLocalClan` per map, and it is NOT behind the DDoS
  ifdef** — the benchmark define does not lift it. The default 20 capped the
  latency benchmark at exactly 20 probe samples on every node, whatever the bot
  count or window. `kickLimit*` are `uint8_t` (>255 wraps STRICTER); the
  `drop*` ones are plain `int`, so ask for far more than the sweep can produce.
* **`build_on_compile_node()` does NOT stage the source** — it compiles
  whatever is already in `<work>/sources/`. Call `br.stage_source_once()` first
  or the fleet silently measures a previous run's tree.
* **Never run the load client under `gdb`.** The old bot-actions cells wrapped
  it in `gdb --batch` to catch crashes; measured, every cell then blew past a
  180 s backstop for a window that takes 5.5 s bare, and the client never got
  to print its work metric. A crash is still diagnosable from the exit code +
  output tails.
* **`pkill -f '[.]/<bin>'` must be its own ssh call.** `pkill -f` matches whole
  command lines, so a pkill inside the command that also contains `./<bin>`
  kills its own shell and ssh returns 255 with the cell never run.
* Character creation on a fresh RAM DB needs a real skin: rsync the node
  datapack (and pre-generate its cache) with `keep_skins=True`, else the server
  answers "the datapack has no skin, no character can be created" and no bot
  reaches the map.

## What NOT to put here

* New testing*.py scripts — those live in `test/`. Benchmarks are
  perf measurement, not correctness verification.
* Production code that happens to be perf-sensitive. The benchmark
  workspace exists to *measure* prod code from outside; it doesn't
  ship.

## `benchmarkmapmanager2.py` -- two-stage, per-node generated replay

Same production `min_network()` as `benchmarkmapmanager.py`, over the datapack's
REAL world (647 maps under `map/main/generated`), driven by a replay that is
GENERATED PER EXECUTION NODE AND PER DATAPACK.

* **Stage 1** (`stage1/`, runs on the host): loads the world with the
  PRODUCTION `general/base/Map_loader.cpp`, asks the node `/proc/meminfo` how
  much RAM it has and sizes the population from it (~1.7 KB per player, half
  the node's RAM, ceiling 65530 = the 16-bit connected index), spreads it
  60/30/10 over route/town/interior crowded to 35/200/20 per map (**253** hard
  guard per map), SIMULATES the walk with the production predicate
  `MoveOnTheMap::isWalkableWithDirection`, and writes a `.cpp`: map dimensions,
  spawns, movement vectors, migration schedule, and an end-of-cycle state hash.
* **Stage 2** (`stage2/`, the MEASURED binary): compiles that in and replays
  it. No datapack, no map data, no collision test at runtime -- per player per
  tick a countdown plus a coordinate store (~12% of the tick). A vector of 3
  cells is walked one cell per tick for 3 ticks and only then is the next one
  fetched. That is what lets the ESP32 (no filesystem, static player count) run
  the same benchmark.
* **The fleet builds stage 2 per node**: the generated file is rsync'd to the
  compile node OUT of the `--delete`-mirrored source tree, and passed as
  `-DCC_WORKLOAD_CPP`. `run_profiler_fleet` keys its build dirs on the spec's
  `cmake_defs` (added for this), so nodes sharing a compile parent cannot share
  a binary and run someone else's workload.
* **Replay sizing is the whole trade-off**: too short and the run is mostly
  resets (a reset is one tick where the world jumps home -- it showed up as
  +42% p95), too long and a small board carries a table it will never replay.
  Stage 1 aims at a cycle and shortens it only if the bytes do not fit; the
  budget is 2% of the node's RAM (ESP32: a flash constant). Measured: 0.3% of
  ticks are resets.
* **Oracles**: `replay_mismatch` != 0 is a FAIL (stage 2's state after a cycle
  must equal what stage 1 computed, which also proves no vector walked into a
  wall); the `CATCHCHALLENGER_TESTING` x/y guard still checks every broadcast
  coordinate against the map dimensions the workload carries.
* **Do not put the map KIND in stage 2.** It is a stage-1 notion -- it decides
  how many players spawn per map -- and shipping it only grew the workload.
* Traps already paid for in stage 1: `tryLoadMap()` ABORTS unless
  `parseDatapack()` ran first (without it 24 992 real cells come back as
  walls); dimensions arrive on `loader.map_to_send`, not on the destination
  map; outside a `CATCHCHALLENGER_SERVER` build every parsed XMLDocument is
  parked in `CommonDatapack::xmlLoadedFile` and never freed (clear it per map,
  47 MB -> 6.8 MB); `FacilityLibGeneral::listFolder()` order is not stable
  across machines, so sort it; the map kind comes from the sibling `.xml`
  (`<map type=...>`), not from the path (85 of 647 differ).
* `maps`/`resets`/`median_prep_ns`/`changed_pct`/`workload_*` are recorded but
  kept OUT of the champion metric set. ~26% of slots differ per tick, so the
  "same as last broadcast -> send nothing" path stays the majority of the diff
  and an optimisation of it is measurable.

## epoll vs io_uring A/B — `benchmarkepolliouring.py` (learned the hard way)

IN the routine sweep (`all.sh`). It records the same per-platform history +
charts + champion as the others, tracking BOTH arms. Fixed-TIME like every
other benchmark (a `--seconds` window per cell) and every node measures
CONCURRENTLY -- the client is node-local, so two hosts share no link, CPU or
port; only the builds are capped, per compile node.
The sweep keeps `--reps 3`, which feeds the history trend but is NOT a
publishable separation — quote a win only from a `--reps 8+` run. Single-core
boards (geode, p1mmx) are measured but expect "no separation": the node-local
client takes half the core, so they are scheduler-bound, not syscall-bound.

* **`min..max` separation at n=3 is NOT evidence.** Ranges WIDEN with sample count. A clean non-overlapping geode result at n=3 dissolved at n=4. Use `--reps 8`+; never publish an n=3 separation.
* **io_uring sub-options are NOT optional tuning for a fair A/B.** Without `COOP_TASKRUN`+`TASKRUN_FLAG`+`NO_SQARRAY` (`--tuned`) rpi-4 reported epoll +2.5%; with them io_uring +7.7% — a ~10pt sign flip. Untuned io_uring carries avoidable overhead epoll does not have.
* **SQPOLL measured HARMFUL, do not enable by default** (`--sqpoll`): rpi-4 −8.1% (win destroyed), odroid-n2 −0.4%. It spins a kernel thread costing a core it cannot repay; this server is not submission-bound (broadcast sends are already batched to one `io_uring_enter` per tick). It is also not like-for-like vs single-threaded epoll.
* **IOPOLL cannot affect a network benchmark** — it drives the FILE ring, not the socket ring, and no-ops without O_DIRECT (tmpfs rejects it).
* **`CATCHCHALLENGER_BENCHMARK=ON` is REQUIRED to saturate at all.** `CATCHCHALLENGER_DDOS_FILTER` is derived from `#ifndef CATCHCHALLENGER_BENCHMARK` (`server/base/VariableServer.hpp`) — no independent switch. With the filter in, a saturating client is kicked after ~60 moves. Raising the runtime limits cannot rescue it: they are `uint8_t`, so >255 WRAPS STRICTER.
* **Benchmark builds skip the mapmanagement ping** (`MapVisibilityAlgorithm.cpp`, 5 `#ifdef` sites). The pending-ping check is ACK-based flow control: it makes SERVER throughput a function of the LOAD GENERATOR's RTT. Skipped, not sent ungated — ungated pings exhaust query numbers and the client is kicked for "no free query number".
* **`loop_busy_us` is NOT backend-comparable — never quote it in an A/B.** `CC_BENCH_LOOP_IN/OUT` brackets what happens AFTER `EventLoop::wait()` returns; on io_uring the packet work happens INSIDE `wait()`. `packets_in` and `lat_hist_*` ARE comparable.
* **Client runs node-local; measure contention, don't assume it away.** `/proc/<pid>/schedstat` field 0 = ns on-CPU, **field 1 = run_delay** (ns runnable but waiting). Veto on ASYMMETRIC contention between the two arms, not on the absolute ratio: equal starvation depresses both absolute figures and leaves the RATIO valid.
* **Single-core boards cannot be measured this way.** On the geode the node-local client takes ~half the core, the server tops out at 42% with run_delay ×1.02, and the result is "no separation" both untuned (n=4) and tuned (n=8) — it is scheduler-bound, not syscall-bound.
* **Result, for reference:** io_uring +7.7% (rpi-4), +5.5% (odroid-n2), no separation (geode), all n=8 tuned, non-overlapping, at LOWER server CPU and ~+0.8MB RSS. The old published −9.8% never reproduced under two methodologies and is withdrawn.
* **Why not the >2x the literature reports:** this epoll path already drains up to 62 moves per read syscall (measured `packets_in` vs moves) and per-move application work is ~2.6us, so syscalls are a small fraction. Published multipliers are syscall-throughput microbenchmarks against unbatched baselines, usually with SQPOLL and/or zero-copy receive (impossible here — needs NIC header/data split).
* **Coalescing io_uring's recv does NOT pay — measured, implemented, reverted.** epoll gets 62 moves per read wakeup vs io_uring's 2.5, which looks like a gap to close. It is not: `strace -c -f` on the server shows **1 `recvfrom` + 10 `recvmsg` for a 6.1M-move run** (boot only), i.e. ZERO recv syscalls per move on the io_uring path — there is no syscall to remove, only ~50 userspace instructions per parser entry out of ~1645 per move. A working implementation got 2.0x fewer parser entries (separated) while req/s, server CPU/move and `instructions:u`/move all OVERLAPPED at n=5. ~1% of userspace instructions = noise floor. Reverted per "revert changes that fix nothing".
  * Why the CQEs are thin is TIMING, not capacity: buffers are already 4096 B and carry a measured **3.98 bytes per CQE** (0.097% of one). `recv_multishot` completes from the packet-arrival softirq so a CQE holds one TCP segment; epoll's `recv()` runs a loop iteration later and finds several queued.
  * `IORING_RECVSEND_BUNDLE` is supported here (liburing 2.9) but can NEVER fire: it only grabs extra buffers when a recv overflows one, and 3.98 B never overflows 4096 B.
  * `SO_RCVLOWAT` would genuinely fatten the CQEs but is REJECTED on correctness: a client that sends one small packet and waits for a reply would not be seen until FIN — protocol deadlock — and it inflates the tail latency the constrained targets care about.
  * Non-adjacent CQE merging (hash map keyed by socket) captured no more than merging adjacent runs (the kernel already posts a socket's CQEs back to back) and cost +5.6..+9.0% instructions:u/move, non-overlapping. Do not retry it.
* **epoll's per-dispatch syscalls were CUT 7.8 -> 4.4: `EventLoopClient::read()` is now ONE `recv(MSG_DONTWAIT)`.** It used to ask the kernel twice per read — `ioctl(FIONREAD)` to size it, `fcntl(F_GETFL)` to re-check `O_NONBLOCK` — because the accepted client socket is deliberately left BLOCKING (`main-unix.cpp` disables `make_non_blocking`: a non-blocking socket short-writes the tens-of-KB datapack packets and this backend has no output queue). `MSG_DONTWAIT` is per-CALL non-blocking, so the read needs neither and the write path keeps its blocking full-send. Measured, 250 bots: 7.78 -> 4.41 syscalls/dispatch, 0.1178 -> 0.0693 syscalls/move — epoll now issues FEWER syscalls per move than io_uring (0.0804), not ~28% more.
  * **The win is CPU, and the SIGN of local req/s depends on server load — never read req/s here as the verdict.** Draining faster returns to `epoll_wait` sooner, so each wakeup carries less data: dispatches/move +5..25%. In a co-located closed loop that is extra round trips. n>=5 interleaved, ranges non-overlapping: server 68-80% loaded -> req/s -11.4%, CPU/move -3.3%; server 86% loaded (TWO bot-bench, one core each — one cannot saturate) -> req/s -1.4%, CPU/move -3.69%, work per server-CPU-second +3.84%, server CPU 86.1%->81.7%. The deficit shrinks monotonically as the server saturates, which is the only regime the constrained targets run in. `instructions:u`/move -0.8..-1.5%.
  * **Do NOT co-locate client and server on ONE core to "make CPU scarce".** It reported -30% at n=5 with run_delay/on-CPU 1.598 vs 0.753 — an asymmetric scheduler equilibrium far past RUN_DELAY_SKEW_MAX — and did not reproduce: merely attaching `perf` equalised the arms (identical moves, CPU/move -3.2%). Same trap as the geode row above. A cgroup `CPUQuota` on the server is no better: it caps both arms at the quota, so "CPU per move" degenerates to 1/moves.
* **A CQE is NOT a syscall under `recv_multishot`** — many are harvested per `io_uring_enter` (measured 2.04 recv CQEs per enter, 0.372 enters per move). `bench_packets_in` counts parser DISPATCHES on both backends, so it is comparable as a dispatch count and must never be read as a syscall count.
* **The host IS measured, as the HIGH-CONCURRENCY profile — not as a stand-in for the boards.** 32 cores answering 250 bots is a different operating point from a single-core board, and the two event loops have no reason to behave alike in both; that second profile is the reason its row exists. Read it for what it is, never as the fleet's verdict.
* **This 32-core host cannot reproduce the constrained-node operating point.** Locally epoll gets only 3.7-5.8 moves/dispatch (not 62) and neither server (63-66%) nor single-threaded bot-bench (61%) saturates, so req/s here is not server-bound. For local A/B work use `instructions:u` per move (spread 0.27%) plus server CPU per unit work, not req/s. `bot-bench` has no start barrier, so splitting the fleet across processes does NOT fix this — processes finish their window while others are still onboarding.
