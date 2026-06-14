# SKILL.md — Reverse-Engineering & Bringing Up Embedded Networking Silicon with an AI Agent

> A device-agnostic playbook for the **method, the logic, the hard-won techniques, and the autonomous AI
> agentic loop** used to reverse-engineer and bring up undocumented networking SoCs (NIC / switch / PON /
> PHY datapaths) on a clean kernel. Distilled from bringing up two related GPON ONU SoCs — one to a full
> standalone boot + network stack, one to a working upstream datapath through a real OLT. The chips don't
> matter here; the loop and the tricks do. The techniques in §4 are the part you can't get from a textbook.

---

## 1. The shape of the problem (why a special method is needed)

You are making a clean-room/ported driver drive silicon whose authoritative behaviour is **not in readable
source** — it lives in a vendor binary, a per-chip abstraction layer, and the hardware's own quirks. The
feedback loop is **physical and slow**: a build→reset→boot→settle→observe cycle is minutes, the device
hard-hangs often, and the only real success signal is an *external* peer (the link partner) accepting your
traffic. Internal counters can all be green while the peer sees nothing.

Three consequences drive everything below:
1. **You cannot trust what you read.** Hardware consumes/clears bits as it acts; a register or descriptor
   read back is *post-consume state*, not what you submitted. Your own measurements can be self-consistently
   wrong. Anchor on independent ground truth, not on readbacks.
2. **Cycles are precious.** Maximize information per physical cycle, never idle the hardware, and verify
   every step so you never spend a cycle on a stale/mismeasured artifact.
3. **You have a working reference — use it relentlessly.** The stock firmware *works*. That is your oracle,
   your hardware-health test, and your bisection baseline all at once (§4 is built on this).
4. **Stock working is an EXISTENCE PROOF.** If the vendor firmware brings the link up on this exact silicon,
   then a working configuration/sequence provably *exists* — your task is a **search with a guaranteed
   answer**, not an open question of feasibility. That changes the posture: you never ask "is this possible?"
   (it is), only "where is the difference?" — so you persist, and when you stall you change *how you look*,
   not *whether to continue* (§5.3).

---

## 2. RE method & logic (how to actually find the bug)

**2.1 Establish ground truth before changing anything — and have MORE THAN ONE source of it.** Rank sources:
*a live working reference on identical hardware* (the oracle) ≫ *real vendor source* ≫ *binaries with symbols
you decompile yourself* ≫ *the link partner's own logs/state* (the peer is a computer you can RE too). Hold
**several independent sources at once** and cross-check them: the live twin's registers, a static dump, the
vendor source, the disassembly, and the peer's view. **When two sources disagree, the disagreement is itself
the clue** — the costliest bug here was exactly a *disagreement between the live ring readback (post-consume)
and the disassembly (submit-time)*; trusting one source blindly hid it for many sessions. Distrust your own
register readbacks (until oracle-checked) and any prior AI/decompiled "remake." Turn every "is X correct?"
into a **live diff against an oracle**, not a guess. And **actively hunt for symbols/debug info the vendor
left behind** — an unstripped binary, a `kallsyms`, a symbol/`.map` file, debug strings, an assert with a
source path: each turns a black-box address into named, authoritative ground truth (§4.11). Debug symbols
are among the best sources of truth you will find — go looking for them before you brute-force anything.

**2.2 Derive intent from the source, not from a readback.** Reconstruct *submit-time* values by reading the
reference's code path (disassemble the TX/init routine), then confirm a *metric moved* — never confirm by
re-reading the thing you wrote. The most expensive bugs of the whole effort were "copy the descriptor from a
live ring": the hardware had already cleared the very bits that mattered (an append-checksum bit, a
tag-insertion control bit), so the copy was silently incomplete and traffic was dropped with **zero error
counters**.

**2.3 Bisect by layer, top-down, and prove each layer with its own counter.** Decompose the datapath into
stages (build → DMA/fetch → tag/header insertion → internal routing → classify → schedule → frame → PHY/
analog → peer). Attribute the failure to *exactly one* stage before touching the next: a green counter at N
plus a zero at N+1 localizes the break. Never "fix several things and reboot" — it destroys the signal.

