---
name: testing-client-server-on-everything
description: >
  Abstract, engine-agnostic playbook for testing networked and
  data-handling software (real-time games, MMOs, low-latency services,
  file/data movers) HARD enough to make it field-stable for years:
  across every platform/OS/architecture it ships to, across the full
  chain (build → installer → client → local & remote server → DB),
  across the whole behaviour space (every action × typical traffic ×
  machine-generated/fuzzed traffic), across the "should never happen"
  surface (mid-stream disconnects, idle silent peers, partial loss,
  flap/reconnect, corrupted and hostile input, DDoS at 100× nominal
  load), and — for any irreversible operation — across the data-safety
  surface (interrupt at any byte, every external-resource failure, never
  lose or corrupt user data). Distils how to find bugs the happy path
  hides, prove robustness instead of asserting it, and keep all of it
  deterministic and out of the product code. Distilled from two
  long-lived codebases — an MMO engine that runs on hardware as weak as
  an i486, and a file-copy tool trusted in the field for 15+ years — but
  every rule transfers to any software. Use when designing a test
  harness, adding a regression test, hunting an intermittent/heisenbug,
  hardening against abuse or data loss, or porting to a new platform.
---

# Testing a client/server system on everything

Hard-won doctrine, not generic QA advice. The goal is not "tests pass"
— it is **to actively manufacture every condition the field will throw
at you, before the field does**, and to prove the system survives each
one. The happy path is the easy 5%; this skill is about the other 95%.

The invariants every section serves:

- **The whole chain is tested as users live it** — from the shipped
  installer down to the byte on the wire, not just `make check`.
- **Every platform/OS/arch we ship is in the matrix** — a bug that only
  shows on big-endian, on unaligned access, on the old OS version, or on
  the 66 MHz box is still a shipped bug.
- **The abnormal path is the main path** — disconnects, corruption,
  idle peers, packet loss and abuse get *more* test surface than the
  happy path, because that is where crashes, leaks and exploits live.
- **One bad client never harms another** — hostile or broken input gets
  the offender kicked cleanly, with zero collateral on other sessions
  and zero process death.
- **An irreversible operation never loses or corrupts user data** —
  interrupt, error, collision and flaky media each have a defined, safe
  resolution; the source is sacred until the destination is verified.
- **Everything is deterministic** — same inputs → same bytes/pixels,
  every run, every arch. Non-determinism is a bug in the *test*, fixed
  at the source, never papered over with tolerance.
- **Test code never perturbs product code** — the binary under test is
  the binary you ship; instrumentation lives outside or behind a flag
  that is off in production.

The recurring meta-principle: **a test you didn't write is a bug you
ship. Enumerate the failure modes deliberately — don't wait to discover
them.**

---

## 1. Test the whole chain, the way a user lives it

A unit test proving a function is correct says nothing about whether the
*installed product* works. Build the end-to-end path and exercise it.

- **Start from the shipped artefact, not the build tree.** For a desktop
  target, run the actual **installer** (under emulation if need be —
  e.g. the Windows NSIS/MSI installer driven through `wine64`), launch
  the *installed* client, and play. Bugs in packaging, deploy-qt runtime
  bundling, datapack placement, code-signing and asset paths only appear
  when you test the artefact, not the dev build.
- **Drive the real client, not a mock.** A test client that reimplements
  the protocol drifts from the product client and tests itself. Drive
  the *actual* client binary (solo/offline, against a local server, and
  against a remote server) so the product's own parser, prediction and
  render path are what's under test. Expose an automation channel for
  this (e.g. a local unix socket that injects keys/clicks and reads back
  state/inventory/chat/trade/fight) rather than screen-scraping.
- **Own both ends so the test is deterministic.** Stand up your **own
  local server** and **your own DB** for the run. You cannot assert on a
  shared/remote service whose state you don't control. A local server +
  a wiped, seeded DB makes the result reproducible; the same harness
  then re-points the client at a *remote* server to prove the network
  path, while still controlling the DB.
- **Test every layer's variant of "connect".** Same client, multiple
  arrival paths: solo embedded server, direct host+port, URL/redirect,
  through a gateway/proxy, through a websocket bridge, IPv4 *and* IPv6
  (force the family with a literal-IP host). Each path has its own
  framing/handshake bugs.
