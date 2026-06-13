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

The in-scope target is the **standalone game server** built from `server/cli/`
(`catchchallenger-server-cli`), the single-threaded event-loop server that accepts
player connections on the **game TCP port**. The only attacker input we consider is
**bytes sent over that TCP socket** by a connected (or connecting) client.

Build a local instance to attack — no extra packages beyond a C++ toolchain, CMake
are required:

```sh
cmake -S server/cli -B build/server-cli
cmake --build build/server-cli -j
```

Run it against a datapack and a `server-properties.xml` (automatic_account_creation value="true"; `mainDatapackCode` selects the maincode, `compression=none`
makes packets easy to read on the wire). The file-backed database (`FILE_DB`) build
needs no SQL daemon and is the simplest to stand up.

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

**Out of scope:**

* Vendored third-party libraries (`general/blake3`, `general/hps`,
  `general/libxxhash`, `general/libzstd`, `general/tinyXML2`,
  `client/libqtcatchchallenger/libtiled`, `libogg`/`libopus`/`libopusfile`). Report
  those upstream.
* The client, the GUI admin tool, and the build system.
* The login / master / gateway cluster servers (separate program; may run its own
  bounty later).
* Findings that require a malicious or modified **server**, datapack, or
  configuration — the attacker only controls a client socket.
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
   script or steps to trigger it against a freshly built `server/cli` instance.
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
