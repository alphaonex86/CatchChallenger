# CatchChallenger — Game-Server Bug Bounty (scope & attack-surface guide)

This document is for **independent security researchers**. It tells you **where to
look** and **what we pay for** — it deliberately does **not** describe any specific
vulnerability. The server has been audited and hardened; the goal of this program
is to confirm that, and to reward anyone who finds something we missed.

> A valid report is one **you** discover and prove. We do not list known issues
> here, because there are none we are aware of — if you find one, it is new, and it
> is rewarded. Read the code, build the server, attack it, and show us a reproducer.

https://github.com/alphaonex86/CatchChallenger/

---

## 1. Target

The first in-scope target is the **standalone game server** built from `server/cli/`
(`catchchallenger-server-cli`), the single-threaded event-loop server that accepts
player connections on the **game TCP port**. The attacker input we consider is
**bytes sent over a TCP socket** by a connected (or connecting) **game client** —
against the standalone game server, or against the cluster's client-facing login,
gateway and game-server-alone (see *The cluster servers* below). The master and the
links between cluster nodes are internal-only and out of scope (§2).

Build a local instance to attack — no extra packages beyond a C++ toolchain, CMake
are required:

```sh
cmake -S server/cli -B build/server-cli
cmake --build build/server-cli -j
```

Run it against a datapack and a `server-properties.xml` (automatic_account_creation value="true"; `mainDatapackCode` selects the maincode, `compression=none`
makes packets easy to read on the wire). The file-backed database (`FILE_DB`) build
needs no SQL daemon and is the simplest to stand up.

### The cluster servers

Beyond the standalone server, four more binaries make up the multi-server
(*cluster*) deployment. They speak the SAME wire protocol (§4); each parses a
different slice of it:

| Binary | Source | DB | Untrusted peer | In scope? |
|---|---|---|---|---|
| `catchchallenger-server-login` | `server/login/` | PostgreSQL/MySQL | game client | **yes** — account login / auto-creation, character add / remove / select, then a transparent client↔game proxy |
| `catchchallenger-gateway` | `server/gateway/` | none | game client | **yes** — a thin client↔backend proxy + datapack file-list download/sync |
| `catchchallenger-game-server-alone` | `server/game-server-alone/` | PostgreSQL/MySQL | game client | **yes** (client port) — the same client handlers as `server/cli` |
| `catchchallenger-server-master` | `server/master/` | PostgreSQL/MySQL | login / game-server **nodes** | **no** — internal cluster network only (see §2) |