- **Cover the cluster handshake, not just one box.** If the topology is
  login → token → game-server → master, a single-process test never
  exercises token validation, character locks, or cross-node state. Give
  the cluster its own harness with multiple roles wired together.

> CatchChallenger: `testingclient.py` runs the real client solo and
> against a local `server-filedb`; `testingcompilationwindows.py` builds
> the NSIS/MSI installer and runs the *installed* `.exe` under wine64;
> `testingcluster.py`/`testingclustersecurity.py` wire login+master+game
> together; the QLocalServer automation channel drives two real clients
> through trade/fight.

---

## 2. The platform matrix — one of every kind

Code has *induced behaviour*: the same source behaves differently per
compiler, libc, OS and runtime. You test each platform to learn whether
that difference is a bug **in your code** — and, if it's an external bug,
whether you can work around it on your side.

- **One target of every kind, every OS and version you support.** Not
  "Linux" — the *oldest* and *newest* glibc/kernel you claim. Not
  "Windows" — each Windows you ship to. macOS, the BSDs, Android, the
  consoles, the microcontroller. A platform skipped is a platform
  untested; "should be the same" is the sentence before the bug report.
- **Test to discover induced behaviour, then decide.** When a platform
  diverges, classify it: (a) our latent bug the platform exposed → fix
  our code; (b) an external/platform bug → can we route around it on our
  side (guard the input, hold a stronger reference, pick another code
  path) without forking the vendor? Prefer the workaround in *our* call
  site over patching vendored/upstream code.
- **Each platform port carries its own constraints — test them.** A
  no-filesystem MCU bakes data into flash; a DOS port loses
  `timerfd`/`sendfile`; a console sandboxes the network. The port isn't
  done until its *reduced* path is exercised end to end (server binds,
  loads its baked datapack, announces, accepts a connection).
- **Cross-compile is a test.** Compiling for a target you can't run is
  still a guard: it catches header/ABI/feature drift. Run it as a CI
  gate, pin **Release** for shipping artefacts (a Debug build is huge and
  must never reach an installer — guard on `.exe` size and `.debug_info`
  section size to catch a Debug-by-accident), and emulate-run wherever an
  emulator exists (wine, qemu-system-*, Android emulator).

- **Test the clones and the oddballs, not just the brand names.** A
  reimplementation (ReactOS for Windows, Haiku, Wine, a niche libc)
  surfaces every place you leaned on undocumented behaviour the real
  platform happened to tolerate. They are cheap, brutal conformance
  tests for your platform assumptions.
- **An optional subsystem missing must degrade, never crash.** TLS lib
  absent / offline, an accelerated I/O backend unavailable, a plugin not
  loaded, a CPU without the SIMD level the binary wants — each must fall
  back to a working path, not die. Test the *absence* of every optional
  dependency as its own case.

> CatchChallenger ships from one source to Linux, Windows (MXE), macOS
> (osxcross), Android (Qt-for-Android), MS-DOS (DJGPP+Watt-32), ESP32
> (no-FS datapack-in-flash), plus PS5/Switch notes — each with its own
> `testingcompilation*.py` and a documented reduced path. Ultracopier
> (15+ years field-stable) carries explicit ReactOS and Haiku
> workarounds, a non-SSE Windows-XP/Pentium-MMX fallback build, and
> survives "HTTPS offline / openssl dll not found" without crashing —
> each a bug found only by testing the oddball or the missing dependency.

---

## 3. The architecture matrix — endianness, alignment, word size

Portability bugs are silent on amd64 and fatal elsewhere. The only way
to find them is to run the real workload on the real ISA.

- **Big-endian AND little-endian.** Any code that serialises, hashes,
  memcpy's a struct, or reinterprets bytes is a candidate. A binary
  cache written little-endian won't load big-endian; a `float`/`double`
  blitted via raw `memcpy` is not portable. Run a BE box (or BE
  emulation) in the matrix and assert the on-wire/on-disk format is
  byte-identical across endianness — or explicitly host-order-normalised
  (`htole32`/`le32toh`) at the boundary.
- **Aligned AND unaligned.** x86 forgives unaligned access; ARM/MIPS/
  RISC-V may fault or trap-and-emulate (slow). Test an arch that *cares*
  so a packed-struct field read at an odd offset surfaces as a crash in
  CI, not a field crash report.
