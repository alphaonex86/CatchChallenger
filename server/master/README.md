# catchchallenger-server-master

The **master** is the cluster's coordination point. Login servers and
game-servers connect to it; clients never do. Without a master the
cluster cannot operate — there is no fallback.

Build target: `catchchallenger-server-master` (this directory's
`CMakeLists.txt`, one binary). Mandatory defines:

* `CATCHCHALLENGER_CLASS_MASTER`
* `CATCHCHALLENGER_SERVER_NO_COMPRESSION`
* `CATCHCHALLENGER_DB_PREPAREDSTATEMENT`
* exactly one of `CATCHCHALLENGER_DB_POSTGRESQL` / `CATCHCHALLENGER_DB_MYSQL`
* mandatory io_uring backend (per root `server/CLAUDE.md`)

## Roles

The master holds three pieces of state nobody else has:

1. **The active game-server roster.** Each gsa (`server/game-server-alone`)
   opens a long-lived TCP session to the master, authenticates with a
   shared `<token>`, and advertises `(host, port, metaData,
   logicalGroupIndex, currentPlayer, maxPlayer)`. The master assigns
   a 32-bit `uniqueKey` per gsa, regenerates on collision, and broadcasts
   the resulting roster to every connected login. See
   `CharactersGroup::addGameServerUniqueKey` + `sendServerChange`.

2. **The CharactersGroup directory and the per-character lock.**
   `CharactersGroup` is a *cluster-shared scope* for characters: each
   gsa attaches to a named group, each character belongs to exactly
   one group, and master mediates "which character lives on which
   gsa" via:

   * `lockTheCharacter(characterId)` — called when a login forwards a
     successful `selectCharacter`; pins the character to a specific
     gsa.
   * `unlockTheCharacter(characterId)` — called when the owning gsa
     reports the character disconnected.
   * `waitBeforeReconnect(characterId)` — short cooldown after a
     normal disconnect so the same character can't double-bind to
     two gsa instances during a flap.
   * `purgeTheLockedAccount()` — background sweep that evicts locks
     whose age has exceeded `maxLockAge`.

   The lock answers the *sticky routing* question. When a client
   logs in twice, the master replies with the gsa that already holds
   the character (or assigns one if free), so a reconnect lands on
   the same world state without manual re-selection. See
   `EventLoopClientLoginMaster::selectCharacter` for the request flow
   and `CharactersGroup::characterIsLocked` for the three states
   `{Unlocked, Locked, RecentlyUnlocked}`.

3. **The shared datapack hash + mirror URL.** Master loads the
   datapack at boot (`BaseServerMasterLoadDictionary`) and forwards
   `(datapackHashBase, datapackHashMain, httpDatapackMirror)` to
   every login on connect. Logins re-broadcast the mirror URL to
   their clients in the `loginGood` packet. Master does *not* serve
   the datapack itself — that's the production HTTP mirror's job
   (or, in `testingcluster.py`, the ephemeral nginx on port 18231).

## Protocol surface

Master speaks two distinct protocols on the same listening port:

* **`PROTOCOL_HEADER_MASTER_LOGIN`** — what login servers identify as.
  Logins authenticate with a shared `<token>` then exchange:
  forward-login, forward-character-select, forward-character-add,
  ping, server-roster updates, current-player counts.
* **`PROTOCOL_HEADER_MASTER_GAMESERVER`** — what gsa instances identify
  as. Same token. gsa registers itself, sends periodic ping, reports
  character lock/unlock, and consumes "incoming client" tokens that
  the master minted for a `selectCharacter` it forwarded to a login.

The header byte that precedes a request determines which dispatcher
parses the rest; mismatched header → `protocolReplyProtocolNotSupported`
and disconnect. Magic numbers are defined in
`general/base/ProtocolVersion.hpp`.

## Configuration (`master.xml`)

Same `TinyXMLSettings` reader as every other server-type. **Keys are
master-specific**, no overlap with login/gsa/game-server-cli:

| Key                           | Default | Meaning                                                                 |
|-------------------------------|---------|-------------------------------------------------------------------------|
| `<ip>`                        | `*`     | bind address                                                            |
| `<port>`                      | —       | listen port (mandatory)                                                 |
| `<token>`                     | —       | shared auth secret; logins+gsa must carry the same value                |
| `<compression>` / `<compressionLevel>` | `none` / 6 | wire compression for the master↔{login,gsa} link                  |
| `<automatic_account_creation>` | `false` | propagated to logins via CommonSettingsCommon                          |
| `<character_delete_time>`     | `604800`| seconds before a soft-deleted character is gone for good                |
| `<min_character>` / `<max_character>` | `0` / `3` | per-account character bounds, propagated cluster-wide              |
| `<max_pseudo_size>`           | `20`    | character-name byte cap                                                 |
| `<maxPlayerMonsters>` / `<maxWarehousePlayerMonsters>` | `8` / `30` | inventory bounds, cluster-wide                          |
| `<maxLockAge>`                | `5`     | seconds a character lock persists after disconnect (sticky-reconnect)   |
| `<purgeLockPeriod>`           | `180`   | seconds between background lock-sweeps                                  |
| `<gameserver><charactersGroup_N>` | —    | per-CharactersGroup db credentials (loaded via `loadCharactersGroup`)   |