The **master** is reached only by other cluster nodes over the internal VPS / LAN
network and is never exposed to clients or the internet — so it, and every inter-node
link (including the game-server's `LinkToMaster`), is **out of scope**. The bounty
targets the three **client-facing** servers above.

`login`, `master` and `game-server-alone` build **only** against a real SQL backend
(`-DCATCHCHALLENGER_DB_POSTGRESQL=ON` or `-DCATCHCHALLENGER_DB_MYSQL=ON`) and only
work **together as a cluster**; the `gateway` is DB-less. The simplest way to stand
the whole thing up exactly as we test it — an ephemeral PostgreSQL, the binaries
wired together with a shared master token, and a real `qtcpu800x600` client driven
end-to-end to the map — is `test/testingcluster.py` (cluster) plus
`test/testinggateway.py` (gateway); the per-server robustness cases live in
`test/testingclustersecurity.py`. Read those for the exact ports,
`server-properties.xml`/`login.xml`/`master.xml` keys, the shared token, and the
login → master → game-server hand-off.

```sh
cmake -S server/master -B build/master -DCATCHCHALLENGER_DB_POSTGRESQL=ON
cmake -S server/login  -B build/login  -DCATCHCHALLENGER_DB_POSTGRESQL=ON
cmake -S server/game-server-alone -B build/gsa -DCATCHCHALLENGER_DB_POSTGRESQL=ON
cmake -S server/gateway -B build/gateway        # no DB
cmake --build build/master -j && cmake --build build/login -j && \
  cmake --build build/gsa -j && cmake --build build/gateway -j
```

**Test only against your own instance.** Do not attack the public servers, other
players, or any `*.herman-brule.com` host.

---

## 2. Scope

**In scope** — code reachable from the game TCP port:

* The wire protocol framing and dispatch:
  `general/base/ProtocolParsing*.cpp`, `server/base/ClientNetworkRead*.cpp`.
* The per-packet handlers and the game logic they call:
  `server/base/`, `server/base/ClientEvents/`, `server/crafting/`, `server/fight/`,
  and the shared engine in `general/base/` and `general/fight/`.
* Server state and persistence reachable through those handlers (player inventory,
  cash, monsters, position, quests, clans, trade, shops, factories, plants).
* **Login server** (`server/login/`) — the pre-login handshake, account login &
  auto-creation, and the character add / remove / select handlers
  (`EventLoopClientLoginSlaveProtocolParsing.cpp`), plus the transparent client↔game
  proxy it becomes after select.
* **Gateway** (`server/gateway/`) — the pre-login handshake, the datapack file-list
  download / sync, and the move / chat / select pass-through to the backend
  (`EventLoopClientLoginSlaveProtocolParsing.cpp`, `DatapackDownloader*.cpp`).
* **Game-server-alone** (`server/game-server-alone/`) — its client-facing handlers
  are the same `server/base/ClientNetworkRead*` code as `server/cli` (in scope). Its
  master-link reply parsing (`LinkToMaster*.cpp`) is **out of scope** — see below.

**Out of scope:**

* Vendored third-party libraries (`general/blake3`, `general/hps`,
  `general/libxxhash`, `general/libzstd`, `general/tinyXML2`,
  `client/libqtcatchchallenger/libtiled`, `libogg`/`libopus`/`libopusfile`). Report
  those upstream.
* The client, the GUI admin tool, and the build system.
* **The master server (`server/master/`) and every inter-node link** — login↔master,
  game-server↔master, and the game-server's `LinkToMaster*` reply parsing. They speak
  ONLY on the internal VPS / LAN cluster network: the master is never bound to a public
  interface, never reachable by a client or from the internet, so a remote attacker
  cannot reach it. **Out of scope.**
* Findings that require a malicious or modified **server binary**, **datapack**, or
  **configuration** — the attacker only controls a *client* socket (game / login /
  gateway / game-server). Tampering with another node's binary or config is out of scope.
* Pure volumetric DoS (flooding bandwidth/connections). Algorithmic
  amplification from a *single small packet* (see §3) **is** in scope.
* Anything only reachable with the `CATCHCHALLENGER_HARDENED` build flag on — the
  production server is built with it **off**. Test with the default (off) flags.
* Remote action, to improve performance some action is allowed on local map.

---

## 3. What we reward (vulnerability classes)

We pay for a TCP-reachable defect that, triggered by client bytes, causes any of:

1. **Memory safety** — out-of-bounds read or write, use-after-free, double-free,
   uninitialised read, stack overflow, or a heap-corrupting container/iterator
   misuse. Prove it with a sanitizer (ASan/UBSan) or valgrind trace.
2. **Crash / unhandled exception** — any input that aborts or terminates the
   server process, or an uncaught C++ exception that unwinds out of a handler.
3. **Hang / infinite loop / CPU amplification** — a single bounded packet that
   pins a core or never returns, denying service to other players.
4. **Economy integrity** — creating, duplicating, or destroying items, cash,
   monsters, recipes, plants, or XP without the legitimate cost or ownership;
   any integer overflow/underflow in a price, quantity, count, or index that
   yields free or negative goods.
5. **Ownership / authorization bypass** — acting on, reading, or mutating another
   player's or another character's state; using or selling something you do not
   own; selecting/removing a character you do not own.
6. **World-rule bypass** — moving through collisions or one-way ledges, teleporting
   or reaching a tile the rules forbid.
7. **Persisted-state corruption** — input that leaves the saved character/account
   data wrong (cross-player bleed, malformed blob, phantom values) after the
   connection ends.

A finding that is "the server logged an error and disconnected me" or "the server
ignored my malformed packet and stayed up" is **not** a vulnerability — that is the
intended defence (the server may either kick the offender or safely ignore the
input; both are acceptable). We reward only the outcomes listed above.

---

## 4. The attack surface — zones to probe

Every game packet is `[packetCode:1]( [queryNumber:1] )( [dataSize:4 LE] )[data]`.
`queryNumber` is present for queries/replies (code ≥ 0x80) and the 0x7F reply;
`dataSize` is present only for **dynamic** packets (those whose registered fixed
size is the "not fixed" sentinel in `ProtocolParsingGeneral.cpp` — check that table
before you frame a packet; getting it wrong just stalls the parser). All multi-byte
integers are little-endian. The handler dispatch lives in
`server/base/ClientNetworkReadMessage.cpp` (messages) and
`ClientNetworkReadQuery.cpp` (queries) — read these first; they are the doorway to
every zone below.

Connect, complete the handshake to a selected character, then exercise each zone.
For every packet, vary: the declared size vs. actual bytes, every length/count/index
field at its extremes (0, 1, max, max±1), ids that don't exist or you don't own,
and the **state** you send it from (out of fight, not at a shop, no clan, etc.).

| Zone | Packets / handlers | Where |
|---|---|---|
| Framing & dispatch | size prefix, query numbers, unknown codes, pre-login bytes | `general/base/ProtocolParsingInput*.cpp`, `ClientNetworkRead*.cpp` |
| Movement & map | move, teleport, take object, map visibility | `ClientNetworkReadMessage.cpp`, `ClientEvents/LocalClientHandlerMove.cpp`, `LocalClientHandler.cpp` |
| Chat | local / all / clan / private, text sizes | `ClientNetworkReadMessage.cpp`, `ClientBroadCast*.cpp` |
| Inventory / objects | use, destroy, use-on-monster | `ClientEvents/LocalClientHandlerObject.cpp` |
| Shop & factory | buy, sell, factory buy/sell/list | `ClientEvents/LocalClientHandlerShop.cpp` |
| Crafting & plants | use recipe, plant seed, collect plant | `server/crafting/` |
| Quests | start / cancel / finish / next-step, requirements | `ClientEvents/LocalClientHandlerQuest.cpp` |
| Clan | create / leave / dissolve / invite / accept-refuse | `ClientEvents/LocalClientHandlerClan.cpp` |
| Trade | request / add item & monster / finish / cancel | `ClientEvents/LocalClientHandlerTrade.cpp` |
| Fight | request fight, use skill, change/move monster, learn skill, evolution, escape, heal | `server/fight/`, `general/fight/CommonFightEngine.cpp` |
| Character & datapack | add / select / remove character, datapack file list/sync | `ClientNetworkReadQuery.cpp`, `server/base/ClientLoad/` |

Pay special attention to: arithmetic on attacker-controlled `price × quantity`,
`count`, `stepCount`, and array/bitmap indices derived from 8- or 16-bit id fields;
container access (`.at()`, `operator[]`, `erase`, `front`/`begin`) whose
precondition depends on a packet field; and any handler whose effect depends on the
player's current position or game state.

### Cluster servers

The cluster speaks the same framing; only the dispatch entry differs per binary. Boot
the cluster (`test/testingcluster.py` / `test/testingclustersecurity.py`) so the
login → master → game-server hand-off works, then probe each **client-facing**
listener below. (The master and the inter-node links take bytes only from other
cluster nodes on the internal network — out of scope, §2 — so they are not listed.)

| Server | Dispatch entry | Reachable handlers (by state) | Where |
|---|---|---|---|
| login | `parseInputBeforeLogin` / `parseQuery` | `0xA0` handshake; `0xA8` login; `0xA9` create-account; `0xAD` stat; then (Logged) `0xAA` add-char, `0xAB` remove-char, `0xAC` select-char | `server/login/EventLoopClientLoginSlaveProtocolParsing.cpp` |
| gateway | `parseInputBeforeLogin` / `parseMessage` / `parseQuery` | `0xA0` handshake; `0xA1` datapack file-list; `0xAC` reconnect-select; `0x02`/`0x03` move/chat pass-through | `server/gateway/EventLoopClientLoginSlaveProtocolParsing.cpp`, `DatapackDownloader*.cpp` |
| game-server-alone | `ClientNetworkRead*` (same as `server/cli`) | every client handler of the standalone game server | `server/base/` |

Cluster-specific things to push on: the pseudo / login / password length bytes in the
login `0xAA`/`0xA8`/`0xA9` packets; the `charactersGroupIndex` / `profileIndex` /
`skinId` / `monsterGroupId` indices in login `0xAA`/`0xAB`/`0xAC`; and the gateway's
datapack file-list loop (`number_of_file`, per-file `textSize`, `partialHash`), its
`serverReconnectList[charactersGroupIndex][uniqueKey]` lookup, and the
`DatapackDownloader*` file-name / hash reassembly path.

---

## 5. Rules of engagement

* Attack **only** a server instance you run yourself. Never touch production, other
  players, or shared infrastructure.
* No social engineering, no physical attacks, no attacks on the project's hosting,
  email, DNS, or accounts.
* Do not exfiltrate or destroy data you don't own; use throwaway test accounts.
* Give us reasonable time to fix before any public disclosure.

---

## 6. How to submit

Send a private report (contact via the canonical site) containing:

1. **Class** (from §3) and a one-line impact summary.
2. **Exact reproducer**: the raw bytes / packet sequence, the starting state, and a
   script or steps to trigger it against a freshly built `server/cli` instance (or,
   for a cluster finding, the cluster stood up via `test/testingcluster.py`).
3. **Evidence**: the crash backtrace, sanitizer/valgrind output, or before/after
   server state proving the impact.
4. The **commit hash** you tested and your build flags.

Reports that only point at a code line "that looks risky" without a working
reproducer are not eligible. Mark the file:line if you can — but the reproducer is
what earns the reward.

---

## 7. Reward tiers (guidance)

| Severity | Examples | Reward |
|---|---|---|
| Critical | Remote memory corruption with control, server-wide crash from one packet, unrestricted item/cash duplication | top tier |
| High | OOB read/write, reliable crash/hang, cross-player state corruption, ownership bypass | high |
| Medium | Bounded economy abuse, cap bypass, world-rule bypass with limited impact | medium |
| Low / Info | Minor hardening gaps, latent issues not reachable in the production build | none |

Final severity and amount are at the maintainers' discretion, based on real-world
impact and reproducibility. **Duplicate or already-fixed issues are not rewarded** —
which is exactly why this document hands you the map but not the treasure.