- **32-bit AND 64-bit, and the small word sizes.** Pointer size,
  `size_t`, `long`, time_t, and integer-promotion differences bite.
  Keep a 32-bit target live. Use the smallest integer that fits the
  domain (8-bit coords, 16-bit type IDs) — and then *test* that the
  bound actually holds on the narrow type on every arch.
- **The weakest hardware is a first-class node, run daily.** A 66 MHz
  i486 / Geode LX800 / RPi1 / MIPS2 is the canary: a change free on a
  modern core can be a visible stall there, and timing-dependent bugs
  (races, ordering, back-pressure) reproduce on slow silicon that hide
  on fast. A win on amd64 that regresses MIPS is not a win.

> CatchChallenger: 8-bit map coords (`COORD_TYPE`), 16-bit type IDs;
> `datapack-cache.bin` is explicitly "not endianless" (HPS raw-memcpy
> floats) so a LE-built cache is refused on BE; the fleet includes
> i486/Geode/MIPS LXC + cross nodes so every commit is validated across
> the ISA range.

---

## 4. Coverage: every action × typical traffic × machine-generated traffic

Three layers, in increasing volume. You need all three; each finds bugs
the others can't.

- **Every in-game action, deliberately scripted.** Walk every map,
  trigger every teleport, fight, catch, craft, trade, plant, capture a
  city, exhaust every menu and inventory operation. A feature with no
  test is a feature that regresses silently. This is the floor.
- **Typical multi-actor behaviour.** Many clients doing realistic things
  at once — movement, visibility enter/exit, simultaneous fights — to
  shake out interaction bugs (visibility cache coherency, ACK flow
  control, ordering) that a single actor never reaches.
- **Machine-generated behaviour at scale — toward millions of cases.**
  Hand-written cases are too few to cover the state space. Generate
  them: every packet/handler enumerated and driven with valid,
  boundary, and malformed payloads; every map round-tripped and
  re-rendered; every datapack permutation loaded. Make one case cheap
  (staging cache, parallelism) so you can run an enormous number.
- **Verify per-handler, in isolation, with a clean baseline.** Spin a
  fresh server per handler, drive *only* that handler, and diff its
  leak/error fingerprint against a do-nothing baseline so a new
  alloc-site or new valgrind context is unambiguously *that handler's*
  fault. Per-case isolation stops state bleeding from one case into the
  next and turning a real find into noise.

> CatchChallenger: `testingfight.py`, `testingmapmanagement.py`,
> `testingpathfinding.py` cover scripted actions; `testingmulti.py`/
> `testingbots.py` the multi-actor load; `testingprotocolstate.py` spins
> a fresh valgrind server per `h_*.py` handler test and reports
> baseline-delta leaks; `testingmap2png.py`/`testingmap4client.py`
> re-render every map and compare pixels.

---

## 5. The "should not occur" path IS the main test surface

The field is hostile and lossy. Every "this can't happen" is a daily
event at scale. Each one gets a test, and each one has a *correct*
behaviour you must assert — not merely "didn't crash".

- **Disconnect at every point in every flow.** Drop the socket mid-
  handshake, mid-login, mid-character-select, mid-datapack-sync, mid-
  fight, mid-trade, mid-download. For each: the server frees that
  session's resources, the *other* sessions are untouched, and no leak
  remains. Assert the cleanup, not just the survival.
- **Connect and then say nothing.** A peer that opens a socket and never
  sends a valid handshake must be **kicked after a bounded idle
  timeout** — not held forever (a slot-exhaustion DoS) and not crashed.
  Test the timer fires and the slot is reclaimed.
- **Define and test the BEST behaviour, not just a safe one.** For a
  transient network blip the right answer is often **silent, seamless
  reconnect/resume** with no user-visible interruption — so test that
  the client reconnects and resumes state without the player noticing,
  not merely that it eventually recovers. Pick the ideal behaviour for
  each fault and assert *that*.
- **Half-open and asymmetric loss.** One direction dies while the other
  lives; the OS hasn't noticed yet. Test that application-level
  keepalive/ACK eventually reaps the dead half.

> CatchChallenger memory: silent-ignore of invalid input is an
> acceptable, tested behaviour — a handler may safely *absorb* bad input
> without kicking; the contract test requires no-crash / no-corruption,
> not necessarily a kick.

---

## 6. Adversarial input — corruption and hack attempts

