# Benchmark Results & Performance Analysis

Generated from 28 candidate runs, 354 history JSONs across 3 benchmarks, 13 nodes, 6 architectures.

## Fleet Overview

| Node | Arch | Type | perf counters | callgrind |
|---|---|---|---|---|
| local (Ryzen 9 7950X3D) | amd64 | OoO strong | Y | N |
| i7-11370H | x86_64 | OoO strong | Y | Y |
| atom-n455 | x86_64 | OoO weak | N | Y |
| geode LX | i686 | in-order | N | Y |
| pentium-3 | i686 | in-order | Y | Y |
| pentium-m | i686 | in-order | Y | Y |
| cubieboard-2 (Cortex-A7) | armv7 | OoO | Y | N |
| rpi-zero-w (ARM11) | armv6 | in-order | Y | N |
| rpi-3 (Cortex-A53) | aarch64 | OoO | Y | Y |
| rpi-4 (Cortex-A72) | aarch64 | OoO | Y | Y |
| odroid-c2 (Cortex-A53) | aarch64 | OoO | Y | Y |
| odroid-n2 (Cortex-A73) | aarch64 | OoO | N | Y |
| rtl9607c | mips2r2 | in-order | N | N |

## 1. In-order vs Out-of-Order CPU Impact

### On burst workload (serversave — 1-shot save, ~60M instructions)

| Node | Type | IPC | Wall × vs i7-11370H | Instructions |
|---|---|---|---|---|
| i7-11370H | OoO x86_64 | 1.764 | **1× (ref)** | 59.1M |
| local 7950X3D | OoO amd64 | — | 1× | — |
| pentium-m | **in-order** i686 | **1.301** | **3×** | 55.0M |
| odroid-n2 A73 | OoO aarch64 | — | 2× | — |
| rpi-4 A72 | OoO aarch64 | 1.013 | 2× | 59.2M |
| rpi-3 A53 | OoO aarch64 | 0.607 | 4× | 59.7M |
| odroid-c2 A53 | OoO aarch64 | 0.562 | 4× | 60.7M |
| cubieboard-2 A7 | OoO armv7 | 0.400 | 8× | 66.3M |
| pentium-3 | **in-order** i686 | 0.355 | **12×** | 75.6M |
| rpi-zero-w ARM11 | **in-order** armv6 | 0.253 | **18×** | 64.3M |
| rtl9607c | **in-order** mips2r2 | — | **18×** | — |
| geode LX | **in-order** i686 | — | **25×** | — |

In-order penalty is **12–25×** on constrained hardware. The pentium-m is a notable outlier (only 3×, IPC 1.30) — see anomalies below.

### On sustained load (mapmanager — 300 players)

| Node | p300 ticks/s | Median tick ns (p300) | P95 tick ns (p300) |
|---|---|---|---|
| i7-11370H | 1938 | 610819 | 637024 |
| local 7950X3D | 1727 | 607594 | 947909 |
| odroid-n2 A73 | 271 | 3905375 | 3930125 |
| rpi-4 A72 | 225 | 4731019 | 4731019 |
| pentium-m | 203 | 5283606 | 5283606 |
| odroid-c2 A53 | 119 | 9264383 | 9365177 |
| atom-n455 | 110 | 10089630 | 10286652 |
| rpi-3 A53 | 110 | 9819927 | 9819927 |
| cubieboard-2 A7 | 59 | 18567048 | 18789550 |
| pentium-3 | 64 | 16889078 | 17378112 |
| rtl9607c | 48 | 23964800 | — |
| rpi-zero-w ARM11 | 45 | 24942926 | — |
| geode LX | **12** | **85286731** (85ms!) | 97719833 |

At 300 players the gap widens to **43×** between fastest and slowest. The geode takes **85ms per tick** — one core fully saturated, delivering 12 ticks/s.

## 2. CPU vs RAM Bandwidth — Cache Analysis

### Cache miss rates (mapmanager sustained load)

| Node | Cache Miss % | Notes |
|---|---|---|
| **rpi-4 (A72)** | **3.51%** | **Highest — best optimization target** |
| cubieboard-2 (A7) | 1.61% | |
| odroid-c2 (A53) | 1.51% | |
| local (7950X3D) | 1.42% | 128MB L3 helps |
| rpi-3 (A53) | 1.55% | |
| i7-11370H | 0.00% | perf_event config issue (not real) |