**2.4 The oracle-diff harness is the highest-leverage tool you can build.** Add an introspection dump to
your driver that prints **every relevant register in the exact text format of an oracle snapshot**, then
`diff`. (Practical: dump only mapped windows — unmapped regions fault; emit non-zero only with an end-marker
— a full dump overruns the serial buffer; gate the snapshot on a *ready state* — transient FSMs read
garbage.) Caveat learned the hard way: a static oracle snapshot rarely covers *every* block — when the bug
turned out to be in an un-captured register window, the static diff was blind and only a **live read off the
working twin** (§4.5) could have caught it. Capture broadly.

**2.5 Distinguish the digital layer from the physical/analog layer.** A counter that says "transmitted" is
not the peer receiving a *valid signal at the right time*. One path can prove the analog front-end works
(an autonomous handshake succeeds) while a precisely-timed, triggered transmission on the same path still
fails. Treat them as separate problems with separate evidence.

**2.6 Two recurring tells.** "Everything matches the reference but it's still broken" has **two** roots,
both real here: (a) your *measurement* is wrong — a readback using the same wrong assumption as the write, a
post-consume value, or a **mislabeled register map** (wrong offset/stride/packing makes you read a different
register and "confirm" the wrong thing — a stride mislabel cost ~30 rounds here); or (b) the gap is **not a
register value at all** — an **init sequence/order**, a **clock or internal-link enable**, a **once-only
init step skipped on a warm re-init**, or pure **timing**. When a static register diff is exhausted, stop
diffing values and replicate the reference's full init *sequence* (and check what its first-call-only path
does). "It works on warm boot but not cold" → bootloader-inherited state masks an init your driver omits.

---

## 3. The AI agentic loop (autonomous, adaptive, fast)

### 3.1 Power/reset control is the enabler of autonomy
Put the device's **and the link partner's** power/reset under programmatic control (relay board, smart-PDU,
reset GPIO). This single capability turns a human-in-the-loop process into an unattended one: the agent
recovers from the inevitable hard-hangs and iterates around the clock. Without it the loop stalls on the
first crash. Power-cycling the **peer** too is essential — it forces a clean re-handshake when the DUT is
stuck cycling and the peer's state has gone stale. (Make the controller stateful/idempotent so toggling one
output never disturbs another.)

### 3.2 The closed test cycle
```
 hypothesis → BUILD → RESET → LOAD → SETTLE → OBSERVE → DECIDE/ADAPT
```
- **BUILD with a freshness gate.** Never trust an exit code; assert the artifact's timestamp advanced past
  the build start. Silent build failures that boot the *previous* image are the #1 source of false results.
- **RESET / LOAD** via §3.1 — and prefer a **volatile boot** (§4.2), not a flash write, every cycle.
- **SETTLE on a ready-state predicate**, not a fixed sleep — gate all reads on "the device reached the
  state where the metric is meaningful" (it may cycle through transient states).
- **OBSERVE the metric ladder** (§3.3) + the external peer.
- **DECIDE/ADAPT** (§3.6).

### 3.3 A metric ladder makes every cycle a verdict — and NEVER blind-test
Define an *ordered* set of progress signals from "first internal evidence of life" to "the real end-to-end
success," so each cycle is unambiguously pass/fail **and** tells you which layer you're on. End the ladder
on a test that exercises the *real* datapath (the peer accepting traffic / a real service coming up), not an
internal counter — counters can lie, the peer cannot.

**NEVER blind-test.** Before you run *any* change, state — in advance — the **observable that will
distinguish "fixed/progressed" from "no change,"** and confirm you can actually read it. A change with no
way to measure its effect is **not a test, it's a guess** — running it teaches you nothing and wastes a
precious cycle (§1.2). If the change has no existing signal, **build the verification first** (add a
counter, a `/proc` dump, a read-after-write check §4.9, a peer-side query) *before* you test it. And never
infer "fixed" from the *absence of a crash*, from the code change *looking right*, from a test merely
*completing*, or from a register you *wrote* (read it back — §4.9); a fix is only a fix when a pre-declared
observable moved. State the expected delta, run, then compare to what you predicted (§3.6).