Assume the client is the attacker. The server is the only trust
boundary; it must treat every byte as hostile.

- **Check the return of every call; handle the error, never crash on
  it.** The default for *all* code — not just the security path — is to
  test every function's return value and resolve a failure gracefully
  (retry / skip / defer / fall back / return an error up / clean
  disconnect). **`abort()` is a last resort**, used only when there is
  genuinely no safe way to continue, and even then it must be *clean*
  error management (clear message, freed resources), never a bare crash.
  A live server/app degrades; it does not die because one call failed or
  one client misbehaved. Verify this **both by manual review and by
  test** — an unchecked return or an abort-on-error is itself a defect to
  catch.
- **Surface the error tiered to the audience, never spam — and always
  keep a full path open.** *Advanced/pro* software informs the user of
  every error, but rate-limited so it never floods. *Basic-user* software
  shows a simplified message, or stays silent in normal usage when
  nothing is actionable. **In all cases** there is a **debug mode that
  exposes the full error detail**, and the error is **always written to
  the console** regardless of UI tier. Test all three: the user-facing
  tier shows the right level, debug mode reveals the full detail, and the
  console line is present — a swallowed error with no console trace is a
  defect even when the UI is correctly quiet.
- **Corrupt deliberately, everywhere.** Flip bits, truncate packets,
  over-/under-state lengths, send out-of-range IDs, wrong-state packets
  (a fight action before logged in), oversized payloads, integer-
  overflow framing, malformed compression. For each: the server **kicks
  the offender cleanly** — no crash, no abort (in production), no UB —
  and **no other player is disconnected or corrupted**. That collateral
  check is the whole point: a crash or cross-session corruption is the
  difference between a kick and an exploit.
- **The lone sanctioned abort is a TEST-ONLY tripwire.** To stop a
  "should be impossible" breach from hiding inside a generic timeout, a
  parse-returns-false / invariant breach may `abort()` with the offending
  packet's hex dump **in a test build only**, so the bug surfaces as a
  backtrace. In **production the very same breach takes the graceful
  path** (clean disconnect, never SIGABRT-and-drop-every-player). One
  flag, opposite behaviour per build — this is the *only* place abort is
  acceptable, precisely because it never reaches a shipped binary.
- **Re-run an abort under a debugger for the backtrace.** A parse-fail
  abort is deterministic (same packet → same branch → same abort), so
  re-running under gdc/gdb reproduces it exactly — preferred over a live
  attach, which perturbs timing and partially unwinds before the handler
  lands.
- **Security gets its own battery, and any process exit is a FAIL.**
  Separate the "is it correct" suite from the "can it be broken" suite.
  In the abuse battery, *any* process exit during the run is a failure —
  a real server doesn't die because a client misbehaved.

> CatchChallenger: `-DCATCHCHALLENGER_HARDENED=ON` (on in every test
> build, off in production) turns parse-fail into an `abort()` with a hex
> dump; `process_helpers.rerun_under_gdb` captures the backtrace;
> `testingclustersecurity.py` treats any process exit during the battery
> as FAIL.

---

## 7. Be robust to micro-faults — flap, partial loss, reordering

Beyond clean disconnects, the network corrupts traffic in small,
annoying ways. These cause the subtle, intermittent field bugs.

- **Quick disconnect → immediate reconnect (flap).** A client that
  drops and reconnects within the same second must not leave a ghost
  session, a stale lock, a doubled entity, or a leaked slot. Test the
  rapid cycle, repeated, and assert server resource count returns to
  baseline each time.
- **Partial packet / partial read.** TCP gives you bytes, not messages:
  a packet arrives in two reads, two packets arrive in one read, a
  message straddles a buffer boundary. Drive the parser with every
  split point to prove it reassembles correctly and never advances on
  incomplete data.
- **Reorder / duplicate / delay.** Especially for anything UDP-like or
  ACK-gated: deliver out of order, twice, late. The protocol must be
  idempotent where it claims to be and latency-tolerant everywhere.
- **Resource-leak detection is the verdict, not a side-check.** Run the
  whole abnormal-path battery under a leak/error detector
  (valgrind/sanitizers) and make "introduced no new leak vs baseline"
  the pass condition — flapping and partial-read bugs almost always leak
  before they crash.

---

## 8. AI / fuzz testing — let a machine find what you didn't imagine

