# benchmark/ — CPU/memory profiling and AI-driven optimisation

**Always read `../CLAUDE.md` first.** When Claude is launched with
`benchmark/` as the working directory, the root CLAUDE.md is NOT auto-
loaded — open `../CLAUDE.md` before doing anything else and combine its
rules with the ones below. The root file owns project-wide conventions
(C++/Qt, CMake-per-binary layout, multi-arch support down to i486/MIPS2/
RISC-V, never-search-from-`/`, "don't ask — just continue") and they
apply here too.

## Purpose

Workspace where the AI proposes performance optimisations, runs them
against a defined workload, compares metrics against the previous
champion, and decides keep / discard / escalate. The goal is to make
forward progress on perf without ever silently shipping a regression.

## Accept / reject / escalate — strict decision matrix

After a candidate run completes on **every benchmark target**, compare
each metric against the current champion's record for that target:

| metric outcome (across all benchmarks × all nodes) | action |
|---|---|
| at least one strictly better AND none worse (rest equal-within-noise) | **KEEP** — promote to new champion |
| at least one strictly worse AND none better (rest equal-within-noise) | **DISCARD** — revert, do not commit |
| any other mix (some up, some down) | **ESCALATE** — leave artifacts in place, surface a summary, wait for human |

"Equal-within-noise" = within ±1 stddev across the warmup-dropped
samples on the same node. A single-run difference inside the noise
band IS NOT a metric movement. Don't tune for noise.

## Where benchmarks run — dynamic combination set

Every benchmark runs on:

* The current workstation (host) — compiled here, run here.
* Every `execution_nodes[]` entry in
  `/home/user/Desktop/CatchChallenger/remote_nodes.json` whose
  `benchmark: true` flag is set AND whose **runtime load average (1
  min) is < 1.0** at the moment the benchmark is about to start.

The combination set is **dynamic**: built right before each benchmark
batch, not precomputed at startup. A node that was idle when the
harness launched can become busy ten minutes later — re-check load
on every node, every batch.

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

## Reproducibility — pin the variables we can pin

Run-to-run noise on a shared VPS easily dwarfs a 5 % optimisation.
Before measuring anything:

* **CPU affinity** — pin the benchmark process to one physical core
  (`taskset -c <N>`). Stops the scheduler from migrating it mid-run.
* **CPU governor** — set `performance` on the pinned core for the
  duration of the run, restore afterwards. Without this `ondemand`
  ramps the frequency mid-sample and adds 10 %+ noise.
* **Disable turbo** for stricter runs (echo `1` into
  `/sys/devices/system/cpu/intel_pstate/no_turbo`). Optional — turbo
  on is what production sees, turbo off gives lower-noise numbers.
  Record the setting in the artifact.
* **Warm-up** — discard the first N iterations (default 3) before
  starting measurement. Cold caches, lazy-loaded shared libs, JIT'd
  Qt resources all inflate the first samples.
* **Repeat** — run each measurement R times (default 10), report
  median + stddev, not the single best. The "best of 10" is
  misleading; the noise floor is what matters for "did the change
  move things".

These knobs live in the per-benchmark config so a particular workload
(say, a 500 ms Qt paint) can override them when sane defaults don't
fit.

## Champion baseline + history

Each benchmark target keeps a **per-arch champion record** under
`benchmark/results/<benchmark-name>/<arch>/champion.json`. Schema:

```
{
  "commit": "<sha>",
  "date":   "<ISO-8601>",
  "node":   "<execution_node label>",
  "metrics": {
    "<metric-name>": { "median": <n>, "stddev": <n>, "unit": "<u>",
                       "better": "lower" | "higher" }
  },
  "profilers": { "<profiler-name>": "<artifact path>" }
}
```

History is git-tracked; champion.json is overwritten in-place when a
new champion is promoted (the `git log` of the file IS the history).
Don't keep a parallel JSON list — it desync's.

When proposing an optimisation, the agent reads each champion.json,
compares the candidate's run against every metric, and writes the
decision (KEEP / DISCARD / ESCALATE) + per-metric deltas back to
`benchmark/results/<benchmark-name>/<arch>/candidate-<sha>.json`. The
candidate file is git-ignored except for the rare ESCALATE case the
operator wants to preserve for discussion.

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

## Auto-revert on REGRESSION

When the decision matrix says DISCARD, the harness MUST `git
restore` (or revert the patch) so the next iteration starts from
the champion again. Otherwise small regressions stack up across
iterations and the agent ends up "optimising" away from where it
started without anyone noticing. ESCALATE leaves the patch in
place since the human is going to look at it.

## Don't optimise into the noise floor

If the median delta is within ±1 stddev of the noise band on every
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
* One-off perf scratch — those belong in a personal worktree, not
  the tracked tree. Only land benchmarks that exist to validate a
  recurring metric.
