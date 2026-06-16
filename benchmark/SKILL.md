---
name: high-performance-low-ram-client-server
description: >
  Abstract, engine-agnostic playbook for building extremely fast,
  extremely small-footprint client/server software (real-time games,
  MMOs, low-latency services). Distils the architecture that lets one
  single-threaded server hold hundreds of clients in 10-20 MB base RAM
  plus <1 KB per client, stay playable over <10 KB/s / >1000 ms links,
  saturate into a stable plateau instead of a death spiral, and run on
  hardware as weak as a 66 MHz i486 or an AMD Geode LX800 — while the
  SAME source also scales out to a multi-role cluster and down to a
  no-filesystem microcontroller. Use when designing a protocol, a hot
  path, a memory layout, a persistence/distribution strategy, or a
  cluster, where performance-per-watt and operability on bad hardware /
  bad networks are first-class goals.
---

# High-performance, low-RAM client/server design

Hard-won, empirical doctrine — not generic advice. It targets software
that must be **fast on weak hardware** and **playable on bad networks**,
where every byte of RAM and every round-trip is a cost. The running
example is a real-time MMO engine, but every principle is written to
transfer to any client/server system.

The invariants the whole design serves:

- **<1 KB of RAM per connected client**, **10-20 MB base** for a server
  holding hundreds of clients.
- **One core, no locks** — a single-threaded event loop scales
  vertically before anything is allowed to go horizontal.
- **Playable on a <10 KB/s, >1000 ms, lossy link** — the protocol is
  async end to end and latency-tolerant, never round-trip-bound.
- **Saturates into a plateau, not a spiral** — past a load knee set by
  the node's CPU/bandwidth, per-operation cost stays flat; only
  frequency/coverage degrades.
- **Runs on the worst hardware we ship to** (i486-class, Geode LX800,
  MIPS2, RISC-V, RPi1) and the worst we can imagine (a no-filesystem
  MCU), from one source tree.

The recurring meta-principle behind almost every section: **move work to
where it is paid once — build time, load time, or one machine in the
cluster — so the hot per-tick path is pure reads.**

**Server hot-path keys (in priority order).** Almost all *server*
performance reduces to five levers — the rest of this document elaborates
them:

1. **Async end to end** — non-blocking I/O, never a synchronous
   round-trip on the hot path (§5.2).
2. **Async DB** — `asyncRead`/`asyncWrite` return immediately, results
   arrive via a queued callback; the loop never stalls on the store
   (§5.2, §9).
3. **Prepared SQL** — cache prepared statements per backend, bind only
   on the hot path; no concatenation/re-parse per query (§4); no joins,
   fetch related rows in separate queries (§9).
4. **epoll, or io_uring where available** — one event-loop façade,
   backend chosen at build time, no runtime branch (§2).
5. **Cache-resident hot set** — keep the per-tick working set lean and
   in L1/L2; preprocess at load so the loop only reads (§3, §4).
6. **Map management is the dominant hot path** — broadcasting each
   player's move to every other player on the same map so everyone sees
   everyone is naively **O(n²)** (n players × n visible peers) *and* runs
   constantly (every step of every player), so it dominates both CPU and
   **bandwidth** — the single most expensive ongoing cost (the way the
   one-time datapack load dominates startup). Attack it with: serialise
   once / send to many (§5.5), a hard visible-peer cap that converts
   O(n²)→O(1) past the knee (§3, §6), and latest-state coalescing instead
   of an event backlog (§5.7).

---

## 1. Measure on the worst case, every day

Optimisation is data-driven, and the data comes from the *floor* of the
hardware/network range, not the ceiling.

- **Daily worst-case-hardware run.** Keep a genuinely slow machine in the
  loop as a *daily* gate (e.g. an AMD Geode LX800). A change that is free
  on a modern core can be a visible stall there; you only catch it by
  running the real workload on the slow silicon continuously. The slow
  box is the canary for accidental per-tick cost.
- **Daily worst-case-network run.** Exercise the protocol over a
  deliberately awful link — high jitter, **<10 KB/s**, **>1000 ms** ping
  (an anonymising overlay such as I2P is a convenient source of exactly
  this). If the game is still playable there, the protocol is genuinely
  async and latency-tolerant; if it stalls, you have a hidden synchronous
  round-trip to find and kill. This is how you *prove* client-side
  prediction actually covers the hot paths.
