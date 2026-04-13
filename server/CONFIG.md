# CatchChallenger Server Configuration

## Overview

The CatchChallenger server uses **XML-based configuration files** parsed by the `TinyXMLSettings` class (wrapping TinyXML2). All settings live inside a `<configuration>` root element. Groups are represented as nested XML elements, and values are stored as `value` attributes.

```xml
<configuration>
    <setting value="..."/>
    <group>
        <nested-setting value="..."/>
    </group>
</configuration>
```

Missing settings are automatically populated with defaults on first run by `NormalServerGlobal::checkSettingsFile()` (in `base/NormalServerGlobal.cpp`). Values are validated and clamped by `BaseServer::loadAndFixSettings()` (in `base/BaseServer/BaseServerSettings.cpp`).

## Configuration Files by Server Type

| Server Type | Config File | Notes |
|---|---|---|
| Normal Server (GUI/CLI) | `server-properties.xml` | Full standalone server |
| Login Server | `login.xml` | Authentication server |
| Game Server (alone) | `server-properties.xml` | Connects to master server |
| Epoll Server | `server-properties.xml` | Linux high-performance variant |

The file is located in the application/build directory.

## Root-Level Settings

| Setting | Type | Default | Description |
|---|---|---|---|
| `server-ip` | string | `""` (all interfaces) | IP address to bind |
| `server-port` | int | random 10000-65535 | Listen port (valid: 1-65535) |
| `max-players` | int | `10` | Max concurrent players (clamped 1-65533) |
| `useSsl` | bool | `false` | Enable SSL/TLS |
| `proxy` | string | `""` | SOCKS proxy address |
| `proxy_port` | int | `9050` | Proxy port |
| `automatic_account_creation` | bool | `true` (GUI), `false` (epoll) | Auto-create accounts on login |
| `anonymous` | bool | `false` | Anonymous mode |
| `sendPlayerNumber` | bool | `false` | Broadcast player count to clients |
| `character_delete_time` | int | `604800` (7 days) | Character deletion delay in seconds |
| `max_character` | int | `3` | Max characters per account (min: 1) |
| `min_character` | int | `1` | Min characters per account |
| `max_pseudo_size` | int | `20` | Max character name length |
| `maxPlayerMonsters` | int | `8` | Max monsters in party |
| `maxWarehousePlayerMonsters` | int | `30` | Max monsters in warehouse |
| `everyBodyIsRoot` | bool | `false` | Give all players admin access |
| `teleportIfMapNotFoundOrOutOfMap` | bool | `true` | Teleport player if map is invalid |
| `dontSendPseudo` | bool | `false` | Hide player names from others |
| `dontSendPlayerType` | bool | `false` | Hide player types from others |
| `plantOnlyVisibleByPlayer` | bool | `false` | Plants visible only to planter |
| `forceClientToSendAtMapChange` | bool | `true` | Force client state update on map change |
| `pvp` | bool | `true` | Enable PvP combat |
| `tolerantMode` | bool | `false` | Relaxed error checking |
| `compression` | string | `"zstd"` | Compression type: `"zstd"` or `"none"` |
| `compressionLevel` | int | `6` | Compression level (clamped 1-9) |
| `datapackCache` | int | `-1` | Datapack cache: `-1` disable, `0` no timeout, `>0` timeout in seconds |
| `httpDatapackMirror` | string | `""` | HTTP mirror URLs for datapack download (`;` separated) |
| `server_blobversion_datapack` | int | `0` | Server datapack blob version |
| `common_blobversion_datapack` | int | `0` | Common datapack blob version |
| `programmedEventFirstInit` | bool | `true` | Whether programmed events need initialization |

## Group: `<db>` - Database Synchronization

```xml
<db>
    <db_fight_sync value="FightSync_AtTheEndOfBattle"/>
    <positionTeleportSync value="false"/>
    <secondToPositionSync value="0"/>
</db>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `db_fight_sync` | string | `"FightSync_AtTheEndOfBattle"` | When to sync fight data. Options: `FightSync_AtEachTurn`, `FightSync_AtTheDisconnexion`, `FightSync_AtTheEndOfBattle` |
| `positionTeleportSync` | bool | `true` | Sync position on teleport |
| `secondToPositionSync` | int | `0` | Position sync interval in seconds (0 = disabled) |

## Groups: `<db-login>`, `<db-base>`, `<db-common>`, `<db-server>` - Databases

Four separate databases are configured with identical structure:

- **db-login** - Authentication/account data
- **db-base** - Character/player data
- **db-common** - Shared game data
- **db-server** - Server-specific data

```xml
<db-login>
    <type value="mysql"/>
    <host value="localhost"/>
    <db value="catchchallenger_login"/>
    <login value="catchchallenger-login"/>
    <pass value="catchchallenger-pass"/>
    <file value="database.sqlite"/>
    <tryInterval value="5"/>
    <considerDownAfterNumberOfTry value="3"/>