Scripted tests encode what you *thought of*. A fuzzer finds the rest.

- **Fuzz the protocol adversarially.** Mutate real captured traffic and
  generate structured-but-wrong packets; replay against the live server.
  The bar is the same as §6: kick or absorb cleanly, never crash, never
  collateral.
- **Seed and fingerprint for reproducibility.** A fuzz find is worthless
  if you can't reproduce it. Fix the seed, record the corpus/fingerprint,
  and make a failing input a permanent regression case.
- **An AI agent makes a good fuzzer and a good oracle.** It generates
  unusual-but-plausible sequences a random fuzzer won't, and it can
  judge whether the *response* was reasonable, not just whether the
  process lived. Let it drive bursts on purpose (so don't arm
  burst-tripwires on that suite).

> CatchChallenger: `testingbyIA.py` (+ `testingbyIA.fingerprint.json`)
> is the adversarial fuzz suite — it deliberately floods, so the
> event-rate tripwire is left OFF for it.

---

## 9. Critical subsystems get bespoke harnesses with every edge case

Some subsystems are load-bearing enough that a generic test isn't
enough — enumerate *their* specific failure modes.

- **Cluster / distributed state.** Test token expiry, node death mid-
  session, character-lock contention, split-brain, a game-server that
  vanishes while holding locks. Single-box tests never reach these.
- **Pathfinding / simulation core.** Determinism (same map+seed → same
  path), unreachable targets, degenerate maps, worst-case fan-out
  performance on the slow box, and agreement between client prediction
  and server authority.
- **HTTP / asset distribution — test the protocol's whole error
  surface.** Not just 200-OK download: **disconnect in the middle of a
  download** (and resume), `403`, `404`, redirect, truncated body,
  wrong/short `Content-Length`, slow drip (slowloris), corrupt
  compressed payload, a **partial pre-existing client cache** (resume
  only the missing parts), and the mirror-unavailable fallback. Each is a
  real field condition for an open mirror.
- **Image / render verification with tolerance, not checksums.** Lossless
  formats don't encode canonically — the same pixels produce different
  bytes per run. Never compare renders by md5/sha; use per-pixel
  tolerance with a diff-mask, and triage by diff-percentage (a tiny diff
  = a real renderer/RNG bug; a huge diff = wrong inputs/config). Then fix
  the *source* of non-determinism (pin RNG, freeze animation, lock GL
  state) rather than widening the tolerance.

> CatchChallenger: `testinghttp.py` covers partial cache, mirror
> fallback and 4xx; `testingpathfinding.py`, `testingcluster.py` and the
> ±10%-per-channel/0-pixel-budget image checkers
> (`testingmap2png.py`/`testingmap4client.py`) are the bespoke harnesses.

---

## 10. Load & DDoS — design for 100× nominal, and discriminate it

Robustness at the floor of hardware means the abuse ceiling is *close*,
so abuse-resistance is a design property you must test.

- **A DDoS is typically 100×+ your nominal load.** Test there: a burst
  far above the real peak. The goal is not to serve it — it's to (a)
  **survive** (no crash, no leak, no unbounded memory), (b) **bend, not
  spiral** (past a load knee, per-operation cost stays flat and only
  frequency/coverage degrades — never a death spiral), and (c) **shed
  the abuse while real clients keep playing**.
- **Make the system easy to discriminate attack from load.** The more
  sharply normal traffic is bounded (max visible peers, max packet rate,
  fixed small per-client buffers), the easier it is to tell an attacker
  apart from a legitimate spike — and the easier to drop only the
  attacker. Test that the discriminator actually fires on the 100× burst
  and *doesn't* on a legitimate crowd.
- **Cap per-child resources so a wedged/abused run dies clean.** In the
  harness, bound CPU-seconds and address space per spawned server (e.g.
  RLIMIT_CPU / RLIMIT_AS) so a tight wakeup loop or unbounded alloc
  surfaces as a clean abort with a backtrace, not a 14-hour orphan
  burning a core. Gate these off under valgrind (10–20× memory).
- **Tripwire the pathological loop.** A bounded event-rate guard (abort
  if the dispatcher delivers > N events/s with no legitimate burst)
  catches the EPOLLIN-never-cleared / re-arm-under-backpressure class
  *before* it eats a CPU core for hours. Leave it OFF for suites that
  flood on purpose; ON for everything with no legitimate burst.