The rpi-4's 3.5% cache miss rate is consistent across all 11 candidate runs. The Cortex-A72 only has 1MB L2, and the mapmanager working set at 200+ players exceeds it. This is the single most promising optimization target — data layout prefetching or structure reordering would directly improve rpi-4 throughput.

## 3. botactions is Network-Bound (Not CPU)

All 13 nodes show only **0.1–0.3s total CPU time** regardless of 2 or 300 bots, with wall time fixed at 30s. This benchmark measures TCP/network round-trip latency, not server CPU performance.

**Anomaly:** On local, CPU time peaks at b60 (0.6s then *drops* at b200/b300 (0.13s) — physically illogical. Bot throttling or rate-limiting may be kicking in.

| Bot Count | local CPU (user+sys) | i7 CPU | geode CPU | rpi0w CPU |
|---|---|---|---|---|
| b2 | 0.230s | 0.265s | 0.285s | 0.280s |
| b32 | 0.440s | 0.300s | 0.325s | 0.290s |
| b60 | 0.595s | 0.285s | 0.290s | 0.280s |
| b200 | **0.200s** | 0.285s | 0.270s | 0.325s |
| b300 | **0.130s** | 0.275s | 0.285s | 0.320s |

## 4. Performance Trends Over Candidates

### mapmanager on local (Ryzen 7950X3D) — p300 ticks/s

```
Candidate        p5_t/s    p50_t/s   p300_t/s   wall_s
2026-05-24       61270     39318     1727       14.32    ← champion baseline
2026-05-25T04:58  61809     41577     2028       14.12    ← +17% improvement
2026-05-25T08:41  59258     40617     1192       14.28    ← -31% noise spike (local only)
2026-05-25T18:17  67033     41662     2056       14.31    ← +19% peak
2026-05-25T22:31  68260     41756     2063       14.26    ← +19%
2026-05-26T06:51  66105     41740     2059       14.33    ← stable
2026-05-26T11:01  65311     41677     1942       14.15    ← slight pullback
```

A **~19% improvement** was achieved between May 24 and May 25, but it was **never promoted to champion** — all candidate JSONs show `decision=none` even though per-node history JSONs contain decisions (1 KEEP = current champion, 1 DISCARD, 9 ESCALATE).

## 5. Decision Pipeline State

| Benchmark | History files | Decisions found |
|---|---|---|
| benchmarkmapmanager | 133 | 1 KEEP (champion), 1 DISCARD, **9 ESCALATE** |
| benchmarkserversave | 143 | 1 KEEP (champion), 1 DISCARD, **9 ESCALATE** |
| benchmarkbotactions | 78 | 1 KEEP (champion), **5 ESCALATE** (no DISCARD — all within noise) |

The pipeline writes decisions to per-node history JSONs but never propagates to `champion.json` — no champion has been promoted since May 24/25 despite measurable improvements.

## 6. Notable Anomalies

### pentium-m IPC: suspiciously high for in-order

Pentium-M (Banias/Dothan, 2004, in-order) gets **IPC 1.30–1.39** consistently across all candidates — higher than Cortex-A72 (1.01) and close to i7 (1.76). Same code on pentium-3 (same arch) gets 0.355–1.075. Likely GCC targeting different instruction selection/subsetting for the two P6-family variants.

### rpi-zero-w consistently dropped

Since ~May 25 12:00, rpi-zero-w is missing from mapmanager and serversave candidates (but present in botactions). Physical node issue (SD card? power? wifi disconnect?).

### Local 08:41 regression isolated

The -31% regression was local-only (all other nodes at ±2%), confirming host load noise. The noise filter correctly caught it.

### serversave DISCARD batch f9ca71c6

Local showed **+186% cache misses** and **+27% cycles** vs champion (even though wall_s improved 25%). odroid-c2 also regressed +14%. This batch was correctly flagged.

## Summary: Optimization Priorities

1. **rpi-4 cache miss rate (3.5%)** — data layout/prefetching for the mapmanager working set
2. **In-order targets (geode, rpi-zero-w, rtl9607c)** — 12–25× slower than x86_64 desktop; IPC improvements here have multiplicative impact
3. **Promote the +19% improvement** — batch 149d7370 (currently ESCALATE) should be KEEP
4. **Investigate botaction CPU decrease at high counts** — likely a bot throttling bug
5. **Fix champion promotion** — decision pipeline writes to history but never updates `champion.json`
