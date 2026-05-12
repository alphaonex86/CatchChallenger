# catchchallenger-server-login

The **login** is the cluster's public-facing front door. Game clients
connect to it; it never serves gameplay itself. A cluster typically
runs many logins behind DNS round-robin / load-balancer; each login
maintains exactly one persistent link to the master.

Build target: `catchchallenger-server-login` (this directory's
`CMakeLists.txt`, one binary). Mandatory defines:

* `CATCHCHALLENGER_CLASS_LOGIN`
* `CATCHCHALLENGER_DB_PREPAREDSTATEMENT`
* `CATCHCHALLENGER_DYNAMIC_MAP_LIST`
* exactly one of `CATCHCHALLENGER_DB_POSTGRESQL` / `CATCHCHALLENGER_DB_MYSQL`
* mandatory io_uring backend (per root `server/CLAUDE.md`)

## Roles

1. **Account + character authentication.** Login owns the *login
   database* (`db-login`: accounts, password hashes, ban list) and
   the *common database read-side* (`db-base`/`db-common-N`: characters,
   clans, reputation). It runs the entire authentication flow — hash
   compare, in-progress dedup, ban check, character list — without
   ever talking to the master. The master is only contacted at
   `selectCharacter` time, *after* the credentials have been
   validated locally.

2. **Master link.** A single `LinkToMaster` per process keeps a
   long-lived authenticated TCP session open to the master. The
   login forwards three things over this link:

   * `forward-login(account_id)` — master replies with the
     CharactersGroup the account belongs to.
   * `forward-character-select(character_id)` — master replies with a
     short-lived token + the `(host, port)` of the gsa to redirect
     the client to (or an error code if locked/unavailable).
   * `forward-character-add/delete` — master persists the character
     mutation and broadcasts to any other login that cached it.

   See `LinkToMasterProtocolParsing.cpp` /
   `LinkToMasterProtocolParsingReply.cpp` /
   `LinkToMasterProtocolParsingMessage.cpp` for the wire format.

3. **Server list + datapack hash propagation.** Master pushes the
   current gsa roster, the datapack hash, and the HTTP mirror URL to
   every login. Login serialises that into the `loginGood` packet
   (or the `serverList` packet for the multi-server UI) so the
   client can:
   * verify it has the right datapack version locally,
   * fetch missing files from the HTTP mirror (or fall back to the
     inline in-protocol push when the mirror string is empty — see
     "Datapack mirror" below),
   * pick a gsa to redirect to.

4. **Per-client inactivity guard.** `TimerDetectTimeout` walks the
   client list once per cadence and disconnects sockets that have
   been idle for too long (60 s in pre-`GameServerConnected` states,
   10 min once a game-server link is established). `lastActivity` is
   refreshed on every `parseIncommingData` AND inside `onAsyncRecv`
   (the io_uring multishot delivery path) — both code paths must
   touch it or new clients get kicked at 60 s while bytes are
   actively arriving.

## Datapack mirror

The login's `httpDatapackMirror` setting controls what the client
receives in the `loginGood` reply:

* **Non-empty URL** (e.g. `http://catchchallenger.fw.local/datapack/`) —
  client fetches `datapack-list/base.txt` and `pack/datapack*.tar.zst`
  over HTTP. This is the production default and the fast path.
* **Empty string** — login encodes a zero-length mirror in the reply.
  Client clears its mirror URL and falls back to the in-protocol
  datapack pull: client sends packet `0xA1` with its local file list,
  login replies with the missing bytes from `./datapack/` next to
  the binary. See `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  and the `BaseServer2.cpp` line that pins
  `datapack_basePath = applicationDirPath + "/datapack/"`.

The in-protocol path requires login to be built **without**
`CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR` (login is never built
with that define; only gsa is). When the build includes
`CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION`, an
empty mirror is fatal (the decompressor that handles the inbound
datapack-list packet is excluded), and login `abort()`s at startup.

## Protocol surface

Login speaks **`PROTOCOL_HEADER_LOGIN`** to clients. Wire format is
identical to a non-cluster `catchchallenger-server-cli` for everything
through `selectCharacter`, then the client receives a
`(token, host, port)` triple and reconnects to a gsa using
`PROTOCOL_HEADER_GAMESERVER`. Login never proxies gameplay packets;
once the client has the token, the login link can be torn down.

If the deployment opts into *proxy mode* (`<mode value="proxy"/>` in
`login.xml`), login keeps a per-client `LinkToGameServer` open and
relays bytes both ways instead of asking the client to reconnect.
This is the workaround for clients behind NAT or firewall rules that
block outbound to high ports. Direct mode is the default.

## Configuration (`login.xml`)

Same `TinyXMLSettings` reader as every other server-type. **Keys are
login-specific**, no overlap with master/gsa/game-server-cli:

| Key                                       | Default | Meaning                                                                       |
|-------------------------------------------|---------|-------------------------------------------------------------------------------|
| `<port>`                                  | —       | listen port for game clients (mandatory)                                      |
| `<server-ip>`                             | `*`     | bind address                                                                  |
| `<max-players>`                           | —       | per-instance connection cap                                                   |
| `<mode>`                                  | `direct`| `direct` (client reconnects to gsa) or `proxy` (login relays bytes)           |
| `<httpDatapackMirror>`                    | `http://localhost/datapack/` | URL advertised to clients; empty → in-protocol push        |
| `<compression>` / `<compressionLevel>`    | `zstd` / 6 | wire compression for client→login                                          |
| `<automatic_account_creation>`            | `false` | auto-create account on first `login` if pseudo unknown                        |
| `<character_delete_time>`                 | `3600`  | soft-delete grace period (seconds)                                            |
| `<min_character>` / `<max_character>`     | `0` / `3` | per-account bounds                                                          |
| `<max_pseudo_size>`                       | `20`    | character-name byte cap                                                       |
| `<maxPlayerMonsters>` / `<maxWarehousePlayerMonsters>` | `8` / `30` | inventory bounds                                                      |
| `<tolerantMode>`                          | `false` | log instead of abort on some protocol violations                              |
| `<master><host>`                          | —       | master IPv4/IPv6 to connect to (mandatory)                                    |
| `<master><port>`                          | —       | master listen port (mandatory)                                                |
| `<master><token>`                         | —       | shared auth secret; master and login must agree (mandatory)                   |
| `<master><tryInterval>` / `<master><considerDownAfterNumberOfTry>` | `5` / `3` | master-reconnect cadence            |
| `<db>`, `<db-login>`, `<db-base>`, `<db-common-N>` | — | per-DB credentials (login uses login + base + per-CharactersGroup common) |