The **only** acceptable "blind" move — and strictly a *last resort* — is **making a value byte-identical to
the working oracle**: "I set this register exactly like live stock, so it should work." That is justifiable
*because matching the oracle is itself the verification* (you're not guessing a value, you're copying a
proven one). Even then: (a) **read it back** to confirm the write took (§4.9), and (b) **copy the LIVE oracle
value, not a vendor-source/SDK default** — they differ in practice (live `US_OPTIC_SD_TH` read `0x00504bfa`,
while the SDK *init* source said `0x00a07fff`; porting the SDK value silently failed). Prefer a real
observable; fall back to oracle-parity only when no metric exists, and never confuse "matches the SDK source"
with "matches the running reference."

### 3.4 Three iteration tiers — always use the fastest that can answer the question
1. **Live introspection (seconds):** read/write registers or poke state on the *running* device with no
   rebuild and no reboot. Test a value hypothesis instantly. Build this hook early — it is also how you do
   live substitution (§4.3).
2. **Rebuild + reload (minutes):** for code changes — the closed cycle of §3.2.
3. **Parallel research (off-device, concurrent):** to *generate* the next hypotheses (§3.5) while the
   hardware tests the current one. Most questions answer at tier 1 or 3; reserve slow tier 2 for confirmed
   code changes.

### 3.5 Pipeline the hardware + fan out hypothesis generation
- Run each hardware test as a **background task** and let the harness **notify on completion — do not poll**
  (polling wastes the wait and burns context). Spend the wait preparing the **next N candidate tests**
  ("always 5 deep"). When the device frees up, launch the next immediately. When a result invalidates a
  branch, **discard that branch's prepared tests and regenerate** for the surviving one. The hardware never
  idles while you think; you never think while it could be testing.
- For deep "how does the reference do X?" questions, **fan out parallel research agents** (each on a
  different artifact — disassembly, the abstraction layer, the SDK, the oracle diff, the driver audit) →
  **adversarially verify** each finding (a separate agent told to *refute* it; default-refuted if the
  evidence doesn't hold) → **synthesize** a ranked list of concrete edits + a test plan. Give every agent
  the *established facts + an explicit "do not re-investigate these" list* so effort goes to the unknown.
  This runs in parallel with the physical loop. RE *proposes* a ranked hypothesis list; the peer *disposes*.
  Make the fan-out robust to a lost final step — **persist each agent's findings independently** so a failed
  synthesis (an API hiccup, a crash) doesn't discard the research that already ran.

### 3.6 The adapt step (OODA) + baked-in correctness checks
Read the metric against the hypothesis's prediction: **moved as predicted → confirmed**, advance a layer;
**unchanged → refuted**, pull the next prepared test; **a different metric moved → re-localize** (your
model was a layer off). Bake correctness checks into *every* iteration: build-freshness; a **tooling
self-check** (if a known-non-zero value reads as zero, your *tool* is broken, not the hardware); oracle-diff
over expectation; and the submit-vs-readback discipline of §2.2.

### 3.7 Persistent cross-session memory makes long RE tractable
Record every **confirmed fact**, every **disproven hypothesis with the reason**, and every **tooling trap**
to a durable store, reloaded each session. The *reason* matters as much as the verdict — disproven
conclusions get re-examined when a deeper finding lands, and only the recorded why lets you do that without
re-running everything. This is the difference between linear progress and re-deriving forever.

---

## 4. Hard-won techniques (the non-obvious tricks that actually unblock you)

These are the moves that don't show up in any datasheet. They all exploit the fact that **a working stock
firmware exists** — treat it as live, queryable, and partially transplantable, not just as a static dump.

### 4.1 Boot the stock firmware to tell a hardware fault from a software bug
When a result *could* be physical damage (a dead laser, a fried PHY, a bad optical level) **or** your
software, **stop guessing and boot the known-good stock firmware on the same unit.** If stock brings the
link fully up, the hardware is *fine* and the bug is 100% in your code — you never again waste a cycle
suspecting the silicon. If stock *also* fails, you have a hardware/wiring/peer problem and no amount of
driver work will help. This binary test should be the **first** thing you do whenever a "is the hardware
even okay?" doubt appears. (Concretely here: ranging + the stock stack working proved the laser and optics
were healthy, which redirected the entire effort away from laser-calibration dead-ends toward burst timing.)