> CatchChallenger: ≤254 visible peers per map and 1–4 KB/client are the
> bounds that make abuse cheap to spot; `RLIMIT_CPU`/`RLIMIT_AS` per
> child and `CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE` (1000 ev/s) are
> the tripwires; the DDoS sections of `testingbots.py`/`testingmulti.py`
> run the burst.

---

## 11. Test the build and the tools, not just the runtime

The shipped product includes its build system and its content tools.
Both ship bugs.

- **The build is a test target.** Configure + build every binary on a
  *vanilla* toolchain with no extra packages (prove it builds out of the
  box on a bare VPS), AND with every optional accelerator present (prove
  the auto-detect doesn't hard-require them). A build that only works on
  the maintainer's box is broken.
- **Content/asset tools get their own tests.** Converters, generators,
  packers, map tools — run them on real inputs and validate the output
  *loads and plays* in the engine, not just that the tool exits 0. A
  generator that emits a subtly-wrong asset is a content bug factory; its
  test is the early warning.
- **A tool failure often points at an engine bug.** When a converter or
  validator chokes on real data, it frequently exposes a latent engine
  assumption — treat the tool's complaint as a signal, not just a tool
  bug.

> CatchChallenger: `testingcmake.py` proves the per-binary CMake builds;
> `testingtools.py` exercises the content tools; map/datapack generators
> are validated by rendering their output through the real client.

---

## 12. Keep test/benchmark code out of the product

The binary you measure must be the binary you ship; otherwise the test
verdict describes a thing that doesn't exist.

- **Don't instrument the binary under test.** Adding trace prints, log
  lines or counters mid-test changes timing, stdout volume, pipe back-
  pressure and detector counts — the verdict no longer reflects the
  original bug. Workflow: kill the run → add traces → rebuild → re-run
  from scratch → read → strip every trace → rebuild → re-run to confirm.
  Never edit source *between* cases that share a build.
- **Keep core code and non-core (tools/tests/benchmarks) cleanly
  separated.** Test-only behaviour lives behind a flag that is OFF in
  production (hardening aborts, event tripwire, resource caps) or in
  separate harness code entirely — never woven into the hot path where it
  can have side effects on the shipped behaviour. Zero cost when off.
- **Generated/temporal test data lives outside the source of truth.**
  Staging copies, caches, tags, learned state go in a dedicated work dir
  keyed to the input — never written back into the read-only datapack/
  asset tree, so the same assets test identically everywhere and a
  rogue write can't corrupt the source.
- **Build artefacts never land next to checked-in sources.** Out-of-tree
  build dirs; generated files (objects, moc/qrc/ui, Makefile, caches)
  must never appear beside the `.cpp`/`.hpp`. Fix the tool, don't
  `.gitignore` the mess.

---

## 13. Determinism is the precondition for everything above

Every assertion in this skill — pixel match, leak baseline-delta, fuzz
reproduction, cross-arch byte-identity — collapses if the system isn't
deterministic.

- **Same input → same output, every run, every arch.** Pin every RNG
  seed, freeze animation/clock, lock GL/render state, fix iteration
  order of unordered containers where it reaches output. A flaky test is
  a *real* bug (usually an uninitialised value, an ordering dependency,
  or a hidden clock) — find it, don't retry-until-green.
- **Fixed-time, not fixed-iteration, for throughput claims.** Measure
  work in a fixed wall-clock budget so a slow arch's number is
  comparable to a fast one's, instead of just taking longer and hiding a
  regression.
- **Isolate every case.** Fresh process, fresh run dir, fresh DB/state
  per case so nothing bleeds forward. Shared mutable state is the
  number-one source of "passes alone, fails in the suite".
- **Reproduce a heisenbug by changing tools laterally.** If it vanishes
  under valgrind (timing), try sanitizers or gdb; if it only shows
  naked, capture a core. Match the tool to the bug's nature.

---

## 14. Make failures legible and the harness fast

A test suite nobody can read or afford to run gets disabled — which is
the same as not having it.

- **A failure must name the bug, not just say FAIL.** Emit the exact
  diff (failing pixel + channel + coords + mask path; the offending
  packet's hex; the new leak's alloc-site). On a hang, capture the stack
  (gdb `thread apply all bt`) before killing — and read it: blocked on
  `epoll_wait`/`recv` is idle (not a hang); a tight loop in your code is
  the bug.
- **Bound every test's wall time, sized to ~2× its healthy run.**
  Generous enough that a healthy run never trips, tight enough that a
  wedge surfaces as a TIMEOUT instead of soaking the build. Enforce from
  outside (a `timeout` wrapper) and inside (a watchdog that dumps a
  traceback before the kill). Fix the bug; don't widen the cap.
- **Never silently drop a test.** The total count must not decrease. An
  environment-broken test is a FAIL/TIMEOUT worth surfacing, not a
  commented-out line. Fix the host or the test.
- **Only changes that move FAIL→PASS stay.** No "maybe this helps"
  patches; revert anything that doesn't demonstrably fix a failure.
- **Spend the host: overlap and cache.** Stage shared inputs once and
  symlink (RAM-disk); parallelise independent suites and per-node loops;
  keep a compile or a test phase always running. Observe where the wall
  time goes (per-phase timing + host CPU/RAM sampling) and attack the
  dominant phase — parallelise if idle-waiting, cache/`-j` if one core
  is pinned.

> CatchChallenger: `all.sh` enforces per-script wall caps + a `tee`'d
> console log + per-phase `time.json` + `monitor.json`; `failed.json`
> drives `--continue` to re-run only failures; `stage_datapacks.py`
> stages once to tmpfs and symlinks; the remote-node thread pool overlaps
> compile and test across the fleet.

---

## 15. Irreversible operations — data safety is the prime directive

For any operation a user can't undo — moving/deleting files, writing a
save, mutating a record, a destructive migration — "didn't crash" is not
enough. **Losing or corrupting one byte of user data is the worst bug
the software can have**, worse than a crash, and it earns the most test
surface. This is the discipline that makes software *trusted* over years,
not just *correct* on the happy path.

- **The source is sacred until the destination is verified.** For a
  move = copy + verify + delete, never delete/modify the source before
  the destination is confirmed good (size + checksum). Order the steps so
  any interruption leaves the original intact. Test the interruption at
  each step and assert the source survived.
- **Never let a partial result masquerade as a complete one.** Write to a
  temp/`.unfinished` name and **atomically rename on success** so an
  interrupted write is recognisably incomplete, never a truncated file
  with the final name. Test: kill the process mid-write and assert the
  destination is either absent/temp or byte-complete — never a silent
  half-file. Then test **resume** picks up exactly where it stopped.
- **Verify the write, and know the limit of verification.** Offer a
  checksum of what landed — but document that a device which reports a
  good value while having physically written garbage defeats any
  copy-time check. Don't claim a guarantee the storage layer can't keep.
- **Refuse the operations that are silently destructive.** Move a folder
  into itself; copy that would overwrite the only copy; a path that
  resolves through a symlink/junction out of the intended tree; a target
  on a known-flaky network location. Maintain a blacklist/guard for
  classes of operation that have caused data loss, and test each is
  refused — *preventing* the loss is the test, not recovering from it.
- **Cross-boundary moves must fall back, not fail or half-move.** A
  rename across filesystems/drives (`EXDEV`/errno 18) can't be atomic —
  fall back to copy-then-delete with the verify-before-delete ordering
  above. Test a move whose source and destination are on different media.

> Ultracopier lineage: collision/error matrices below; temp-file +
> rename on completion; "black list some network folder to prevent data
> lost"; "fix move folder into it self"; "errno=18 → switch real-move to
> full copy"; checksum (xxh) verify; disabled shift-drag move on Windows
> because the OS routed deletes through the trash. Each line is a data-
> loss bug that became a permanent guard + test.

## 16. Enumerate the external-resource failure taxonomy

Every operation on something you don't own — a file, a socket, a disk, a
remote service — has a *fixed, enumerable* set of failure modes. Trusted
software is the software that has a defined, tested behaviour for **each
one**, instead of an exception handler that hopes.

- **List the failure modes per resource and test each deliberately.**
  For storage: not-found, permission denied, read-only target, disk
  full (`ENOSPC`), path-too-long, file locked/in-use by another process,
  name collision, device removed mid-operation, I/O error on a bad
  sector. For network: refused, reset, timeout, half-open, DNS failure,
  TLS failure. Don't wait to encounter them — *inject* each and assert
  the defined outcome.
- **Every recoverable error needs the same four resolutions.**
  **Retry** (the failure was transient), **Skip** (abandon this item,
  continue the rest), **Abort** (stop everything cleanly), and **Defer**
  (push this item to the end of the queue so one stuck item never blocks
  the other thousand). Test all four paths per error type — the *defer*
  path especially, because a queue that head-of-line-blocks on one bad
  item is a classic field hang.
- **Collisions are a matrix, not a yes/no.** Skip / Overwrite /
  Overwrite-if-newer / -if-older / -if-different-size / -if-different-
  date / Rename / Cancel — plus a remembered default so the user isn't
  asked 10 000 times. Test each branch, and test the "always do this
  action" default actually persists and applies.
- **Errors happen on the destination, not the source — assert which
  side.** Most failures of a transform/copy are on the *output* side;
  conflating them with source errors sends the user chasing the wrong
  thing. Test that the error names the correct side.
- **Test against every medium and mount type.** Local disk, USB, SD, a
  NAS accessed directly vs mounted as a network drive, a slow device.
  The same code path behaves differently on each — and the slow/odd
  medium is where the timeout, buffering and locking bugs live.

> Ultracopier encodes exactly this: `FileExists_{Skip,Overwrite,
> OverwriteIfNewer,OverwriteIfOlder,OverwriteIfNotSameSize,…,Rename,
> Cancel}` and `FileError_{Retry,Skip,PutToEndOfTheList,Cancel}` — a
> remembered default per type, a "put to bottom" defer so one locked
> file can't stall the batch, and per-medium handling (NAS-as-network-
> drive workaround, per-device buffer sizing).

