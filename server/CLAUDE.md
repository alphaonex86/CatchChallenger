# server/ — server binaries

Combine with root CLAUDE.md.

## Per-server-type configuration

Every server binary (`server/{login,master,gateway,game-server-alone,epoll}`) has its **own** `server-properties.xml` schema. Common `<configuration>` root + same `TinyXMLSettings` reader (`server/base/TinyXMLSettings.hpp`, mimics QSettings), but **keys/names/groups differ per-server-type**. Don't assume a key in one exists in another.

Game server (`server/cli`, `server/game-server-alone`) — `NormalServerGlobal::checkSettingsFile`:
- ROOT: `server-port`, `server-ip`, `pvp`, `automatic_account_creation`, `max-players`, `character_delete_time`, `min_character`, `max_character`, `max_pseudo_size`, `maxPlayerMonsters`, `maxWarehousePlayerMonsters`, `everyBodyIsRoot`, `teleportIfMapNotFoundOrOutOfMap`, `sendPlayerNumber`, `tolerantMode`, `compression`, `compressionLevel`.
- `<master>` group: `external-server-ip`, `external-server-port` (only when `CATCHCHALLENGER_CLASS_ONLYGAMESERVER`).
- Groups: `<MapVisibilityAlgorithm>`, `<DDOS>`, `<mapVisibility>`, `<rates>`, `<chat>`, `<programmedEvent>` (with `<day><day>`, `<day><night>`), `<db>`, `<db-login>`, `<db-base>`, `<db-common>`, `<db-server>`, `<city>`, `<content>`.

Login/Master/Gateway have own keysets via their `*GlobalLogin::checkSettingsFile`.

Implications:
* `_TEST_SERVER_PROPERTIES_SEED` templates in `testingclient/bots/http/multi/server.py` are **game-server only**. Don't reuse for login/master/gateway.
* Game-server keys read from XML **root**, NOT inside `<general>` wrapper. Wrapping `server-port` in `<general>` makes server fall back to random-port default at `NormalServerGlobal.cpp:106`.
* New non-game-server tests need their own seed template based on that server's checkSettingsFile.

## Server Initialization

BaseServer phases (preload_N_*): datapack → skins → DB → monsters/skills/buffs → DDoS → events → zones → maps → profiles → visibility → players → dictionary → industries → city capture → RNG → finalize.

## Support
* CLI Linux: epoll/io_uring (priority to io_uring), FILE_DB + PostgreSQL client in async with prepared query (high performance mode) + MariaDB client in async in async with prepared query (high performance mode)
* CLI all OS: select, include windows, FILE_DB (compatibility mode)
* GUI: All OS via QT, FILE_DB + All SQL supported by Qt (compatibility mode and simple for end user)
* remember, you have to be compatible with select/epoll/qt
* cli (all in one) and game-server-alone server, can be compiled io_uring as opticional and maximum performance
* master, login, gateway is io_uring only (mandatory), use sendfile() or function for performance when it need