Note that **every CommonSettingsCommon field** (`automatic_account_creation`,
`character_delete_time`, `min/max_character`, `max_pseudo_size`,
`maxPlayerMonsters`, `maxWarehousePlayerMonsters`) must match the
master's copy of the same fields — they're serialised into the C211
reply's raw byte blob and compared at login bring-up. A mismatch
aborts the login at `LinkToMasterProtocolParsingMessage.cpp` with
`C211 different CharactersGroup`. `test/testingcluster.py` mirrors
every value in both `master.xml` and `login.xml` for this reason.

The full key list lives in `EventLoopServerLoginSlave::checkSettingsFile`.

## Database

Login needs:

* **db-login** — account table (read+write: account creation, password
  update). Always per-cluster; never sharded.
* **db-base** — character / clan / reputation tables (read-side; the
  authoritative writes go through master). For sharded clusters this
  may be one DB per CharactersGroup.
* **db-common-N** — one section per CharactersGroup the login serves.
  Each contains its own `(type, host, login, pass, db)` quad. The
  per-group DB is what stores characters that live on the gsa
  instances inside that group.

Login does **not** touch per-server tables (`character_forserver`,
`character_bymap`, `factory`) — those belong to gsa.

## Lifecycle

1. Read `login.xml`, validate keys, load CommonSettingsCommon.
2. Open DB connections (login + base + each common-N).
3. Connect to master, present the shared token, receive the gsa
   roster + datapack hash + mirror URL.
4. Bind listening socket.
5. Pre-render the static `protocolReplyCharacterList` blob using the
   master-provided data (so per-client login replies are a single
   memcpy + write).
6. Accept clients forever. Every `selectCharacter` round-trips
   through master before replying.

## Background timers

* **`TimerDetectTimeout`** — per-client idle disconnect (1 min in
  pre-Game states, 10 min in `GameServerConnected`). Also walks the
  internal `LinkToMaster` and `LinkToGameServer` lists with the same
  idle bounds.

## Source map

| File                                             | Owner                                                                                     |
|--------------------------------------------------|-------------------------------------------------------------------------------------------|
| `main-unix-login-slave.cpp`                      | argv, settings, event-loop dispatch, accept loop                                          |
| `EventLoopServerLoginSlave.{cpp,hpp}`            | listening side: settings, master link bring-up, datapack hash                             |
| `EventLoopClientLoginSlave.{cpp,hpp}`            | per-client state machine (None→ProtocolGood→Logged→CharacterSelecting→GameServerConnected)|
| `EventLoopClientLoginSlaveHeavyLoad.cpp`         | character-list rendering, addCharacter, selectCharacter forwarding                         |
| `EventLoopClientLoginSlaveProtocolParsing.cpp`   | client→login wire-format decoders                                                          |
| `EventLoopClientLoginSlaveWrite.cpp`             | login→client wire-format encoders                                                          |
| `LinkToMaster.{cpp,hpp}`                         | persistent login→master link                                                               |
| `LinkToMasterProtocolParsing*.cpp`               | wire format for the login→master and master→login channels                                 |
| `LinkToGameServer.{cpp,hpp}` (proxy mode only)   | per-client login→gsa relay                                                                 |
| `LinkToGameServerProtocolParsing.cpp`            | wire format for the relay                                                                  |
| `CharactersGroupForLogin.{cpp,hpp}`              | login-side mirror of master's CharactersGroup roster                                       |
| `TimerDetectTimeout.{cpp,hpp}`                   | idle-disconnect sweep                                                                      |

## Operational notes

* **Stateless w.r.t. gameplay.** A login can be added or removed at
  any time without losing player state — characters live on gsa,
  master tracks the routing. Drain by stopping new accepts and
  letting existing sessions time out.
* **`lastActivity` is per-receive-path.** Both `parseIncommingData`
  (epoll dispatch) and `onAsyncRecv` (io_uring multishot delivery)
  refresh `lastActivity`. Add a third receive path → it must touch
  the timestamp too, otherwise `TimerDetectTimeout` kicks active
  clients at 60 s. Same trap applies to `LinkToMaster` and
  `LinkToGameServer` — they also refresh `lastActivity` on every
  read.
* **Token round-trip with master is the slow path.** Cache locality
  matters: `selectCharacter` forwards a tiny packet to master and
  blocks on the reply before the client gets its gsa redirect.
  Master is single-threaded; login → master is the cluster's natural
  serialization point. Don't add work to that path without measuring.
* **Test rig.** `test/testingcluster.py` brings up the full
  master + 2×login + 2×gsa topology with ephemeral PG/MariaDB plus
  an ephemeral nginx on port 18231 fronting the datapack source.
  The script exercises both `<httpDatapackMirror>` values
  (`http://localhost:18231/datapack/` and `""`) so the in-protocol
  push code path stays tested.