## 17. Tell your bug from the environment — and make every report reproducible

Not every failure is your code. Trusted software can *tell the
difference* fast, and turns every field report into a repeatable test.

- **Distinguish your-bug from bad hardware/environment.** A "random
  crash with no operation in progress" is often faulty RAM, a bad disk,
  or a hostile AV/sandbox — not your code. Reproduce on both real
  hardware and a clean VM; if it only happens on one box, suspect the
  box (memtest, SMART, disable the AV) before burning days in the
  debugger. Don't "fix" a phantom that lives in someone's RAM.
- **Pin down reproducibility before debugging.** The questions that
  collapse the search space: exact version, is it systematic or random,
  does it happen with the *single smallest* input, and what are the
  resource types involved (local/USB/NAS, IPv4/IPv6, which OS build).
  Bake these into the bug-report template and into the regression test
  you write from the report.
- **Every confirmed field bug becomes a permanent guard + test, in the
  same change.** The reason a 15-year-old tool is still trusted is that
  each data-loss or crash bug, once understood, was turned into a blacklist
  entry, a fallback, or an assertion *and* a test — so it can never
  silently return. Fast turnaround + permanent regression is the moat.
  (A guard handles the bad case gracefully — it is not an abort; see §6.)
- **Check the shipped artefact one last time before publishing.** Verify
  the final binary's size/shape against a known baseline before it goes
  out — a Debug-by-accident, a missing-strip, or a packaging regression
  is caught here, after every other gate.

> Ultracopier: FAQ explicitly triages "random crash, no copy" → test
> RAM with memtest86+; the bug-report protocol asks version /
> reproducible? / single file? / source+dest type; "now check binary
> size before publish it" is a shipped CHANGELOG line. CatchChallenger's
> `size_check.py` is the same last-gate idea.

## The condensed loop

1. **Enumerate the failure modes** — platforms, arches, actions, faults,
   abuses — and write one test each. The unwritten test is the shipped bug.
2. **Run it on the worst hardware/network and on every ISA**, not just
   the dev box.
3. **Drive the real artefact end to end** — installer → client → server
   → DB — deterministically, you owning both ends.
4. **Spend most of the budget on the abnormal path** — disconnect,
   corruption, idle, loss, flap, 100× load — asserting the *correct*
   behaviour, not just survival.
5. **For anything irreversible, prove no data is ever lost** — interrupt
   at any byte, inject every external-resource failure, verify before
   you delete, refuse the silently-destructive operation.
6. **Detect leaks/errors as the verdict**, isolate every case, keep
   instrumentation out of the shipped binary.
7. **Make failures legible and the suite fast**, never drop a case, keep
   only fixes that move FAIL→PASS — and turn every field bug into a
   permanent guard + regression in the same change.
