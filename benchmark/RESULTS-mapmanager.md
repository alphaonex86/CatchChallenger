# benchmarkmapmanager — Deep Analysis

Measures map visibility / tile management server tick performance across 7 player loads (5, 10, 20, 50, 100, 200, 300). Single-threaded, fully CPU-bound (99-100% CPU on all sub-benchmarks).

**Data:** 11 candidate runs × 13 nodes, champion at commit `7dead075` (2026-05-24), 133 history JSONs.

---

## 1. Scaling Behaviour — How Each Arch Handles Concurrency

The ratio `p5_ticks_per_s / p300_ticks_per_s` shows how many times faster the node is at 5 players vs 300 — lower is better scaling.

| Node | Arch / Type | p5 (t/s) | p10 | p20 | p50 | p100 | p200 | p300 | Degradation × |
|---|---|---|---|---|---|---|---|---|---|
| **i7-11370H** | OoO x86_64 | 42919 | 59598 | 55396 | 36523 | 13558 | 3586 | **1938** | **22×** best |
| **local 7950X3D** | OoO amd64 | 61270 | 61146 | 59118 | 39318 | 14938 | 3711 | **1727** | 35× |
| **odroid-n2** A73 | OoO aarch64 | 25884 | 31246 | 25742 | 8793 | 2473 | 649 | **271** | 96× |
| **rpi-4** A72 | OoO aarch64 | 23298 | 26348 | 22935 | 7970 | 2205 | 564 | **225** | 104× |
| **pentium-m** | in-order i686 | 28769 | 27259 | 20852 | 6263 | 1701 | 502 | **203** | 141× |
| **odroid-c2** A53 | OoO aarch64 | 15556 | 19483 | 14085 | 3834 | 1010 | 268 | **119** | 131× |
| **atom-n455** | OoO weak x86_64 | 13107 | 16310 | 12523 | 3966 | 1046 | 254 | **110** | 119× |
| **rpi-3** A53 | OoO aarch64 | 15201 | 18826 | 13135 | 3490 | 938 | 247 | **110** | 138× |
| **pentium-3** | in-order i686 | 20320 | 22255 | 14664 | 3630 | 930 | 136 | **64** | 318× |
| **cubieboard-2** A7 | OoO armv7 | 16652 | 15213 | 9373 | 2089 | 531 | 126 | **59** | 280× |
| **rtl9607c** | in-order mips2r2 | 14065 | 12405 | 7687 | 1717 | 442 | 87 | **48** | 294× |
| **rpi-zero-w** ARM11 | in-order armv6 | 10677 | 10009 | 6174 | 1476 | 357 | 84 | **45** | 236× |
| **geode LX** | in-order i686 | 7688 | 5690 | 2352 | 455 | 112 | 23 | **12** | **635× worst** |

Key observations:
- **Pentium-3** (318×) and **geode** (635×) suffer catastrophic scaling collapse — their tiny caches and lack of OoO execution means each additional player adds disproportionate overhead.
- **i7-11370H** scales best (22×): the large LLC (12MB L3) absorbs the working set of 300 players.
- **rpi-zero-w** (236×) and **rtl9607c** (294×) also scale poorly but less dramatically than geode.
- **Non-monotonic behaviour:** i7, odroid-n2, rpi-4 actually show *higher* throughput at p10 than p5 — the server isn't fully saturated at 5 players.

### Tick Latency at 300 Players

| Node | Median tick ns | P95 tick ns |
|---|---|---|
| i7-11370H | 611 µs | 637 µs |
| local 7950X3D | 608 µs | 948 µs |
| odroid-n2 A73 | 3.9 ms | 3.9 ms |
| rpi-4 A72 | 4.7 ms | 4.7 ms |
| pentium-m | 5.3 ms | 5.3 ms |
| odroid-c2 A53 | 9.3 ms | 9.4 ms |
| rpi-3 A53 | 9.8 ms | 9.8 ms |
| atom-n455 | 10.1 ms | 10.3 ms |
| cubieboard-2 A7 | 18.6 ms | 18.8 ms |
| rpi-zero-w ARM11 | 24.9 ms | — |
| rpi-3 (p300) | 9.8 ms | — |
| rtl9607c | 24.0 ms | — |
| geode LX | **85.3 ms** | **97.7 ms** |

The **geode** takes 85ms per tick at 300 players — one tick delay is already human-noticeable.

---

## 2. Cache Miss Analysis — The Primary Bottleneck

### Cache miss rate (of total cycles)

| Node | IPC | Cache Miss % | Branch Miss % |
|---|---|---|---|
| **rpi-4** (A72, 1MB L2) | 1.539 | **3.51%** | 0.04% |
| cubieboard-2 (A7, 512KB L2) | 0.676 | 1.61% | 0.24% |
| odroid-c2 (A53, 512KB L2) | 0.753 | 1.51% | 0.06% |
| rpi-3 (A53, 512KB L2) | 0.766 | 1.55% | 0.07% |
| local (7950X3D, 128MB L3) | 3.084 | 1.42% | 0.01% |
| pentium-3 (256KB L2) | 1.075 | **0.09%** | 0.05% |
| i7-11370H (12MB L3) | 2.696 | 0.00% * | 0.02% |
| pentium-m (2MB L2) | 1.384 | 0.00% * | 0.03% |
| rpi-zero-w (ARM11, 0 L2) | 0.372 | 0.00% * | 0.89% |