### 4.2 Develop on a VOLATILE boot — never wear out or brick the flash
Do **not** flash your dev build to NAND/NOR on every iteration. Flash has limited write/erase cycles (you
*will* wear it out over hundreds of build cycles) and a bad write can **brick** the board. Instead, drop to
the bootloader (U-Boot) and **load the kernel into RAM and `bootm`** (tftp the image, or use a ramdisk /
NFS root), leaving the persistent stock firmware untouched. Two huge wins: (1) flash longevity + zero brick
risk during dev; (2) **crash recovery is a plain power-cycle back to working stock** — combined with §3.1
that's fully unattended iteration. Only flash once a build is actually proven. (Note a trap: bootloaders
often have *no NAND fallback* — if you remove the tftp image hoping it'll boot stock, U-Boot may just halt.
Boot stock by *intercepting U-Boot and running its NAND/boot command*, not by deleting your image.)

### 4.3 ★ Bisect by LIVE substitution against the running reference (the most powerful method) ★
Static "diff my regs vs stock" tells you *what differs*; this tells you *which of your parts is wrong*.
**Start from the fully-working stock system and replace it with your code one part at a time, observing at
each step whether it still works.** Each swap that keeps it working clears that part; the swap that breaks
it is the culprit — you've binary-searched the fault to one component instead of staring at a dead whole
stack. A practical substitution ladder, coarse→fine:
1. **Stock everything** — the baseline that *works* (and proves the hardware, §4.1).
2. **Your userspace + stock kernel/binary** (replace the top layer only).
3. **Your kernel/driver + stock userspace** (replace the bottom layer only).
4. **Your single module / one driver function + stock rest** — `rmmod` the stock module, `insmod` yours,
   re-test; or swap one init routine.
5. **Live register/parameter substitution** — with the live-introspection hook (§3.4 tier 1), overwrite one
   register block with your values on the *running* stock and watch it keep working or break — no rebuild.
Run it forward (stock→yours, find the first break) *and* backward (yours→stock, find the one swap that
fixes it). The breaking/fixing swap localizes the work. Caveats: respect ordering/init dependencies (a part
may need its prerequisites present), and don't kill a *running* daemon to "test if it's needed" — that
proves nothing (§4.4). The payoff is precision: when a swap breaks it you don't just learn *which* part is
wrong — you often learn *what* to fix, because you can immediately diff your part's behaviour/registers
against the stock part it just replaced (same hardware, same moment, known-good vs yours). This converts "my
whole port is broken, somewhere" into "this one part, and here's exactly how it differs from working" — it is
the single most important technique in this document.

### 4.4 To learn what init does, REBOOT and observe — don't kill the running daemon
Killing a running service to see if the link drops tells you almost nothing (state is already established).
To learn *what brings a subsystem up*, **reboot the working system and capture exactly what it does at
boot** — which config it writes, which sequence it runs, what values it sets (SN, password, mode, the init
order). Then make your system issue the same. The bring-up *sequence* is the prize, and you only see it at
boot, not at steady state.

### 4.5 Read a live working twin as a register oracle
If you have a second identical unit (or the same unit on stock), get a shell on it and **read its registers
live while it's in the working state** — the ground truth for "what should this register be," including the
blocks your static dump never captured. Non-destructive to your DUT. On locked vendor boxes: the linux shell
usually hides behind a restricted CLI + a per-unit password (often derived from the MAC/serial, MD5-checked,
not in any doc); and busybox frequently lacks `devmem`/`head` — so **tftp a tiny `mmiord`/`i2crd` helper
into `/tmp`** and run that. The device's own tftp client is often interactive.

