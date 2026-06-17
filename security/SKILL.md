---
name: securing-a-low-footprint-mmo-server
description: >
  Hard-won, project-specific doctrine for keeping a small, fast,
  long-lived networked game server safe against a hostile internet —
  remote crash/DoS, protocol abuse, player-on-player flooding,
  credential brute-force, and information leaks — WITHOUT a big security
  team and WITHOUT bloating the binary or the running cost. The premise
  is the opposite of "add a WAF in front": security is written INTO the
  engine, in-tree, in the hot path, and is kept correct by the test
  suite as the code evolves. Distilled from CatchChallenger, an MMO
  engine that must stay safe while running on hardware as weak as a
  66 MHz i486 and over links as bad as I2P. Use when adding a remote
  packet handler, touching the protocol parser, validating client
  input, designing a rate limit, handling passwords/tokens, or judging
  whether a finding is a real remote vulnerability.

  NOTE: the embedded-silicon reverse-engineering & bring-up playbook
  that used to live here has MOVED to reverse-engineering/SKILL.md.
  This file is now purely about the SECURITY of the project. See also
  the sibling playbooks: test/SKILL.md (how we prove robustness) and
  benchmark/SKILL.md (the performance design that IS our DoS defence).
---

# Securing a low-footprint, long-lived MMO server

Hard-won doctrine, not a generic security checklist. The threat model is
concrete: **the only attacker that matters is bytes on a TCP socket from
a connected or connecting game client.** Everything below exists to make
those bytes unable to crash, hang, corrupt, drain, or out-trick the
server — on weak hardware, over bad links, for years, with a tiny team.

The invariants the whole design serves:

- **No remote packet can crash or hang the server.** Not a malformed
  one, not a hostile one, not a 100×-rate flood. A crash is the worst
  bug class here — one packet kills the process for *every* player on it.
- **Security lives in-tree and in the hot path**, versioned with the
  code, exercised by the test suite — never a bolted-on external box
  that drifts out of sync with the protocol.
- **Defence costs near-zero at runtime.** The cheapest DoS defence is a
  server that stays cheap under load; performance *is* a security
  property here (see benchmark/SKILL.md).
- **Every byte from the client is untrusted and re-derived server-side.**
  The client is an attacker's puppet; the server is the sole authority on
  what is true.
- **The hardest surface — the binary/protocol layer — gets the most
  scrutiny.** That is where memory corruption hides and where a textbook
  "missing check" is usually a false positive (or a real RCE).

---

## 1. The attack surface is the parser, and only the parser

**1.1 Name the one entry point.** Remote untrusted input enters as raw
socket bytes and is turned into actions in exactly one place: the
protocol parser (`general/base/ProtocolParsing*`, dispatched into
`server/base/ClientNetworkRead*.cpp`). The socket plumbing around it
(EventLoop, io_uring, UnixSocket, SocketUtil) is **not** the attack
surface — it moves bytes, it does not interpret them. When you triage a
finding, the first question is "is this reachable from the byte stream a
client controls?" If it sits in operator-config handling, in the admin
stdin console, or in code the binary under attack never instantiates,
it is a hardening note, not a remote vulnerability. This distinction is
recorded per-file in `security/SECURITY-IA.md` — read it before
re-litigating a "bug" someone already triaged as a false positive.

**1.2 The parser invariant kills most "missing size check" findings.**
`ProtocolParsingBase::parseData()` reassembles a packet until **all
declared bytes have arrived** before it dispatches to a handler. So a
per-packet handler is *guaranteed* a buffer of at least `size` valid
bytes. A handler that reads a FIXED number of bytes (`memcmp 5`,
`memcpy 64`) ≤ its declared `packetFixedSize` is safe **even if it
appears to "ignore size"** — verified empirically with short pre-auth
packets under valgrind: 0 errors. A REAL out-of-bounds read needs a
**dynamic-size** handler that trusts an *inner* length/count field and
reads past `size`. Look there, not at the fixed-size handlers. This
single invariant retires the bulk of automated "CWE-125" noise.