- **Cross-environment matrix.** Hardware behaves non-linearly. Validate
  from modern CPUs down to embedded SoCs. Skipping a noisy node is fine;
  skipping an *architecture* is not — a win on amd64 that regresses MIPS
  is not a win.
- **Fixed-time, not fixed-iteration.** Measure work done in a fixed
  wall-clock budget (ops in 10 min), never time-to-complete-N. Fixed
  counts just take longer on a slow arch and hide throughput
  regressions; fixed-time makes i486↔amd64 numbers comparable.
- **Document deliberate trade-offs; reject brute force.** When no
  universal win exists, choose consciously and back it with a benchmark.
  Spending 16× compute for 2× throughput is an architectural failure —
  optimise performance-per-watt before adding boxes.
- **Benchmarks detect, profilers locate, and only steady state counts.**
  A before/after benchmark says *whether* the fleet moved; profilers say
  *where*. Use several (sampling + instruction-accurate + allocation +
  RSS). Exclude startup/linking/load from the profile — a long-lived
  server's only meaningful metric is per-tick cost, and a change that
  adds startup work to cut per-tick cost is a *win*.

---

## 2. The single-threaded event-loop core (why it's fast)

- **One thread on an I/O multiplexer, no locks.** No mutexes, no atomics,
  no cache-line bouncing, no context switches, no lock-ordering bugs. The
  working set stays hot in L1/L2. This is *also* what makes the code
  portable to i486-class hardware. Multi-threading and clustering are
  introduced only when single-core is mathematically exhausted **and**
  the use-case demands it — synchronisation overhead is never paid
  speculatively. **Vertical before horizontal.**
- **Optimise only proven hot paths.** Profile to find where cycles land;
  optimise that and leave the rest maximally clear. The single-threaded
  event loop is intentionally bounded (e.g. ≤254 visible peers per map,
  §3) so per-tick cost has a ceiling regardless of load.
- **Algorithms that respect the machine.** Textbook-optimal isn't
  hardware-optimal. Logic that minimises branch mispredictions and
  pointer-chasing beats a lower-Big-O algorithm that stalls the pipeline.
  Design for how the CPU executes, not just the asymptotic count.
- **Counter-intuitively, doing *more* — simpler — work is often faster.**
  On real hardware the bottleneck is usually cache misses and
  mispredicted branches, not instruction count. A bulk, branch-free,
  cache-resident pass repeatedly beats a "clever" one that does less work
  but chases pointers or branches:
  - Broadcasting the *full* current state to everyone (more bytes) can
    cost less CPU than computing a minimal per-recipient diff (less data,
    but scattered reads + per-recipient state + branches) — see §5.5.
  - A plain scalar equality loop over a packed array can beat an
    SSE/`memcmp` "prescan" skip-gate: at realistic change rates the gate
    pays its compare on most groups and then walks them anyway, so the
    extra branch is pure loss. *Measure the skip before adding it.*
  - Static, preallocated, reused fixed buffers (no per-tick allocation)
    stay cache-hot; a fresh allocation each tick is slower even though it
    "uses less memory at rest".
  - Tightening a bound can remove work: capping a list at 254 lets every
    index be one byte — that "drops a branch, halves the network width,
    and is less code" all at once. A constraint can be an optimisation.
  - Keep the always-resident hot structure *lean* so it stays in cache;
    never park temporaries/buffers inside it. Cache residency of the hot
    data is worth more than the few bytes a temporary would save.
- **One façade, many I/O backends, picked at build time.** All call sites
  speak one event API (the epoll vocabulary); the backend is `epoll` by
  default with `select`/`poll`/`io_uring` selectable by build define, the
  non-default ones translating internally. Porting to a new OS or a
  constrained target (DOS, an RTOS) is one adapter class, not a rewrite
  of every call site, and there is **no runtime branch** on the hot path.

---

## 3. Memory as a cache-residency problem (why it's small)

Low memory is not mainly about saving RAM; it is about keeping the
working set close to the ALU so the runtime loop never waits on DRAM.

- **Compact type widths, deliberately.** Size every field to its real
  domain, not to `int`. Coordinates that never exceed 127 are one byte;
  entity IDs that fit 65 535 are 16-bit; flags/levels are 8-bit. Tiny
  fields shrink every structure *and* every packet that carries them, and
  pack more entities per cache line.