</db-login>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `type` | string | compile-time default | Database type: `"mysql"`, `"postgresql"`, or `"sqlite"` |
| `host` | string | `"localhost"` | Database host (mysql/postgresql only) |
| `db` | string | varies per group | Database name (mysql/postgresql only) |
| `login` | string | varies per group | Database username (mysql/postgresql only) |
| `pass` | string | `"catchchallenger-pass"` | Database password (mysql/postgresql only) |
| `file` | string | `"database.sqlite"` | SQLite file path (sqlite only) |
| `tryInterval` | int | `5` | Retry interval in seconds (min: 1) |
| `considerDownAfterNumberOfTry` | int | `3` | Retries before marking DB as down (min: 1) |

**Validation**: `tryInterval * considerDownAfterNumberOfTry` must not exceed 600 seconds (10 minutes). If exceeded, both values reset to defaults.

Default database names per group:

| Group | Default DB Name | Default Login |
|---|---|---|
| `db-login` | `catchchallenger_login` | `catchchallenger-login` |
| `db-base` | `catchchallenger_base` | `catchchallenger-base` |
| `db-common` | `catchchallenger_common` | `catchchallenger-login` |
| `db-server` | `catchchallenger_server` | `catchchallenger-login` |

## Group: `<rates>` - Experience and Currency Multipliers

```xml
<rates>
    <xp_normal value="1.000000"/>
    <gold_normal value="1.000000"/>
    <xp_pow_normal value="1.000000"/>
    <drop_normal value="1.000000"/>
</rates>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `xp_normal` | double | `1.0` | XP multiplier (1.0 = 100%) |
| `gold_normal` | double | `1.0` | Gold multiplier |
| `xp_pow_normal` | double | `1.0` | Power XP multiplier |
| `drop_normal` | double | `1.0` | Item drop rate multiplier |

Internally these values are multiplied by 1000 for integer storage.

## Group: `<DDOS>` - DDoS Protection

```xml
<DDOS>
    <computeAverageValueTimeInterval value="5"/>
    <waitBeforeConnectAfterKick value="30"/>
    <kickLimitMove value="60"/>
    <kickLimitChat value="5"/>
    <kickLimitOther value="30"/>
    <dropGlobalChatMessageGeneral value="20"/>
    <dropGlobalChatMessageLocalClan value="20"/>
    <dropGlobalChatMessagePrivate value="20"/>
</DDOS>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `computeAverageValueTimeInterval` | int | `5` | Analysis window in seconds (min: 1) |
| `waitBeforeConnectAfterKick` | int | `30` | Cooldown after kick (seconds) |
| `kickLimitMove` | int | `60` | Max moves per interval before kick |
| `kickLimitChat` | int | `5` | Max chat messages per interval before kick |
| `kickLimitOther` | int | `30` | Max other actions per interval before kick |
| `dropGlobalChatMessageGeneral` | int | `20` | Drop percentage for general chat |
| `dropGlobalChatMessageLocalClan` | int | `20` | Drop percentage for clan chat |
| `dropGlobalChatMessagePrivate` | int | `20` | Drop percentage for private chat |

## Group: `<chat>` - Chat Permissions

```xml
<chat>
    <allow-all value="true"/>
    <allow-local value="true"/>
    <allow-private value="true"/>
    <allow-clan value="true"/>
</chat>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `allow-all` | bool | `true` | Enable global/general chat |
| `allow-local` | bool | `true` | Enable local area chat |
| `allow-private` | bool | `true` | Enable private messages |
| `allow-clan` | bool | `true` | Enable clan chat |

## Group: `<mapVisibility>` - Map Visibility Optimization

```xml
<mapVisibility>
    <enable value="true"/>
    <minimize value="network"/>
    <Max value="50"/>
    <Reshow value="30"/>
</mapVisibility>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `enable` | bool | `true` | Enable visibility optimization |
| `minimize` | string | `"network"` | Optimization target: `"cpu"` or `"network"` |
| `Max` | int | `50` | Max visible players (min: 5, clamped to max_players) |
| `Reshow` | int | `30` | Reshow threshold (min: 3, must be <= Max) |

## Group: `<MapVisibilityAlgorithm>` - Algorithm Selection

```xml
<MapVisibilityAlgorithm>
    <MapVisibilityAlgorithm value="0"/>
</MapVisibilityAlgorithm>
```