**1.3 Fixed-size-before-login is itself a defence.** ~26 packet types use
the `0xFE` marker for variable size; everything reachable *before* the
client authenticates is fixed-size on purpose — so an unauthenticated
peer "can't send a 4 GB size packet and saturate the bandwidth"
(comment in `ProtocolParsingGeneral.cpp`). Dynamic packets are
hard-capped in `parseDataSize()` to `CATCHCHALLENGER_BIGBUFFERSIZE-8`,
with tighter per-mode ceilings (master 4 KB, mirror 64 KB). **Never add
a variable-length pre-auth packet.** If you must, cap the inner length
against the remaining buffer *before* using it, and add a hostile-input
test for it.

**1.4 Strict-alignment safety is a correctness AND a portability bug
class.** The parser reads multi-byte fields with `memcpy`/`loadLe32()`
wrappers, never a direct cast-and-dereference — a misaligned header on a
strict-alignment arch (MIPS, some ARM) would SIGBUS, which is a remote
crash. Same for endianness: store/load little-endian explicitly. A
packet handler that compiles on x86 but faults on a 66 MHz target is
still a shipped remote-DoS.

---

## 2. Layered rate limiting, integrated into the read path

DoS resilience is not a front-end appliance; it is a few bytes of state
per client, checked inside the same function that reads the packet.

**2.1 The sliding-window counter (`server/base/DdosBuffer.hpp`).** A tiny
fixed-array circular buffer (`DdosBuffer<uint8_t,3>`) holds the last N
time-windows of a counter; `total()` is the windowed sum,
`incrementLastValue()` counts an event, `flush()` (driven by a timer)
rolls the window. Three live per client, by *category*:
`movePacketKick`, `chatPacketKick`, `otherPacketKick` (`Client.hpp`).
Each category has its own ceiling
(`serverSettings.ddos.kickLimit{Move,Chat,Other}`); exceed it and the
client is disconnected with a logged reason (`ClientNetworkReadMessage.cpp`).
Separate categories matter: a player legitimately sends many moves but
few clan-creates, so one budget would either be too loose for moves or
too tight for the rest.

**2.2 Saturating, branch-light, allocation-free.** The counter saturates
at the type max (`uint8_t`→255) instead of overflowing, never allocates,
and is ~7 bytes of state. That is the whole point: the *defence* must be
cheaper than the attack, or you have built an amplifier. Do not "improve"
it into something with a heap allocation or a per-event syscall.

**2.3 Drop-don't-kick for chat; kick for protocol abuse.** Flooding the
*chat* channels is throttled by *dropping* messages once a windowed
counter passes `ddos.dropGlobalChatMessage{General,LocalClan,Private}`
(`ClientBroadCast.cpp`, `ClientLocalBroadcast.cpp`) — a chatty player is
silenced, not disconnected, because chat flood is annoyance, not
exploitation. Protocol-rate abuse (too many packets/moves) *kicks*.
Match the response to the harm: silently absorb annoyance, disconnect
abuse, never crash on either (see test/ doctrine on "safe-absorb").

**2.4 Don't rate-limit the server's own replies.** There is a deliberately
disabled block in `ClientNetworkRead.cpp`: "you are into the server, if
server emits too many queries, fix the emit, not the reply." Rate limits
are for *untrusted* input only; throttling trusted internal traffic just
hides a real bug.

---

## 3. Validate every action server-side — except movement speed

The server re-derives the truth; the client is never believed. For each
client action ask the four questions and answer them from server state:
**correct position? on the correct map? allowed to do this action? owns
the item/monster?**

**3.1 Movement.** Direction is range-checked; each single step is
validated against the collision map (`flat_simplified_map`, value <200 =
walkable, 250–253 = one-way ledges checked *per direction*, 254 =
blocked); map borders are enforced; "move vs look" coherence with the
previous action is enforced; teleporter use re-checks the gating
condition (clan ownership, quest done, item held, bot beaten). See
`MoveOnTheMap.{hpp,cpp}`, `MapBasicMove.cpp`, `LocalClientHandlerMove.cpp`.