- **Shared immutable data, per-client mutable delta only.** Load all
  static content (item/monster/skill/map definitions — the "datapack")
  **once** into a single read-only singleton shared by every client.
  Clients reference it by ID — zero-copy, never duplicated. Per-client
  cost is therefore only *mutable* state, which is what keeps it under
  ~1 KB. The 10-20 MB of content is paid once for the whole process.
- **Flat 1D arrays over pointer structures; reduce richness at load.**
  Store grid/collision data as a flat `array[x + y*width]`, one byte per
  cell. The rich authoring format (multi-layer tile map) is **collapsed
  at load into the runtime-minimal representation** (walkable / blocked /
  ledge / zone-id). Predictable, contiguous, a couple of cache misses to
  resolve any cell, and trivially serialisable. Apply the pattern
  generally: authoring-format richness is for tools; the runtime gets the
  minimal reduction.
- **Sparse maps for optional state.** State most clients don't have
  (per-zone progress, reputation, active quests, item stacks) lives in
  sparse maps keyed by ID, allocated on demand — absent entries cost
  nothing. Dense arrays only where occupancy is near-total.
- **Pack hot snapshots; diff the packed form.** For per-tick fan-out,
  pre-compose each visible entity into a flat array of small packed words
  (e.g. `x | y<<8 | dir<<16 | id`). One contiguous buffer instead of
  scattered object reads; one word-compare per slot instead of
  field-by-field; one `memcpy` to roll the snapshot forward. A truncated
  variant (fewer id bits) halves the footprint at the cost of a rare,
  self-correcting collision — an explicit, measured trade.
- **Bound the working set.** Cap how much any client can force the server
  to touch: a hard cap on visible peers (slot indices in a `uint8_t`)
  makes per-tick work and per-tick memory static, immune to crowd-size
  spikes.
- **Page space on demand.** The world is a **graph of small maps linked
  at their borders**, loaded as players approach and dropped when empty —
  never one giant map in RAM. Bounds memory regardless of world size;
  generalises to any large spatial/graph dataset.
- **Keep the always-resident hot structures lean.** The data touched
  every tick (map logic, per-map state) is in cache permanently; do not
  grow it without need and never store scratch buffers inside it — a
  temporary parked in a hot struct evicts real working-set data. This is
  the direct counter to the saturation death-spiral (§6).
- **Layout discipline.** Alignment, struct packing, predictable access so
  hot structures fit L1/L2; order fields to avoid padding; group fields
  touched together.

---

## 4. Preprocess at init; the runtime loop only reads finalised state

Shift every computation you can to a phase paid once, so the hot loop is
pure reads.

- **Ultra-preprocessed state.** Resolve names→IDs, build relationship
  tables, validate references at load. The per-tick path reads finalised
  structures and never parses, validates, or string-matches.
- **Binary cache for instant warm start.** Serialise the fully-resolved
  content (and per-client save state) to a binary cache and `memcpy` it
  back on boot — no XML/JSON re-parse per restart. Warm boot is
  load-blob-into-struct.
- **Precompiled query/message skeletons.** Split a templated string
  (a SQL statement, a protocol message) once into `[static, hole, static,
  hole, static]`; at runtime *compose* it by `memcpy`-ing the constant
  chunks and filling only the holes — no concatenation, no re-parse, no
  allocation per use. Likewise cache prepared statements per backend and
  only bind parameters on the hot path.
- **Intern repeated values to small indices.** Build static dictionaries
  mapping arbitrary database IDs / strings to compact internal indices
  once at load. Thereafter transmit and store the 1-byte index, not the
  20-byte string — a 10-20× shrink for repeated enums (skins, map IDs,
  types) that compounds across every client and every packet. Both ends
  hold the same dictionary (synced at login) and reconstruct locally.
- **One-time costs are not regressions.** Startup linking, content load,
  relocation are paid once over a multi-hour run — weight them ~zero.
  Front-loading work to load time to make every later call a direct hit
  is the pattern, not the anti-pattern.

---

## 5. The protocol

Where performance, RAM and bad-network resilience meet.

### 5.1 Dense, portable binary framing

- **Custom binary packing, never a verbose text format on the wire.**
  Eradicate every useless byte.