| Value | Algorithm |
|---|---|
| `0` | Simple |
| `1` | None |
| `2` | WithBorder |

## Group: `<city>` - City Capture Events

```xml
<city>
    <capture_frequency value="month"/>
    <capture_day value="monday"/>
    <capture_time value="0:0"/>
</city>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `capture_frequency` | string | `"month"` | `"week"` or `"month"` |
| `capture_day` | string | `"monday"` | Day of week: `monday` through `sunday` |
| `capture_time` | string | `"0:0"` | Time in `H:M` format (hour 0-24, minute 0-60) |

## Group: `<programmedEvent>` - Day/Night Cycle Events

```xml
<programmedEvent>
    <day>
        <day>
            <value value="day"/>
            <cycle value="60"/>
            <offset value="0"/>
        </day>
        <night>
            <value value="night"/>
            <cycle value="60"/>
            <offset value="30"/>
        </night>
    </day>
</programmedEvent>
```

Each event has:

| Setting | Type | Description |
|---|---|---|
| `value` | string | Event identifier (e.g. `"day"`, `"night"`) |
| `cycle` | string | Cycle period in minutes |
| `offset` | string | Start offset in minutes within the cycle |

The default creates a 60-minute cycle where day runs for 30 minutes then night for 30 minutes.

## Group: `<content>` - Datapack and Server Content

```xml
<content>
    <mainDatapackCode value="test"/>
    <subDatapackCode value=""/>
    <server_message value=""/>
    <exportedXml value=""/>
    <daillygift value="itemidnumber,percentluck;itemidnumber,percentluck"/>
</content>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `mainDatapackCode` | string | auto-detected or `"[main]"` | Main map folder name under `map/main/`. Must match `^[a-z0-9\- _]+$` |
| `subDatapackCode` | string | `""` | Sub-variation folder (optional) |
| `server_message` | string | `""` | Server welcome message (max 16 lines, 128 chars/line, `\n` separated) |
| `exportedXml` | string | `""` | Exported XML data |
| `daillygift` | string | `""` | Daily gift config: `itemid,percent;itemid,percent` |

## Group: `<statclient>` - Authentication Token

```xml
<statclient>
    <token value="630FBD806D4330FB97286CFB2D9178B2"/>
</statclient>
```

| Setting | Type | Description |
|---|---|---|
| `token` | string | Hex authentication token for stat client |

## Group: `<master>` - Master Server Connection (Game Server Only)

Only present when compiled as `CATCHCHALLENGER_CLASS_ONLYGAMESERVER`:

```xml
<master>
    <host value="localhost"/>
    <port value="9999"/>
    <external-server-ip value=""/>
    <external-server-port value=""/>
    <uniqueKey value="..."/>
    <charactersGroup value="..."/>
    <logicalGroup value=""/>
    <token value=""/>
    <considerDownAfterNumberOfTry value="3"/>
    <tryInterval value="5"/>
    <maxLockAge value="30"/>
    <purgeLockPeriod value="15"/>
</master>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `host` | string | `"localhost"` | Master server host |
| `port` | int | `9999` | Master server port |
| `external-server-ip` | string | same as `server-ip` | Externally visible IP |
| `external-server-port` | string | same as `server-port` | Externally visible port |
| `uniqueKey` | string | random | Unique server identifier |
| `charactersGroup` | string | random | Characters group identifier |
| `logicalGroup` | string | `""` | Logical grouping identifier |
| `token` | string | `""` | Authentication token for master |
| `considerDownAfterNumberOfTry` | int | `3` | Retries before marking master as down |
| `tryInterval` | int | `5` | Retry interval in seconds |
| `maxLockAge` | int | `30` | Max lock age in seconds |
| `purgeLockPeriod` | int | `15` | Lock purge period in seconds |

## Group: `<Linux>` - Linux TCP Options (Epoll Only)

```xml
<Linux>
    <tcpCork value="false"/>
    <tcpNodelay value="false"/>
</Linux>
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `tcpCork` | bool | `false` | Enable TCP_CORK socket option |
| `tcpNodelay` | bool | `false` | Enable TCP_NODELAY socket option |

## Example Complete Configuration

See `epoll/build/llvm-Debug/server-properties.xml` for a working example.

## Key Source Files

| File | Purpose |
|---|---|
| `base/TinyXMLSettings.hpp` | XML config parser (read/write) |
| `base/NormalServerGlobal.cpp` | Default value initialization (`checkSettingsFile`) |
| `base/BaseServer/BaseServerSettings.cpp` | Validation and clamping (`loadAndFixSettings`) |
| `base/ServerStructures.hpp` | Settings data structures |
| `base/GlobalServerData.hpp` | Global access to loaded settings |