Production defaults err on the side of caution; the testing rig
(`test/testingcluster.py`) lowers `maxLockAge` and `purgeLockPeriod`
to 1 s / 2 s so the sticky-routing assertion doesn't have to wait
out a 3-minute cooldown.

The full key list lives in `EventLoopServerLoginMaster::checkSettingsFile`
and `EventLoopServerLoginMaster::loadCharactersGroup`.

## Background timers

`PurgeTheLockedAccount`, `CheckTimeoutGameServer`, `AutomaticPingSend`
— each owns one timerfd, all wired into the same io_uring/epoll loop
as the client sockets. They run on cadences:

* `PurgeTheLockedAccount` — every `purgeLockPeriod` seconds.
* `CheckTimeoutGameServer` — every `CharactersGroup::checkTimeoutGameServerMSecond`
  ms; declares a gsa dead after `CharactersGroup::gameserverTimeoutms` ms
  of no ping reply.
* `AutomaticPingSend` — every `CharactersGroup::pingMSecond` ms;
  triggers the master→gsa keepalive ping.

## Database

Master needs read+write access to the **common** DB (account, character) on a per-CharactersGroup basis. Each `<gameserver><charactersGroup_N>`
section in `master.xml` carries its own `(type, host, login, pass, db)`
because different groups may live in different physical DBs. Master
loads `maxAccountId` / `maxCharacterId` / `maxMonsterId` at boot
(`load_account_max_id` etc.) so new IDs are minted without round-tripping
to the DB on every create.

Master never touches the per-server data (`character_forserver`,
`character_bymap`, `factory`) — that belongs to the gsa.

## Lifecycle

1. Load config + datapack.
2. Open DB connections, prime per-character-group max-id counters.
3. Bind listening socket, set up timerfds.
4. Accept logins + gsa connections.
5. Track + forward forever; broadcast roster changes; expire dead
   gsa instances; release character locks on schedule.

There is no graceful "drain" mode. SIGTERM tears the master down;
logins detect the link drop and try to reconnect; clients see their
own login link RDHUP and fall back to "server unreachable". Cluster
operators are expected to roll the master with overlap (start a new
master, point logins/gsa at it via DNS, then stop the old) to avoid
a visible outage.

## Source map

| File                                         | Owner                                                                    |
|----------------------------------------------|--------------------------------------------------------------------------|
| `main-unix-login-master.cpp`                 | argv parsing, settings, event-loop dispatch                              |
| `EventLoopServerLoginMaster.{cpp,hpp}`       | listening side: bind, accept, settings, datapack load                    |
| `EventLoopClientLoginMaster.{cpp,hpp}`       | per-connection state for both login and gsa peers                        |
| `EventLoopClientLoginMasterProtocolParsing.cpp` | wire-format decoders for the master protocol                          |
| `CharactersGroup.{cpp,hpp}`                  | cluster scope: gsa roster + character locks + per-group DB              |
| `PurgeTheLockedAccount.{cpp,hpp}`            | background sweep for expired locks                                       |
| `CheckTimeoutGameServer.{cpp,hpp}`           | declares an unreachable gsa dead                                         |
| `AutomaticPingSend.{cpp,hpp}`                | master→gsa keepalive cadence                                             |
| `PlayerUpdaterToLogin.{cpp,hpp}`             | rate-limited per-player count broadcasts to logins                       |
| `TimeRangeEvent.{cpp,hpp}`                   | day/night cycle scheduler propagated to gsa                              |

## Operational notes

* **One master per cluster.** No leader-election, no replication. If
  redundancy is wanted, run a second cluster behind a DNS-level
  failover; do not run two masters against the same DB.
* **Master has no `correctly bind:` race risk like the game-cli.** Its
  preload is purely DB + datapack; the long uint8_t-overflow story
  in `EventLoopClientList::EventLoopClientList()` is gsa-specific.
* **DB outages stall the cluster.** If the master loses its DB
  connection mid-run, it `abort()`s and returns `EXIT_FAILURE`; the
  cluster has no in-process retry — operators wrap the binary in a
  systemd/podman supervisor that re-execs on crash.