- **Tiny header.** One-byte opcode; a one-byte query-id only when a reply
  is expected; a length field **only for variable-size packets** —
  fixed-size opcodes are looked up in a size table and carry **no length
  and no delimiters**. A "move" can be 2 bytes. Direction/reply is
  encoded in the opcode space so a single byte classifies packet vs query
  vs reply and its direction. The size table doubles as a security gate:
  pre-trust opcodes are marked fixed-size (or blocked) so an unauthenticated
  peer can never declare an attacker-chosen length (see §10).
- **Endian- and alignment-portable by construction.** Every multi-byte
  field is read/written through explicit little-endian conversion and
  *unaligned-safe* loads (a small fixed-size `memcpy` that the compiler
  folds to a single instruction on x86 and to safe byte loads on strict
  MIPS/ARM/RISC-V). The same wire bytes and the same on-disk cache work
  on every endianness and alignment regime — no per-arch format.

### 5.2 Async and parallel end to end

- **Non-blocking I/O everywhere.** The parser is callback-driven per loop
  iteration: read what's available (bounded per read), process it, stash
  any partial-packet tail to prepend next time, return. Nothing in the
  hot path blocks on a socket.
- **Parallel in-flight queries with id correlation.** Many requests can
  be outstanding at once; each carries a small query-id (a fixed pool)
  and replies match back by id via O(1) array lookup — no per-request
  allocation, no hash map. Out-of-order replies are fine.
- **Async, parallel database access on the same model.**
  `asyncRead`/`asyncWrite` return immediately; results arrive later via a
  queued callback (FIFO, bounded depth). The event loop never stalls on
  the DB and many queries are in flight at once. The bad-network daily
  test (§1) is what *proves* nothing is secretly synchronous.

### 5.3 Selective compression — only where it pays

- **Compress only payloads proven to benefit** — large, bulk,
  once-per-session transfers (the character/world load). Measure first.
- **Never compress hot per-tick packets** (move, chat, ping): the CPU and
  latency cost outweighs any gain on tiny packets and taxes exactly the
  constrained CPUs we exist for.
- **Compile-time and runtime off switches.** The whole compression module
  is behind a build flag (zero cost when absent) and a runtime
  `compression=none`; the uncompressed path is a first-class branch.

### 5.4 Static parts: build once, memcpy

- **Pre-build constant packets and headers at startup**, then `memcpy`
  them out, patching only the few dynamic bytes (a query-id, a token).
  Fixed reply templates and the bulk-load header are computed once. The
  send path becomes *copy a constant + fill a couple of bytes + append
  dynamic data* — no per-send reconstruction, minimal cache pollution.

### 5.5 Serialize once, send to many (fan-out)

- **Build a broadcast packet once; blast the same bytes to every
  recipient**, excluding only the originator, instead of re-serialising
  per recipient. The naive "loop recipients, serialise each one's view"
  is **O(N²)** on a crowded map (N recipients × N visible peers); building
  one shared snapshot per map per tick and reusing those exact bytes for
  all N sends makes the CPU **O(N)**. This single change — *serialise once,
  send raw N times* — is the biggest fan-out win and the dominant cost in
  any one-event-many-subscribers system (map state, chat, presence).
- **Expose the CPU↔bandwidth trade as a knob.** Two interchangeable
  strategies over the *same* packed format: **min-CPU** (stateless —
  build the full-state snapshot once and send it to everyone every tick,
  clients filter out self/unchanged; **O(N) CPU**, more network) and
  **min-network** (stateful — keep each recipient's last-sent state and
  diff against it, send only inserts/removes/moves; more CPU per tick,
  far less network). min-CPU is the case where *more data is less CPU*:
  the redundant full broadcast is one cache-hot, branch-free serialise
  reused N times, whereas the bandwidth-minimal diff costs per-recipient
  state and scattered comparisons. Pick per deployment — constrained-CPU
  and constrained-bandwidth servers want opposite ends — over one packed
  format so switching is configuration, not a rewrite.

### 5.6 Latency tolerance

