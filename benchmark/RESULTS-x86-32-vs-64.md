# x86 32-bit vs 64-bit — comparison across all benchmarks

> ## 2026-06-06 — SAME-CPU result (resolves "The central caveat" below; now incl. ARM)
>
> The fleet now runs BOTH bitnesses on the **same silicon**: every x86_64 box
> also builds+runs an i686 (`-m32`) sibling, every aarch64 board also runs an
> armv7 sibling (built on the native armv7 compile node). So **throughput is
> finally an ISA signal** — each row below is one CPU, only the bit-width
> changes. Data: commit `60d7d19`, `benchmarkserversave` + `benchmarkmapmanager`.
>
> ### Verdict: 64-bit is FASTER under real load; 32-bit uses LESS RAM
>
> **Speed — mapmanager steady-state throughput (ticks/s, higher = faster).
> Cell = 32-bit relative to 64-bit on the SAME CPU; `−` means 32-bit is slower:**
>
> | CPU (same silicon) | 10 players | 50 players | 200 players | 300 players |
> |---|---|---|---|---|
> | i7-11370H (x86_64 ↔ i686) | +2.3% | −3.5% | **−19.7%** | **−21.6%** |
> | fitxlan AMD A4 (x86_64 ↔ i686) | +2.1% | −14.2% | **−14.9%** | **−16.3%** |
> | rpi-4 Cortex-A72 (aarch64 ↔ armv7) | +15.0% | −5.3% | **−11.3%** | **−12.9%** |
> | rpi-3 Cortex-A53 (aarch64 ↔ armv7) | +2.3% | −11.6% | **−9.8%** | **−10.7%** |
> | odroid-n2 Cortex-A73 (aarch64 ↔ armv7) | +10.7% | −3.6% | **−7.1%** | **−6.2%** |
> | odroid-c2 Cortex-A53 (aarch64 ↔ armv7) | +4.3% | −10.8% | **−11.6%** | **−11.9%** |
>
> **At realistic/heavy load (50–300 players) 64-bit wins on EVERY CPU, by
> ~6–22% (typ. ≈12%), and the gap widens with load.** 32-bit edges ahead ONLY at
> near-idle (≤10 players), where its smaller cache footprint helps — the regime
> where the server isn't stressed. (rpi-3 ran warm — thermal-throttle suspected
> — but the direction holds across all six CPUs.)
>
> **One-shot `save` (serversave) confirms it — 64-bit uses fewer CYCLES for the
> same work (same CPU, same clock):** i7 −6.6%, fitxlan −10.7%, odroid-c2 −10.3%,
> rpi-4 −13.2%, rpi-3 −20.3% (32-bit always more cycles). 64-bit also runs
> ~10–16% fewer instructions (16 GPRs + native 64-bit ops) — on x86 AND ARM
> (perf-stat: ~+13% instructions on the i686/armv7 side).
>
> **Memory — 32-bit wins (narrow pointers), mapmanager peak RSS, 32 vs 64:**
> odroid-n2 −24.6%, rpi-4 −15.0%, rpi-3 −13.6%, i7 −11.0%.
>
> **Bottom line:** **64-bit** where the box has RAM (~10–22% more throughput
> headroom under load); **32-bit** on the sub-64 MB targets where the ~15–25%
> RAM saving outweighs the throughput cost. Consistent across x86 AND ARM — the
> first time measured on one CPU per row (the cross-CPU analysis below could
> not isolate the ISA from the silicon; this section does).
>
> *(armv7 `callgrind` is unavailable — valgrind on the aarch64 boards has no
> 32-bit-ARM tool, recorded SKIP — so the ARM instruction-count signal comes
> from perf-stat, not callgrind IR.)*

Source: append-only history JSONs under `benchmark/history/`. Each benchmark
pinned to **one commit present on all six x86 exec nodes** so the compare is
apples-to-apples:

| benchmark | commit |
|---|---|
| benchmarkmapmanager | `0aa4edd` |
| benchmarkserversave | `0aa4edd` |
| benchmarkbotactions | `ea5021f` |

Exec nodes by ISA width:

| ISA | exec node | compile node | CPU | clock |
|---|---|---|---|---|
| **64-bit** | i7-11370H | amd64-lxc | Core i7-11370H | ~1.1–1.3 GHz* |
| **64-bit** | atom-n455 | amd64-lxc | Atom N455 | ~1.6 GHz |
| **64-bit** | local | local (host) | Ryzen 9 7950X3D | ~3.8 GHz |
| **32-bit** | pentium-3 | x86-lxc | Celeron (P3 core) | ~1.1 GHz |
| **32-bit** | pentium-m | x86-lxc | Pentium M | ~1.4–1.7 GHz |
| **32-bit** | geode | lxc-geode | Geode LX800 | ~0.5 GHz |

\* i7 clock floats (governor); raw throughput is not the ISA signal anyway.

## The central caveat

**No single CPU ran both a 32-bit and a 64-bit build.** Every exec node is
locked to one ISA. So raw **throughput / wall-time cannot isolate the ISA** —
it is dominated by the silicon (a Ryzen at 3.8 GHz vs a Geode at 0.5 GHz says
nothing about 32 vs 64 bit). The honest ISA signals are the metrics that are a
property of the **compiled code**, not of the CPU that runs it:

* **instruction count** (callgrind IR — deterministic; perf instructions)
* **binary size** (codegen density)
* **peak RSS** (pointer width)

Those three ARE clean. Throughput is reported below only grouped by node, with
the hardware confound called out.

---

## 1. Instruction count — the cleanest ISA signal