### 4.6 "Isolated tolerates, packed exposes" — a diagnostic for "works early, fails later"
A sparse/isolated/early operation can tolerate a misconfiguration that a back-to-back/packed/steady-state
operation cannot. When something works during an early or one-shot phase but fails in the operational phase,
look for config that only bites under tight timing or packing — thresholds, settle/margin windows,
burst-mode vs continuous-mode selects, power-save windowing. (Here: an isolated handshake burst got through
with default thresholds, but packed operational bursts back-to-back tripped the device's own "too long /
mismatch" monitor and it suppressed itself — invisible until the bursts were tight.)

### 4.7 A zero counter may be MASKED, not idle
Counters are often gated by an enable/mask register. A counter reading 0 can mean "masked off," not "nothing
happened." Always check the count-mask/enable before concluding a stage is dead.

### 4.8 Treat the serial console as a shared, fragile resource
Re-enumerate the ports every session (mappings drift), probe each before trusting it, `fuser -k` before
opening, and remember that **verbose logging over the console perturbs timing-sensitive FSMs** (it can
itself break a handshake) — use it for one-shot captures, not steady-state runs.

### 4.9 Verify every write actually LANDED before concluding it had "no effect"
A huge amount of effort was wasted on registers that read back unchanged after a write — concluding "this
value has no effect" when the write *never took*. Before believing a write was applied: **read it back
(read-after-write).** If it didn't stick, the register is one of: behind a **write-protect / unlock gate**
(a separate register must be unlocked first, and re-locked after); **read-only** (e.g. MIB counters — writes
silently ignored); **gated/clocked off** (the block isn't powered/enabled); or you have the **wrong
address**. The same check validates a *debug/snapshot block* before you trust its reads — a control register
that reads 0 right after you wrote it is unmapped, gated, or at the wrong offset (a documented snapshot block
turned out to be exactly this — its reads were meaningless until proven writable). "Applied" and "had no
effect" are different conclusions; never conflate them.

### 4.10 Anchor a stripped-firmware disassembly with a known literal string
To disassemble a vendor firmware blob with no symbols: **decompress it first** (LZMA/gzip — the on-flash
image is usually compressed), then find the load/virtual address by grepping for a **known literal string**
inside it (a source path, a version banner, an error message) and matching it to where that string is
referenced. `vaddr = load_base + file_offset`. With one anchor you can disassemble any routine at the
addresses the vendor's RE notes (or a `kallsyms`) reference. This is how the submit-time TX logic (§2.2) was
recovered when no source existed.

### 4.11 Hunt for what the vendor forgot or misconfigured — it leaks ground truth
Vendors ship in a hurry; the build that reaches the field is full of things they meant to strip and didn't.
Go looking for them *first* — they convert guesswork into fact:
- **Debug symbols — and they hide in many places, so check ALL of them:**
  - *Inside an ELF* — `.symtab`/`.strtab` (function+object names) and the DWARF `.debug_info`/`.debug_line`
    sections (`readelf -S`, `objdump -t/-T/--dwarf`, `nm`); production binaries are often only *partially*
    stripped and still carry `.dynsym` or full symbols.
  - *Inside kernel modules (`.ko`)* — modules are **frequently far less stripped than `vmlinux`** (they keep
    symbol + reloc tables for linking), so a `.ko` often names what the stripped kernel hides.
  - *As a SEPARATE file* — split debug info: `*.debug` / GNU `.gnu_debuglink`, `/usr/lib/debug/…`, a
    `-dbg`/`-dbgsym` package, a build-id-indexed debug store, or a `vmlinux`/`System.map`/`/proc/kallsyms`
    sitting next to the stripped runtime image.
  - *As leftover SOURCE* — actual `.c`/`.h` files, makefiles, or generated headers left in the rootfs / SDK
    tarball (the literal answer, not a reconstruction).
  - *In forgotten DUMPS* — core dumps, full flash/memory dumps, factory register dumps, verbose boot logs,
    `.map`/linker files — each can carry symbols, addresses, or ground-truth register values.
  Name every function and register helper this way, then disassemble the *exact* routine by name (the single
  best source of truth — §4.10).
- **An OLDER release, a DIFFERENT-ARCHITECTURE build, or the reference SDK of the SAME code — often shipped
  WITH symbols or even source.** The production binary on your device is stripped, but a sibling build very
  often isn't: an earlier firmware version, a port for another arch/board, an evaluation/SDK build, a debug
  variant. Functions, struct layouts, and register/field semantics rarely move much across versions or
  architectures, so you **cross-map the symboled sibling's names + structures onto your stripped production
  binary** (match by call graph, constants, string refs). Hunting down "the same code, but with symbols,
  somewhere else" is frequently faster than reversing the stripped target directly.
- **Debug strings / asserts / log formats** baked into binaries — they reveal field names, register
  semantics, error conditions, and state-machine names for free (grep the blob).
- **A symbolic register/field database inside the vendor library** (a reg-id → offset/field table the DAL
  uses) — decode it once and every symbolic access becomes a concrete address.
- **Verbose/debug firmware variants, factory/test images, leftover `/debug` or `proc` hooks, sysrq, open
  JTAG/UART, default or algorithmically-derived passwords, world-readable configs** — any of these gives a
  live shell or live ground truth on the working stock.
The rule: **before brute-forcing the unknown, spend a pass looking for where the vendor already wrote the
answer down and forgot to remove it.** It is almost always somewhere.

### 4.12 Getting IN: cracking access on a locked vendor device
The oracle (§4.5) and most live RE assume a shell/root, which the vendor tries to deny. Ways in, roughly in
order of value:
- **A UART/serial console is the single highest-value access vector — get it first.** It is almost always
  physically present (a 3–4 pin header or unpopulated pads near the SoC; find it with a multimeter / logic
  analyzer at common baud rates). Serial gives you the **bootloader (U-Boot)** — which alone lets you
  **volatile-boot your own image (§4.2), dump flash, drop to recovery, and set boot args** — and a **console
  that frequently bypasses the network/CLI lockdown entirely** (or at least gives the restricted CLI you then
  escalate from). It works when the network is down, the firmware is bricked, or SSH/telnet is disabled.
  Everything in this guide — the oracle reads, the DUT loop, driving the peer — runs over serial. If you have
  one port, make it serial.
- **Crack the password from a firmware DUMP.** The restricted-CLI / linux-shell password is almost always
  recoverable from the firmware image: the **plaintext or hash sits in a binary/config** (grep the rootfs,
  then crack the hash offline), or it is **algorithmically derived from per-unit identity** (here the
  `enterlinuxshell` password was literally `<MAC>.<serial>` — MD5-checked but trivially reconstructed once you
  read the derivation in the CLI binary). Get the dump (flash reader, `mtd`/`dd` from any shell, U-Boot, or a
  recovery/service mode) and the secrets fall out.
- **If you can't get a dump, the firmware-UPDATE path is itself the way in.** Build/modify an image with a
  shell enabled (dropbear/telnet, a root password you set, an init hook) and **flash it** — now you own the
  device and can dump the flash from inside. The official updater is an access vector, not just maintenance.
- **Unsigned / unverified updates let you inject ANYTHING.** Cheap CPE very often does **not** verify the
  update image's signature (or verifies it weakly). **Test whether the updater actually checks a signature
  before assuming it does** — if it doesn't, inject arbitrary firmware: enable a console, bake in your driver,
  or drop a flash-dumping payload. (Even when the main image is signed, a downgrade image or a recovery/factory
  path is often unsigned.) This is frequently the fastest route from "black box" to "root + full dump," which
  then unlocks everything else in this document (symbols §4.10/§4.11, oracle reads §4.5, substitution §4.3).

---

## 5. Strategy: pick an attack vector — and change it when you hit a wall

**5.1 There are several independent ways into the same goal. Choose one deliberately; switch when it
stalls.** For an ONU bring-up these are orthogonal attack vectors, each with a different cost/risk profile:
- **Start from mainline (OpenWRT/clean kernel) and adapt** — clean, maintainable base, but you must RE and
  re-implement every missing piece. Best when you want a long-lived result and the gaps are bounded.
- **Start from the stock firmware and rewrite part-by-part** — you begin from something that *works* and
  replace components incrementally (this is §4.3). Best when "it's broken everywhere" and you need an
  always-working baseline to bisect against. Often the fastest way to *localize*.
- **Attack from the device side** — make your device emit the correct output and instrument its internal
  stages.
- **Attack from the peer side** — RE the link partner (its firmware, logs, timers, deactivation logic) to
  learn *exactly what it expects and what makes it reject you*. The peer defines the target spec; the
  device-side work is just hitting it. Crucially, **the peer often rejects you on a TIMER/alarm firing, not
  in response to your last message** — so chasing "what did I send last" misleads; find the *timer/alarm
  condition* (here: an auth-timer + signal/ack alarms drove deactivation, which defined what a valid burst
  must satisfy and reframed the whole device-side search).
These combine: **RE the peer to define the target, then make the device hit it, bisecting against stock.**
Don't marry one vector — if it's not moving, the *vector* may be the problem, not your effort.

**5.2 Recognize the wall, and change your viewpoint or method — you may simply be wrong.** If you've spent
many cycles pushing one approach with no movement on the metric ladder, treat that as **evidence the
approach (or your mental model) is wrong**, not as a reason to push harder. Step back and re-frame: pick a
different attack vector (§5.1), a different layer of the bisection, a different source of truth (§2.1), or a
different tier of tool (§3.4). Admitting a mistaken model early is cheaper than confirming it for ten more
cycles. Two walls in this project broke only after a deliberate viewpoint change: chasing the L2-switch
egress (a red herring) → pivot to the descriptor itself uncovered the missing checksum bit; chasing analog
laser calibration → pivot to *burst-mode timing* uncovered the self-suppression. The skill is noticing
"I'm at a wall" fast and forcing the re-frame, instead of grinding the same hypothesis.

**5.3 If stock works and yours doesn't, there IS a way — so don't stop, change how you see it.** The
working reference is proof the silicon can do exactly what you want (§1.4); every failure of *your* build is
therefore a difference you haven't found yet, not a dead end. So the discipline is: **never conclude "it
can't"** (it provably can) — only "I haven't found the difference *yet*." When one method stalls, that is
information about the *method*, not about feasibility: switch the attack vector (§5.1), the layer, the source
of truth, the tool tier — keep the search alive by changing *how you look and act*, with method and logic,
relentlessly. Most walls here fell only because the search continued past the point where a given mental
model said "this should already work." Persistence here is not stubbornness on one hypothesis — it is
refusing to stop the *search* while freely abandoning any single hypothesis.

## 6. Lessons distilled (abstract, transferable)

- **Hardware lies on readback.** Derive submit-time intent from source; confirm with a moved metric.
- **Zero error counters ≠ nothing sent.** A malformed-but-transmitted frame is dropped silently at the
  peer's MAC/PHY. "Clean and dead" is a signature, not an absence of evidence.
- **The working reference is your most powerful instrument** — as a hardware-health test (§4.1), a live
  oracle (§4.5), and a transplant donor for substitution bisection (§4.3). Don't treat it as a static dump.
- **Develop volatile, flash rarely** — save the NAND, keep a one-power-cycle path back to a working system.
- **Packed register arrays follow the silicon's packing rule, not your intuition**; a readback using the
  same wrong rule "confirms" the wrong write.
- **Bitfield layouts are per-chip even within one family** — resolve from *that* chip's headers/binary; a
  wrong layout puts your value in reserved bits and looks like a dead datapath.
- **Matching the reference register-for-register is a heuristic, not the goal** — some deliberate
  divergences are load-bearing on a given silicon revision; understand the *function* first.
- **Don't reset the engine to recover** if reset has side effects (it can permanently break a path and stop
  the very counters you measure); prefer inheriting the bootloader's working engine and re-arming softly.
- **Some init is state, not value:** warm boot inherits it, cold boot exposes its absence — diff the two.
- **A few fixes will be empirical constants** you can't fully reverse — accept and mark them; a working
  datapath is the goal, not a complete theory.
- **Hold multiple sources of truth; a disagreement between them is a finding, not noise** (§2.1).
- **A wall means your approach is probably wrong** — change vector/layer/source/tool before grinding (§5.2).
- **There are several entry points to the same goal** (adapt-mainline / rewrite-from-stock / device-side /
  peer-side) — pick deliberately, combine them, and switch when one stalls (§5.1).
- **Verify the register MAP (offset/stride/packing) against the vendor reg-DB before trusting any counter** —
  a mislabeled map makes you read the wrong register and "confirm" wrong conclusions for a very long time.
- **DMA engines prefetch descriptors; a doorbell re-polls the stale prefetch, not memory** — sparse traffic
  exposes ring-park bugs that bulk traffic hides; recover by *re-latching the ring start*, not by ringing the
  doorbell, and pick a health metric that survives a reset (ring progress, not a MIB that stops counting).
- **"Applied" ≠ "had an effect."** Read-after-write to prove a write landed before judging its value;
  write-protect/read-only/gated registers ignore writes silently (§4.9).
- **Hunt for the symbols the vendor forgot** — an unstripped / older / other-architecture / SDK build of the
  same code names every function and struct; cross-map it onto the stripped production target (§4.11). Debug
  symbols are among the best sources of truth — look before you brute-force.
- **Stock working is an existence proof: never conclude "it can't" — only "I haven't found the difference
  yet."** When a method stalls, change how you look (vector/layer/source/tool), not whether you continue.
- **Never blind-test.** Declare the pass/fail observable *before* running; a change with no way to measure
  its effect is a guess, not a test. "No crash," "the diff looks right," "the test completed," and "I wrote
  the register" are **not** evidence of a fix — only a pre-declared observable that moved is (§3.3, §4.9). The
  sole last-resort exception is **oracle-parity**: set the value byte-identical to *live* stock (never the SDK
  default — they differ) and read it back; matching the running reference is itself the verification.
- **Getting root on a locked box is usually easy.** A **UART/serial console is the highest-value way in —
  get it first** (it hands you U-Boot: volatile boot, flash dump, recovery, and often a console past the
  lockdown). Then: **crack the password from the firmware dump** (plaintext/hash in a binary, or derived from
  MAC/serial), **or flash your own image** — and **unsigned/unverified updates let you inject anything** (test
  the signature check first; it's often absent). Root → full dump → symbols, oracle reads, substitution (§4.12).

---

## 7. Operating checklist (the loop, condensed)

1. Power/reset under program control — device **and** link partner. Confirm crash-recovery works first.
2. **Boot stock once** to confirm the hardware is healthy (§4.1) before suspecting silicon.
3. Develop on a **volatile (RAM/NFS) boot** via the bootloader; never flash per-iteration (§4.2).
4. Stand up a **live oracle** (working twin / stock) and an **oracle-diff** dump in your driver.
5. Add a **live introspection** hook (read/write running state with no rebuild) — also enables substitution.
6. Define the **metric ladder**, ending on a real end-to-end test. **Before every test, declare the
   observable that proves fix/progress; if none exists, build it first — never blind-test.**
7. When stuck on "which of my parts is wrong," **substitute against running stock part-by-part** (§4.3).
8. Every cycle: **build-freshness gate → reset → volatile-load → settle-on-ready → observe → adapt.**
9. Keep the hardware busy: background tests + 5 prepared hypotheses; never poll; regenerate on branch death.
10. Generate hard hypotheses with **parallel RE agents + adversarial verification**, concurrent with testing.
11. Bisect top-down; **prove each layer with its own counter** before blaming the next.
12. Cross-check **multiple sources of truth**; treat any disagreement between them as a finding (§2.1).
13. Pick an **attack vector** deliberately (adapt-mainline / rewrite-from-stock / device-side / peer-side);
    if the metric isn't moving after many cycles, **change the vector/viewpoint** — the wall is the signal (§5).
14. Persist facts, disproven hypotheses (**with the why**), and tooling traps across sessions.

---

*The durable skill, restated: the working firmware is your oracle, your hardware-health test, and your
transplant donor — boot it to clear the hardware, read it live for ground truth, and replace it part-by-part
to find which of your pieces is wrong. Develop volatile to spare the flash, put power under program control
and pipeline tests so the hardware never idles, generate hypotheses with parallel research and verify them
adversarially, derive intent from source because hardware lies on readback, and prove every layer with a
counter before moving on. Hunt down the symbols the vendor forgot to strip — even in an older or other-arch
build of the same code — because they are ground truth handed to you. And hold onto the one fact that
reframes everything: the stock firmware works, so a working path provably exists — you are never asking
whether it's possible, only where the difference is. So don't stop; change how you see it, and keep
searching, with method and logic, until the peer accepts you.*