- **Minimise round-trips.** Every avoidable request/response is dead time
  on a 1000 ms link. Batch, pipeline, prefer push-when-ready over poll.
  Batch repeated identical actions into one packet (e.g. "move N steps in
  direction D") rather than one packet per step.
- **Push results asynchronously.** Expensive post-load results (computed
  rewards, deferred catch-up) never block the initial response: send the
  client what it needs to start *now*, deliver the computed extras
  whenever the server can; the client is built to accept them late.

### 5.7 Make a reliable stream behave like latest-state datagrams

A reliable, ordered transport (TCP) will faithfully queue *every* update
you hand it — so a slow or saturated client accumulates a growing backlog
of **stale** positions and the link suffers ever-rising latency
(bufferbloat / head-of-line). For state that is only interesting at its
*latest* value (where is everyone now), that ordered reliability is a
liability. Recover UDP's "newest datagram wins" semantics **on top of**
TCP, application-side:

- **Send latest-state snapshots, not an event log.** Each tick emits the
  *current* map state (full snapshot in min-CPU, or a diff against
  last-sent in min-network) — never a queue of individual past moves. An
  update the client was too slow to consume is simply **superseded** by
  the next tick's fresher one; intermediate states are coalesced away, not
  replayed.
- **Gate the next update on the previous being consumed.** Ride a tiny
  reply-required token (a "ping") at the *end* of each update; the client
  only answers it after it has processed that whole update. The server
  sends a new token only when the previous one is still unanswered count
  ≤ 0 — i.e. **when the client hasn't finished the last update, don't pile
  on another**. This is explicit application flow-control + keepalive
  (it also detects a dead client) using the same query-id machinery as
  everything else, with no extra socket state.
- **Never grow a per-client backlog of old data.** Write straight to the
  socket; do not maintain an unbounded application-side send queue. If the
  kernel buffer won't accept the write the client is hopelessly behind —
  drop/disconnect rather than hoard stale state. Per-client memory stays
  O(1) and the TCP window never fills with outdated positions.
- **Under saturation, degrade by *dropping* work, not by queueing it.**
  When a map exceeds the visible cap, *stop sending* its updates for this
  tick rather than build an ever-deeper send queue; the player still gets
  the next tick's fresh snapshot. Bounded, self-healing, and stable on a
  <10 KB/s / >1000 ms link — vs. a backlog that would make it unplayable.

The general rule: for *latest-value* state, prefer freshness over
completeness — coalesce, cap, and drop intermediates; never let a slow
consumer dictate a growing backlog.

---

## 6. Graceful under saturation — bend the curve, don't spiral

Most servers degrade *non-linearly* under load: saturation evicts the
working set from cache, which slows every operation, which deepens the
saturation — a death spiral. And the naive per-tick cost is **O(n²)**
(each of n clients gets a view computed over its n peers), so it doesn't
slow gently, it falls off a cliff. The design goal is the opposite: a
curve that **flattens** at a critical load (set by the node's CPU and
bandwidth) and holds throughput roughly constant. Past that knee the hot
operations must be **O(1) or O(log n)**, not O(n²) — *adding load changes
frequency/coverage, never per-operation cost*. The mechanisms:

- **No growing per-client send queue (O(1) memory under load).** Write
  straight to the socket; never accumulate a backlog of stale state in
  server RAM (§5.7). A client too far behind to drain is dropped, not
  hoarded. The server spends zero CPU managing deep queues and never
  fills the TCP window with old data — the single biggest source of the
  "more players → more stale buffering → collapse" spiral is simply
  absent.
- **Throttle update *frequency* to the consumer, not the producer.** A
  saturated client is updated **less often** — the latest-state + ping-ack
  gate (§5.7) ties each client's outbound cadence to its own drain rate.
  Skipping a lagging client is one comparison: **O(1)**. No matter how
  many players join, each client's outbound bandwidth is capped, so the
  link's maximum is *preserved* instead of being blown by stale backlog.
- **Caps that convert O(n²) into O(1) past a threshold.** When a map
  exceeds its population cap the broadcast returns *immediately* instead
  of doing the O(n²) diff; the per-map cost is bounded by a constant (a
  hard 254-slot limit plus a configurable max). Crowding a single map
  cannot take the server down — the cost stops *growing*, the surplus
  players just don't receive that tick's visibility. The constraint is
  the safety valve.
- **Load-adaptive cadence.** Background/aggregate updates back off as load
  rises — e.g. a player-count heartbeat fires every 500 ms under 30
  players, every 15 s past 30, every 60 s past 1000. Frequency drops as n
  grows so *frequency × n* stays roughly constant: **O(1) aggregate
  bandwidth near saturation** instead of O(n) growth. Generalise: anything
  whose per-event cost scales with population should lower its rate as
  population rises.
- **Group/pack to stay cache-resident (break the cache half of the
  spiral).** Build one shared snapshot per map and diff against contiguous
  packed state (§5.5); keep the always-resident hot structures lean (§3)
  so heavy load doesn't evict them. The hot path's footprint must not grow
  with load — that is what stops "saturated → cache-cold → more saturated".
- **Prefer the mode whose cost grows *predictably*.** Near the knee, the
  redundant-but-flat min-CPU broadcast (O(n), cache-hot, branch-free) is
  safer than a "smarter" per-recipient diff whose state and scattered
  access balloon exactly when you can least afford it. Predictable-linear
  beats clever-but-spiky under stress.

Closing principle: choose data structures and cadences so that **load past
the knee changes what you send and how often — never how expensive each
operation is**. The system then saturates into a stable plateau bounded by
hardware, not into a collapse.

---

## 7. Shared deterministic simulation — the foundation of prediction

This is *why* client-side prediction (§8) is correct rather than hopeful.

- **Run the identical simulation on both ends.** The core game logic (the
  battle/turn engine) lives in shared code and is inherited by **both**
  the client and the server. Both compute the same rules over the same
  shared content definitions.
- **Determinism via a shared random stream, not local RNG.** Randomness
  does not come from `rand()` at decision time; it comes from a
  **pre-shared list of random bytes** that client and server consume **in
  lockstep** (same values, same order, one draw per event). Given the
  same starting state and the same stream, both sides independently
  arrive at the *identical* outcome.
- **Consequence: no per-turn round-trip.** The client predicts the result
  locally and shows it immediately; the server computes the same result
  and only needs to transmit the *inputs* (which action was chosen) and
  authoritative *exceptions*, not every derived effect. Zero-RTT
  gameplay that is still server-authoritative.
- **Template-method layering makes this zero-cost.** Pure shared logic
  sits in a base class with abstract hooks; the side-specific concerns
  (where randomness comes from, how to persist, how to send) are
  overridden in the client and server subclasses. 100 % of the rules are
  shared by inheritance — a balance change propagates to both ends with
  no copy-paste and no version skew — and the only indirection is a
  per-call virtual dispatch chosen once per object, never a hot-path
  `if`.

---

## 8. Client-side prediction — the heart of bad-network playability

The dominant pattern for hiding latency, made *correct* by §7: **the
client predicts optimistically and updates its UI now; the server stays
authoritative; the client reconciles when the real answer arrives.**
Apply wherever a wrong guess is cheap to correct. Concrete shapes:

- **Fire-and-forget UI (chat).** A chat message effectively never fails,
  so render it *immediately* on send — don't gate it on a server echo.
  The server still broadcasts; the local echo just isn't on the
  round-trip. Any message class that can't meaningfully be rejected
  should appear instantly.
- **Optimistic resource spend with reconcile (item use).** On using a
  consumable, **decrement the local inventory immediately** and queue the
  pending use (FIFO). When the server replies, reconcile against the
  queue head: *consumed* → keep the decrement; *failed* → release the
  item back. Instant response; correctness restored on confirmation.
- **Shared synchronised randomness (wild encounters).** Both sides
  consume the §7 shared stream in lockstep, so the client predicts
  locally — step by step — whether an encounter fires and which entity
  appears, with **no request**. The server **refills the stream when it's
  ~half-consumed** (pushes more values once remaining drops below a
  threshold), so it never runs dry mid-play. Deterministic,
  server-authoritative randomness at zero added latency.
- **Predict-and-acknowledge motion.** The client moves locally on input
  and streams compact move deltas (accumulating same-direction runs into
  one packet); the server validates (collisions, encounters) and
  broadcasts authoritative positions, which the client applies for
  *other* players. ACK-based flow control keeps the client from
  outrunning the server.
- **Deferred catch-up after cache load.** Get the client into the world
  from cache first; let post-load gains and reconciliations stream in
  asynchronously and finish in the background (§5.6).

**The discipline behind all of these:** predictions must be cheap to roll
back, the server is always the final authority, and reconciliation is
explicit (a queue head, a stream index, a corrected position). A client
that receives a contradictory authoritative update silently corrects — it
never desyncs and never needs to disconnect.

---

## 9. Persistence — delta in RAM, final-state to the store

Store I/O blocks execution and destroys throughput if mismanaged.

- **Delta-state in memory, final-state to disk.** High-frequency
  mutations (position, XP, inventory churn) live in RAM; the store sees
  only periodic snapshots and significant events (logout, meaningful
  change), never every increment. Cuts store load by orders of magnitude.
- **Decoupled, non-blocking I/O.** A save never blocks the simulation or
  network loop (async DB, §5.2).
- **Separate stores by lifecycle and scaling axis.** Split persistence by
  how data lives and scales: accounts (auth, rarely touched), shared
  cross-instance identity (the high-value character/item data), and
  per-instance ephemeral state (position, local progress — wipeable on
  restart without data loss). Many service instances then share the
  identity store while keeping independent ephemeral stores — horizontal
  scale without distributed transactions.
- **No joins; fetch related rows in separate queries.** Keep each query
  trivial and independently cacheable/prepared; assemble in code.

---

## 10. Scaling out: cluster topology (only after §2 is exhausted)

When one core is genuinely saturated, split by *role*, not by sharding a
monolith. Each role is a small single-threaded process of its own.

- **Role separation.** A **login** server (authenticate, hand out a
  token, list game servers), a **master/coordinator** (track which
  character is active where, hold the locks), a **gateway** (absorb raw
  untrusted connections, rate-limit, proxy clean traffic inward), and the
  **game** servers (pure simulation, see only pre-vetted, token-bearing
  traffic). Concerns that scale differently and fail differently are
  isolated.
- **Token-based cascading auth.** Credentials are validated *once*, by
  login; it issues a random, opaque, **short-lived, single-use** token.
  The game server validates that token against an in-memory list — no
  password table, no crypto, no DB round-trip on the join path. Auth cost
  lands on the low-traffic role, not the hot one. (Generalises to any
  capability/bearer-token handoff between a trusted issuer and a
  performance-critical resource server.)
- **One coordinator owns the exclusive lock.** A single source-of-truth
  map `entity → lock-state` prevents the same character being played
  twice. States are **Locked / RecentlyUnlocked (grace window) /
  Unlocked**; a background **purge timer** expires stale grace locks.
  This buys correct single-ownership and crash-reconnect grace **without
  distributed consensus** — one small authority instead of a quorum.
- **Trusted/untrusted boundary at the edge.** The gateway is a thin proxy
  with no game logic, so it can be scaled out cheaply and is the only
  thing exposed to floods. Per-client, per-packet-class **circular-buffer
  rate limiting** (a fixed ring of time buckets rotated by a 1 s timer,
  separate budgets for move/chat/other) caps abuse in O(1) memory with
  no per-request allocation; over-budget connections are dropped before
  the game server ever sees them.
- **Flood protection is built into the protocol parser, not bolted on as
  an external WAF.** Because the gateway already parses the protocol to
  route it, it rate-limits *in that same pass* — so there is no separate,
  heavy, protocol-aware appliance (WAF / scrubbing proxy) that would have
  to **understand and re-parse the exact same protocol a second time**.
  One parse, integrated limits: cheaper, simpler to deploy (no extra
  box), and impossible to get out of sync with the protocol it guards.
  The flip side of this principle is that the protocol must be **safe to
  parse before trust is established**: the packet-size table makes
  pre-login opcodes *fixed-size* (or blocked), so an attacker cannot send
  a 4 GB length header and exhaust bandwidth/RAM before authenticating.
  Validate cheaply and structurally at the front door; don't allocate on
  an untrusted length.
- **Persistent, multiplexed inter-server links.** Role-to-role traffic
  rides **one long-lived connection per pair**, multiplexing many clients'
  logical flows over it via query-ids and a small state machine — no TCP
  handshake per request, auto-reconnect on loss. The §5.2 query-id
  correlation, reused between servers.

---

## 11. One interface, many backends — swap the implementation, not the code

A single abstract interface with implementations chosen at **build/link
time** (not a runtime branch) lets one source serve testing, embedded,
and production targets at zero hot-path cost.

- **Pluggable storage backend.** `DatabaseBase` is an abstract interface;
  MySQL / PostgreSQL / SQLite / flat-file / in-RAM / black-hole are
  implementations picked by compile define. Production gets a real DB;
  **tests** get the in-RAM backend (no disk, run in parallel);
  **benchmarks** get black-hole (measure logic without store contention);
  **embedded** gets flat-file or RAM. Same call sites throughout.
- **One serializer, many sinks.** The same serialize/parse logic drives
  *any* buffer that implements the read/write interface: a disk file, a
  network socket, a RAM blob — or, for a **no-filesystem
  microcontroller**, a sink that emits the content as compile-time `const`
  arrays placed in flash and read in-place (execute-in-place, zero extra
  RAM, strings point into flash). "Same serializer, different sink" turns
  a filesystem dependency into a build-time code-gen step.
- **Swappable socket, chosen at link.** The protocol talks to an abstract
  socket (`read/writeToSocket`); a real TCP/TLS/WebSocket socket and an
  *in-process fake* socket are different subclasses selected at build —
  the fake lets a **solo/embedded client run an in-process server over
  the identical protocol** with no network, and gives tests a
  deterministic transport. Dispatch is virtual, resolved once per
  connection, never an `if(test)` on the data path.

---

## 12. Adverse-network & open-mirror distribution

A server must be viable on a residential connection in an under-served
region, and content distribution must be cheap and open.

- **Offload static distribution to the edge.** Serve maps, assets and
  client updates from CDNs/mirrors; the core server spends ~100 % of its
  (possibly tiny) uplink on dynamic state, not file serving.
- **Content-addressed integrity over untrusted transport.** The
  authoritative server publishes a strong hash of the content; the client
  fetches files from *any* mirror and verifies the assembled result
  against that hash. Because integrity lives at the **application layer,
  not the transport**, **anyone can host a plain HTTP (not HTTPS) mirror**
  — a tampered or corrupt mirror is rejected regardless of transport.
  This drops the barrier to community mirroring (no certificates, any
  cheap host) without trusting the mirror. Identity = content, not
  location.
- **Layered, independently-versioned content.** Split distribution into
  base / per-realm / per-shard layers, each hashed and fetched in order.
  Clients pull the big shared base once and then only the small deltas
  for their realm/shard; common parts are deduplicated and factored out,
  so cross-file commonality is stored once rather than re-shipped per
  file. A partial-sync failure isolates to its layer.
- **Cache what is expensive and stable — like the content checksum.**
  Caching pays only when *recompute-cost × hit-rate* exceeds the cache's
  own overhead. The content hash is the textbook case: costly to compute,
  changes rarely, consulted constantly (every sync, every mirror
  validation) — so cache it (e.g. keyed off file mtime so a stat replaces
  a re-read). Conversely, avoid "cache hopping" where checking/updating
  the cache costs more than recomputing; a cache that can't justify
  itself is stripped.

---

## 13. Portability is a constraint, not an afterthought

One source tree, every target from a 66 MHz i486 to amd64, and on to DOS
and microcontrollers.

- **Endianness and alignment are handled once, centrally** (§5.1) so no
  call site ever casts a raw pointer to a multi-byte type.
- **Conditional compilation as feature/cost control.** Orthogonal build
  defines (server role, DB backend, no-compression, cache format,
  hardened checks, no-XML) **compile whole subsystems out** for a given
  target — a minimal login binary doesn't *contain* the compression or
  cache code it never runs. Features are removed, not branched around.
- **Hand-roll the hot-path primitives the stdlib does expensively.**
  Locale-aware/regex-based conversions (e.g. string→int validated by a
  regex) can dominate a parse-heavy load; a single linear-scan validator
  matching the same inputs is two orders of magnitude faster, smaller in
  code (less i-cache pressure), exception-free, and deterministic on
  minimal-rootfs targets. Measure, then replace the specific offenders —
  don't reinvent the whole library.
- **Address by stable name, not positional index.** When consuming an
  external/authored format (map layers, config sections), resolve by
  *name* and rebuild your internal order, tolerating reordering,
  grouping, and aliases. Robust against editor/tool changes; the on-disk
  order is never load-bearing.

---

## 14. Zero cost when off

Every measurement, debug, or optional-feature hook must compile to
*nothing* in the shipping build for the constrained targets.

- Prefer a **non-virtual production class** with a benchmark/instrumented
  subclass overriding hooks, so no v-table enters the hot path; otherwise
  gate instrumentation behind a build define that vanishes when off.
- **Never** an `if (g_enabled) {…}` runtime check on a hot path — the
  branch alone is a regression on a 66 MHz target.
- **SIMD: generic stays generic.** Never compile architecture-specific
  intrinsics into the generic build (it's what ships when the CPU lacks
  the feature). Detect features once at startup and dispatch; the generic
  implementation is the fallback and pays nothing. A SIMD tier added for
  one architecture says nothing about the others — each is its own
  measured path.