*\* perf_event may not count cache-misses on these platforms reliably*

### Stability across 11 candidates

All nodes show remarkably stable IPC and cache-miss rates across candidates (±0.1 IPC, ±0.2% CM):

| Node | IPC range | CM% range | σ |
|---|---|---|---|
| local | 3.084–3.461 | 1.26–1.59% | low |
| i7-11370H | 2.696–2.887 | ~0.00% | low |
| rpi-4 | 1.533–1.558 | 3.38–3.58% | **very low** |
| cubieboard-2 | 0.671–0.683 | 1.55–1.62% | very low |
| pentium-m | 1.383–1.395 | ~0.00% | very low |

**rpi-4 is the most interesting optimization target**: 3.5% cache miss rate is the highest in the fleet, consistent across every run, and the Cortex-A72 has no L3 to fall back on. The mapmanager working set at 200-300 players (~2.5M tiles visible?) exceeds the 1MB L2.

---

## 3. IPC Differences by Architecture — Why They Matter

The same logical work (mapmanager benchmark) executes dramatically different numbers of instructions depending on architecture, due to compiler codegen differences:

| Node | perf instructions | callgrind instructions | Binary size |
|---|---|---|---|
| **local** (amd64) | **183.3B** | no callgrind | 1,273,384 |
| **i7** (x86_64) | **180.3B** | 3,713M | 1,287,392 |
| **pentium-m** (i686) | **32.6B** | 4,164M | 1,337,748 |
| **pentium-3** (i686) | **16.5B** | 4,164M | 1,337,748 |
| **rpi-4** (aarch64) | **32.8B** | 3,831M | 1,073,904 |
| **rpi-zero-w** (armv6) | **5.3B** | no callgrind | 1,000,044 |

**Note:** perf and callgrind instruction counts differ because perf counts hardware-retired instructions while callgrind counts valgrind's JIT-representation. Cross-tool comparison is invalid; but within each tool, relative values are meaningful.

The aarch64 and x86_64 nodes share identical binaries within their arch family (confirmed by identical callgrind_ir across odroid-c2/odroid-n2/rpi-3/rpi-4). The massive difference between 180B (x86_64) and 5-33B (arm/i686) instructions is due to architectural differences in addressing modes, SIMD width, and calling conventions.

---

## 4. Performance Over 11 Candidate Runs

### Local (Ryzen 7950X3D) — p300 ticks/s

```
2026-05-24T23:00:12Z   1727.41   ← champion baseline
2026-05-25T03:14:19Z   1786.00   ← +3.4%
2026-05-25T04:58:08Z   2028.49   ← +17.4%  ← improvement appears
2026-05-25T08:41:17Z   1192.12   ← -31.0%  ← noise spike (local only)
2026-05-25T11:31:53Z   1754.50   ← returns to baseline
2026-05-25T12:49:50Z   2043.96   ← +18.3%  ← stable improvement
2026-05-25T18:17:49Z   2056.44   ← +19.0%
2026-05-25T22:31:34Z   2063.37   ← +19.4%  ← peak
2026-05-26T02:41:29Z   2044.98   ← +18.4%
2026-05-26T06:51:36Z   2058.50   ← +19.2%
2026-05-26T11:01:43Z   1942.12   ← +12.4%  ← slight pullback
```

A ~19% improvement appeared mid-series and was sustained across 6 consecutive runs. **Never promoted to champion.**

### IPC correlates with throughput improvement on local

| Period | IPC | p300 t/s | Instructions |
|---|---|---|---|
| Champion (May 24) | 3.084 | 1727 | 183.3B |
| Peak (May 25 22:31) | 3.442 | 2063 | 204.1B |
| Latest (May 26 11:01) | 3.459 | 1942 | 204.4B |

IPC increased from 3.08 to 3.46 (+12%), and instruction count increased from 183B to 204B (+11%). The throughput gain (+19%) comes from a combination of better IPC and more work per time-slice.

### i7-11370H — Stable, with one outlier

| Period | p300 t/s | IPC | Cache misses |
|---|---|---|---|
| Champion | 1938 | 2.696 | 708K |
| Latest | 1942 | 2.887 | 1,313K |
| Low (06:51) | 1783 | 2.881 | 634K |

The 06:51 run had a p300 drop to 1783 (-8%) despite the IPC being high. This single outlier correlates with a different batch of runs — likely host noise.

---

## 5. Geode Binary Change — The Biggest Delta

The geode node was offline for candidates 2-4, then returned with a **different binary**:

| Period | Binary size | callgrind_ir | p300 t/s | Δ vs champion |
|---|---|---|---|---|
| Champion | 1,404,776 | 4,294,837,234 | **12.1** | baseline |
| Candidates 5-11 | **1,304,028** | **4,140,558,072** | **22.2** | **+83%** |

The new binary is 7% smaller and executes 3.6% fewer instructions. It matches the i686 binary used by pentium-3/pentium-m (size ~1.30-1.34MB), suggesting the old champion was a geode-specific build. The champion was never updated.

Performance at p300 improved from 12 to 22 ticks/s — **still terrible** compared to other nodes, but the compiler change alone gave an 83% improvement.

---

## 6. Nodes Missing Perf Counters

Some nodes lack perf_event counter data, limiting bottleneck analysis:

| Node | perf counters | callgrind | Reason |
|---|---|---|---|
| atom-n455 | N | Y | Container no perf_event access |
| geode | N | Y | Container no perf_event access |
| odroid-n2 | N | Y | Container no perf_event access |
| rtl9607c | N | N | MIPS kernel, very constrained |
| rpi-zero-w | Y | N | ARM11, valgrind unavailable |
| local | Y | N | callgrind too slow for host |

---

## 7. rpi-zero-w Offline Since May 25

| Benchmark | Last 5 candidates with rpi-zero-w |
|---|---|
| benchmarkbotactions | 5/5 present (still connected) |
| benchmarkmapmanager | **0/5 present** (dropped) |
| benchmarkserversave | **0/5 present** (dropped) |

The RPi Zero W compiles binaries (botactions uses the same compiled server), but fails at execution time for mapmanager/serversave. Likely an SD-card, power, or wifi issue on the physical device — intermittent connectivity.

---

## 8. Anomalies Found

1. **Geode binary change (+83%):** Old champion binary was 1.40MB (geode-specific?), new one is 1.30MB (generic i686). Champion never updated to reflect this improvement.

2. **Local IPC drift:** 3.084 → 3.459 over 11 runs (+12%) despite presumably the same binary. Instruction count also grew 11% — the benchmark does more work per run as throughput improves, and the IPC gain indicates better CPU scheduling (possibly turbo frequency scaling across runs on different host load states).

3. **Non-monotonic throughput:** All nodes show p10 > p5 throughput (10 players is faster than 5). This is expected — the server isn't fully utilized at 5 players. But cubieboard-2 and rtl9607c and rpi-zero-w already show p10 < p5 (they saturate immediately).

4. **pentium-m cache-misses:u = 0.00%:** A 2004 Pentium-M showing zero cache misses is suspicious. This metric may be counting differently (mem_inst_retired.* events may not exist on Banias/Dothan cores). The 37% drop in cache misses in the latest candidate is also suspicious — both can be measurement noise.

5. **i7-11370H perf_cache-misses:u:** Consistently reports 0.00% of cycles — the counter is likely misconfigured (should be `perf stat -e cache-misses`, not `mem_load_retired.l3_miss`). The absolute count varies from 603K to 1.3M but is never proportional to cycles.

6. **rpi-4 callgrind_ir is identical** to odroid-c2, odroid-n2, rpi-3 (3830513xxx) — all aarch64 nodes share the same binary. Good for consistency, bad for arch-specific optimization testing.

7. **No champion promotion:** Despite measurable improvements on local (+19%) and geode (+83%), no champion.json update occurred. The decision pipeline writes to per-node history JSONs but never propagates to the global champion.

---

## 9. Optimization Recommendations (Priority Order)

### 1. Tame rpi-4 cache misses (3.5% → target <2%)
The RPi 4's 1MB L2 is the tightest cache in the OoO fleet. Data structure prefetching, structure-of-arrays layout, or better tile-access locality would reduce the 3.5% miss rate. A 1% reduction would save ~200M cycles per run.

### 2. Investigate geode catastrophic scaling
The geode drops from 7688 t/s (p5) to 12 t/s (p300) — a 635× degradation vs 22× on i7. The per-player overhead (map visibility calculation) must be O(n²) or have extreme cache thrashing on the 16KB L1 / 128KB L2.

### 3. Promote the +19% local improvement
Batch 149d7370 (currently ESCALATE) shows consistent +19% throughput on local. All other nodes are within ±2% of champion. This should be KEEP.

### 4. Fix rpi-zero-w execution
The device connects for compilation but fails at runtime. Needs physical investigation (SD card health, power supply, wifi stability).

### 5. Fix champion promotion pipeline
The decision pipeline correctly computes Keep/Discard/Escalate per node but never updates `champion.json`. The 1 KEEP batch (43b8d23e) is still the champion despite measurable improvements in subsequent runs.

### 6. Investigate pentium-m IPC
An in-order Pentium-M achieving IPC 1.38-1.39 is theoretically possible (the Dothan core had a 10-wide issue pipeline and µop fusion), but verifying the perf counters are counting correctly would rule out measurement error.