**3.2 The deliberate non-check: movement SPEED / step count.** The move
packet's `stepCount` (uint8) has **no upper rate cap** and grouped moves
are accepted. This is intentional, not an oversight: over I2P or a bad
link the server legitimately receives many moves bunched into one batch,
so rejecting "too fast" movement would punish honest high-latency
players. The *positions* are all still validated step-by-step — a client
can move many tiles in one packet, but every tile must be individually
legal. **If you are tempted to add a speed check, don't** — re-read this
paragraph; the security comes from per-tile legality, not from timing.

**3.3 Actions and ownership.** Inventory/monster/trade handlers all
bounds-check the position into the player's own vectors and verify
possession before acting: monster position `< monsters.size()`, item
present in inventory and quantity sufficient, can't trade the last
monster or trade while in fight, fight actions check the monster is
alive and on the active team (`LocalClientHandler*Trade/Object/FightManage.cpp`,
`CommonFightEngine.cpp`). The pattern is always: validate index → verify
ownership/state → act. Never act on a client-supplied index without both.

**3.4 Invalid input: absorb safely or disconnect — never corrupt, never
crash.** A handler may safely drop nonsense without kicking; the
kick-contract is "no crash, no state corruption," not "punish every bad
byte." Reserve disconnect for malformed framing / protocol-rate abuse.

---

## 4. Coherency at load time, where speed does not matter

There are two regimes. The **hot path** (per-tick movement, visibility)
is cheap and trusts already-validated in-RAM state. The **load path**
(character select, datapack, map load) is rare, so it pays for *full*
coherency checking — this is where you spend cycles freely.

- **Every index from the DB is bounds-checked on the way in.** A map id
  from the database is looked up through
  `DictionaryServer::dictionary_map_database_to_internal`, checked for
  the not-found sentinel, and validated against
  `flat_map_list.size()` before use; rescue x/y are clamped to the
  loaded map's real dimensions; orientation is range-checked with a safe
  default (`ClientHeavyLoadSelectChar.cpp`). The principle: **a stored
  value is as untrusted as a wire value** — DB corruption or an older
  schema must not produce an out-of-bounds access at runtime.
- **The map index is never allowed out of bounds.** This is the
  load-time invariant that makes the hot path able to skip the check.

---

## 5. HARDENED mode: prove the invariants, then ship lean

`CATCHCHALLENGER_HARDENED` (CMake option, default OFF in production) turns
the engine's implicit invariants into **explicit `abort()` checks**:
null-param guards, packet-type tags, vector-size ceilings (e.g. buffs
≤255), post-move bounds asserts, and — in `DdosBuffer` — clock-drift and
"forgot-to-flush >300 s" detection that returns 0 instead of a stale
count. The doctrine around it:

- **Hardened is ALWAYS ON in the test suite** (test/SKILL.md). Any abort
  under test is a real invariant violation that **must be fixed**, not
  silenced — it means the production build, which would *not* abort, was
  about to do something undefined.
- **Production ships hardened OFF for speed**, relying on the test suite
  having already driven every abort to never-fires. So the contract is:
  the *same* malformed packet must (a) `abort()` loudly under hardened/test
  and (b) be gracefully disconnected, never crash, under production. Both
  flavours are tested — see `test/testingclustersecurity.py`, which fires
  a battery of malformed/boundary/hostile packets at every client-facing
  port with hardened ON *and* OFF, respawning any server that dies so the
  whole battery runs, then re-drives a real client to prove survival.
- Hardened also enables **stronger internal data-coherency** and the
  protocol "mirror" re-parse — it is the mode that distrusts our own
  state, not just the client's.

---

## 6. Memory discipline is the core security work

On a C/C++ server, the security bug that matters most is memory
corruption from a remote packet. The test/ doctrine (see test/SKILL.md)
is therefore a security tool, not just a QA tool:

- **Run under strict memory control whenever you touch C/C++.** Every
  `testing*.py` and `all.sh` wires in clang sanitizers
  (`asan`/`lsan`/`msan`, `-fno-sanitize-recover=all`, `halt_on_error=1`)
  and gcc+valgrind (memcheck). A handler that passes functionally but
  trips ASAN/valgrind on a crafted packet is a security failure.
- **Hunt infinite loops and hangs as hard as crashes.** A packet that
  wedges the single-threaded event loop is a full DoS for every player.
  (Real example fixed in-tree: `UnixSocketClientFinal` OOB read + infinite
  loop.) Fuzz toward non-termination, not just toward segfaults.
- **Prefer RAII / `std::vector` over raw `new[]`/`malloc`.** Mismatched
  free and leaks are the recurring memory-bug shapes here; the project
  has repeatedly converted raw buffers to `std::vector<char>` (`.data()`).
- **Don't instrument a binary mid-test.** Adding `[TRACE]` prints to a
  binary under a running `testing*.py` changes timing/stdout/back-pressure
  and invalidates the verdict. Kill the test → add traces → rebuild →
  re-run from scratch → read → strip → rebuild → confirm. (gdb for naked
  reproducible crashes; never gdb *with* ASAN/valgrind.)

---

## 7. Performance is a DoS defence (keep the cost floor low)

The cheapest way to survive a flood is to make each packet cheap, so the
server saturates into a flat plateau instead of a death spiral
(benchmark/SKILL.md). Security-relevant consequences:

- **Decode in-band, zero-copy.** Packets are parsed straight out of the
  recv buffer when no header continuation is pending ("parse directly out
  of `buf` — zero copy"), into a single static read buffer with no
  per-connection allocation. io_uring `recv_multishot` + provided buffer
  rings push this further (kernel→userspace zero-copy); datapack delivery
  uses `sendfile(2)`/splice chains so big transfers cost almost no CPU.
  Single-threaded epoll/io_uring means **no locks and no cross-connection
  aliasing** — and therefore the concurrency-bug class (TOCTOU, data
  races between socket callbacks) simply does not exist here; do not flag
  it.
- **Low cost floor = headroom against floods**, and it is *why* we can run
  exposed on tiny/cheap hardware that is trivial to sandbox tightly (§9).

---

## 8. Credentials, hashing, and not leaking internals

- **Two hash families, by purpose.** `xxhash` (XXH32) for **fast,
  non-security** integrity (datapack file checks, chunking). **blake3**
  (`general/base/CatchChallenger_Hash.*`, 256-bit) for **every final /
  security hash** — login verification, server-to-server cluster auth,
  client→game-server tokens. blake3 is the safe, quantum-compute-proof
  choice; xxhash must **never** guard a security boundary. If you add a
  hash, ask "can an attacker benefit from forging this?" — yes ⇒ blake3.
- **Passwords: salted, shared-secret, never the raw secret on the wire.**
  The stored secret is combined with a per-session random nonce (16-byte
  token from `/dev/urandom`) and blake3'd; the client proves knowledge
  via a `memcmp` of digests, so the DB secret itself never crosses the
  network and a captured login is not replayable.
- **Long-session premise ⇒ strict login-try limits are affordable.**
  Players stay connected for hours, so logins are rare per user; the
  server caps simultaneous un-authenticated connection slots
  (`CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION`, default 50) and
  evicts the oldest pending auth when full. Tight pre-auth budgets cost
  honest users nothing and starve brute-force/connection-flood attempts.
- **Never leak internal DB ids into the protocol.** Runtime uses
  *internal* indices; the DB id is mapped through `DictionaryServer`
  only at serialization boundaries (`Client.cpp` save/load). The client
  sees internal indices, never raw row ids — so it cannot enumerate or
  probe the database layout from the wire. When you add a field to a
  packet, check it is not an internal/DB id in disguise.

---

## 9. The server is *only* a server — keep it that way

- **No useless surface.** The server does one thing; it has no admin web
  panel, no scripting engine, no plugin loader bolted into the hot binary.
  Every feature is attack surface — the smallest server is the safest.
- **Minimal dependencies, on purpose.** A dependency is not added for a
  1% gain: fewer libs = smaller supply-chain surface and less to audit
  and maintain. Where a lib is used (blake3, xxhash, tinyxml2, zstd,
  zlib) it is **system-lib-first with a vendored fallback** — the cross/
  fleet sysroot is a copy of the real target, so its `.so` is what the
  device runs and is preferred; the vendored copy compiles in only when
  `find_library()` fails. Keep vendored copies at the latest upstream and
  **fix bugs at our call site, not in the vendor fork** (a patched fork
  breaks the next re-import).
- **`vector` features are compile-time-removable.** Datapack-send extras
  (compression, mirror streaming) are exactly the heavy, less-trodden
  paths most likely to hide a bug, so they are switchable off:
  `CATCHCHALLENGER_SERVER_NO_COMPRESSION` removes the zstd path entirely
  (and the master binary builds with it off); `compression=none` disables
  it at runtime. Compression is applied **only** to the rare character/
  datapack-load packets, **never** the hot per-tick packets. A removed
  feature is an un-attackable feature — if a deployment doesn't need it,
  compile it out.
- **Minimal needs ⇒ strict sandbox.** Because the server wants almost
  nothing from the OS (a port, a DB socket, a datapack dir, `/dev/urandom`),
  it is trivial to lock down hard — drop privileges, restrict the
  filesystem, seccomp the syscall set. Good sysadmin hygiene is part of
  the security posture, not an afterthought; the lean runtime makes it
  cheap. (Note the local-only hardening items still tracked in
  `SECURITY-IA.md`, e.g. the operator-config `UnixSocketServer` paths —
  out of the remote threat model, but worth fixing for defence-in-depth.)

---

## 10. How to work on this safely (the loop for a future IA/human)

1. **Read `security/SECURITY-IA.md` first.** It is the running audit
   notebook of trust boundaries and already-triaged false positives.
   Don't re-flag what is recorded there; do add new findings to it.
2. **Locate the byte path.** Is your change reachable from a client's
   TCP bytes? If not, it is hardening, not a remote vuln — say so.
3. **Re-derive, don't trust.** Any value from the client (or the DB) is
   untrusted: bounds-check the index, verify ownership/state, then act.
4. **Add the hostile test in the same change.** A new packet handler
   needs a malformed/boundary/flood case in the relevant `testing*.py`;
   run it under sanitizer *and* valgrind, hardened ON. The test count
   never decreases (no commented-out `run_test` lines).
5. **Only keep changes that move a real [FAIL]→[PASS].** No
   "maybe-this-helps" security patches; revert anything that fixes
   nothing.
6. **Match the response to the harm.** Crash → never acceptable.
   Protocol-rate abuse → kick. Annoyance flood → drop. Bad single value
   → absorb or kick, never corrupt.
7. **The bounty docs (`security/EXAMPLE-BOUNTY-BUG-SERVER*.md`) define
   the external scope:** client-facing standalone game server, login,
   gateway, game-server-alone are in scope; master and inter-node links
   are internal-only and out of scope. `security/server.py` is the scan
   tooling; its output is git-ignored.

---

### Cross-references

- **test/SKILL.md** — how we manufacture every hostile condition and
  prove survival (sanitizers, valgrind, fuzzing, cluster security test).
- **benchmark/SKILL.md** — the low-RAM, zero-copy, plateau-not-spiral
  design that makes flood resilience cheap.
- **reverse-engineering/SKILL.md** — (moved from here) the unrelated
  embedded-silicon RE & bring-up playbook.
- **security/SECURITY-IA.md** — the live, per-file audit ledger.
- **security/EXAMPLE-BOUNTY-BUG-SERVER*.md** — external scope & rules.