`benchmarkmapmanager` callgrind IR is deterministic (same input → same count),
so it depends only on the compiled code, **not** the CPU:

| node | ISA | callgrind IR (same map workload) |
|---|---|---|
| atom-n455 | 64-bit | **2.129 × 10⁹** |
| pentium-m | 32-bit | 2.319 × 10⁹ |
| pentium-3 | 32-bit | 2.344 × 10⁹ |
| geode | 32-bit | 2.370 × 10⁹ |

→ **32-bit executes ≈ +10 % more instructions** for identical work.
Corroborated by `benchmarkserversave` perf-instructions: i7 (64-bit)
5.73 × 10⁷ vs pentium-3 (32-bit) 7.52 × 10⁷ → **+31 %** on 32-bit.

Why 64-bit needs fewer instructions: 16 GPRs instead of 8 (far less
spill/reload), native 64-bit ALU ops (no hi/lo register-pair emulation for
64-bit arithmetic/hashing), and more arguments passed in registers.

*(`benchmarkbotactions` perf-instructions is ~equal 32 vs 64 — but that
workload is network/bot-driven and not deterministic, so it is not an ISA
signal.)*

## 2. Binary size — codegen density

`benchmarkmapmanager` (sizes are per compile-arch, identical for both exec
nodes of the same build):

| build | bytes | vs amd64 |
|---|---|---|
| amd64 (64-bit) | 1,270,048 | — |
| i686 / x86-lxc (32-bit) | 1,302,608 | **+2.6 %** |
| i686 / geode (32-bit) | 1,405,880 | **+10.7 %** |

→ 32-bit code is **2.6 % larger** (generic i686) to **10.7 % larger** (Geode
`-march`-tuned, which lacks newer instructions and emits longer sequences).
64-bit wins on density despite REX prefixes and wider pointers — the saved
spill/reload and register-pair code more than pays for it.

*(`benchmarkbotactions` reports an identical 7,342,976 bytes on every arch →
the binary-size probe there measured a non-arch-specific artifact; treat that
cell as a measurement bug, not "32==64".)*

## 3. Peak memory (RSS) — pointer width

Cleanest near-equal-class pair, `benchmarkmapmanager`:

| node | ISA | max RSS |
|---|---|---|
| atom-n455 | 64-bit | 6,044 KB |
| pentium-3 | 32-bit | 5,876 KB |
| pentium-m | 32-bit | 5,916 KB |
| geode | 32-bit | 5,860 KB |

→ **32-bit uses ≈ 3 % less RAM** (half-width pointers). The effect is small
because the map working set is mostly *data*, not pointers.
`benchmarkbotactions` (~70 MB working set) is ~equal 32 vs 64 — data-dominated.

> The large RSS on **local / i7** (8.6–9.2 MB here, 90–110 MB in botactions) is
> NOT an ISA effect — it is glibc's per-core malloc-arena and page-cache
> heuristics on a fast many-core host. Ignore it for the ISA question.

## 4. Throughput — hardware-bound, NOT an ISA signal

`benchmarkmapmanager` ticks/s (higher = better):

| node | ISA | clock | p10 | p50 | p200 | p300 |
|---|---|---|---|---|---|---|
| local (Ryzen) | 64 | 3.8 GHz | 68,270 | 52,200 | 8,256 | 4,277 |
| i7-11370H | 64 | ~1.1 GHz | 59,190 | 44,970 | 7,588 | 3,902 |
| atom-n455 | 64 | 1.6 GHz | 17,280 | 6,590 | 578 | 246 |
| pentium-m | 32 | 1.7 GHz | 28,500 | 10,440 | 970 | 447 |
| pentium-3 | 32 | 1.1 GHz | 23,550 | 6,153 | 205 | 91 |
| geode | 32 | 0.5 GHz | 10,550 | 1,404 | 81 | 45 |

The instructive pair is **atom-N455 (64-bit, 1.6 GHz) vs Pentium-M (32-bit,
1.7 GHz)** — almost the same clock. The **32-bit Pentium-M is *faster*** (p50
10,440 vs 6,590). That is **not** a 32-bit ISA win — it is the Pentium-M's
much stronger per-clock microarchitecture vs the in-order Atom. It simply shows
throughput is decided by the core, not the bit-width, so these numbers must not
be read as "32-bit beats 64-bit".

`benchmarkserversave` wall_s and `benchmarkbotactions` user_s tell the same
story: ordering tracks CPU speed (Ryzen < i7 < Pentium-M < Atom < P3 < Geode),
independent of ISA.

---

## Verdict

On the metrics that the ISA actually controls:

| dimension | winner | margin (this fleet) |
|---|---|---|
| instructions / work | **64-bit** | −10 % (callgrind), −31 % (serversave) |
| binary size | **64-bit** | 32-bit +2.6 % to +10.7 % larger |
| peak RSS | **32-bit** | −3 % (small; data-dominated) |
| throughput | *inconclusive* | dominated by CPU, not ISA |

**64-bit x86 is the more efficient ISA per unit of work** (fewer instructions,
denser code) thanks to 16 registers + native 64-bit ops. **32-bit's only edge
is a small RAM saving** from narrow pointers — which is exactly why it still
matters for the sub-64-MB constrained targets the project exists for, where a
3 % RSS cut can outweigh a 10 % instruction-count penalty. There is **no
throughput conclusion** from this data because no CPU ran both builds; a true
ISA throughput test needs the *same* 64-bit-capable CPU running an i686 build
and an amd64 build back-to-back (e.g. the Atom N455, which can do both).
