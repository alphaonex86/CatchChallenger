# CatchChallenger Preprocessor Defines Reference

Catalogue of every project-specific preprocessor flag in the CatchChallenger
codebase, with the **scope** (which subtrees consume it), a one-line
description, and a few representative source locations. Vendored third-party
libraries are not scanned: `general/{hps,blake3,libxxhash,libzstd}` and
`client/libqtcatchchallenger/{libogg,libopus,libopusfile,libtiled}`. Header
include guards (`CATCHCHALLENGER_*_H[PP]`) are intentionally omitted.

**Scope legend.** Every flag is classified by the top-level subtree(s) that
reference it (not where it’s defined). `general/` is shared library code, so a
flag that appears only there is global; it’s tagged client-only / server-only /
tools-only when references concentrate in one of those subtrees.


## Server architecture

### `EPOLLCATCHCHALLENGERSERVER`
- **Scope:** global (client + server + tools) — server: base,epoll,fight,game-server-alone,gateway,login,master,qt; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: libbot,stats
- **Description:** Enables the epoll-based async I/O server implementation (Linux high-performance server). Drives all server I/O code paths.
- **Used in:**
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoaderBuff.cpp`
  - `general/fight/FightLoaderSkill.cpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/Map_loader.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderMap.cpp`
  - *(…and 79 more)*

### `EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION`
- **Scope:** global (server + tools) — server: base,gateway,login,master; tools: stats
- **Description:** Disables network packet compression (used by master server, stats tool — saves CPU at the cost of bandwidth).
- **Used in:**
  - `general/base/CompressionProtocol.hpp`
  - `general/base/ProtocolParsingGeneral.cpp`
  - `general/base/CompressionProtocol.cpp`
  - `general/base/ProtocolParsingCheck.cpp`
  - `general/base/ProtocolParsingCheck.hpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/gateway/EpollServerLoginSlave.cpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - *(…and 22 more)*

### `EPOLLCATCHCHALLENGERSERVERNOGAMESERVER`
- **Scope:** server-only (+ general/) — server: master
- **Description:** Compiles without local game-server features (set on master so it stays a pure cluster coordinator).
- **Used in:**
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoaderBuff.cpp`
  - `general/fight/FightLoader.hpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp`
  - `general/base/CommonDatapack.cpp`
  - `server/master/master.pro`


## Server / Client class

### `CATCHCHALLENGER_CLASS_ALLINONESERVER`
- **Scope:** server-only (+ general/) — server: base,epoll,game-server-alone
- **Description:** All-in-one server: gateway + login + game-server + master in one process.
- **Used in:**
  - `general/base/ProtocolParsing.hpp`
  - `server/game-server-alone/game-server-alone.pro`
  - `server/epoll/catchchallenger-server-filedb.pro`
  - `server/epoll/catchchallenger-server-test.pro`
  - `server/epoll/timer/TimeRangeEvent.hpp`
  - `server/epoll/timer/TimeRangeEvent.cpp`
  - `server/epoll/BaseClassSwitch.hpp`
  - `server/epoll/catchchallenger-server-cli-epoll.pro`
  - *(…and 9 more)*

### `CATCHCHALLENGER_CLASS_BOT`
- **Scope:** server-only — server: epoll
- **Description:** Bot/load-test class marker for the epoll server target.
- **Used in:**
  - `server/epoll/BaseClassSwitch.hpp`

### `CATCHCHALLENGER_CLASS_CLIENT`
- **Scope:** client-only — client: qtcpu800x600,qtopengl
- **Description:** Client application class marker.
- **Used in:**
  - `client/qtcpu800x600/base/multi.pri`
  - `client/qtopengl/client.pri`

### `CATCHCHALLENGER_CLASS_GATEWAY`
- **Scope:** server-only (+ general/) — server: epoll,gateway
- **Description:** This is the gateway server (reverse proxy for client login + datapack distribution).
- **Used in:**
  - `general/base/CommonSettingsServer.hpp`
  - `general/base/CommonSettingsCommon.hpp`
  - `general/base/Map_loader.hpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/gateway/gateway.pro`
  - `server/epoll/BaseClassSwitch.hpp`

### `CATCHCHALLENGER_CLASS_LOGIN`
- **Scope:** server-only (+ general/) — server: base,epoll,login
- **Description:** This is the login server (authentication + character selection).
- **Used in:**
  - `general/base/Map_loader.hpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/epoll/BaseClassSwitch.hpp`
  - `server/login/login.pro`
  - `server/base/PreparedDBQuery.hpp`
  - `server/base/ServerStructures.hpp`
  - `server/base/BaseServer/BaseServerLogin.cpp`
  - `server/base/BaseServer/BaseServerLogin.hpp`
  - *(…and 2 more)*

### `CATCHCHALLENGER_CLASS_MASTER`
- **Scope:** server-only (+ general/) — server: epoll,master
- **Description:** This is the master server (central authority for cluster state).
- **Used in:**
  - `general/fight/CommonFightEngineBase.cpp`
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoaderSkill.cpp`
  - `general/fight/CommonFightEngineBase.hpp`
  - `general/fight/FightLoader.hpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp`
  - *(…and 15 more)*

### `CATCHCHALLENGER_CLASS_ONLYGAMESERVER`
- **Scope:** server-only (+ general/) — server: base,epoll,fight,game-server-alone,login
- **Description:** Standalone game server (depends on external login/master cluster).
- **Used in:**
  - `general/base/CompressionProtocol.hpp`
  - `general/base/CommonSettingsCommon.hpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - `server/game-server-alone/game-server-alone.pro`
  - `server/fight/BaseServerFight.cpp`
  - `server/epoll/main-epoll.cpp`
  - `server/epoll/main-epoll2.cpp`
  - *(…and 39 more)*

### `CATCHCHALLENGER_CLASS_P2PCLUSTER`
- **Scope:** server-only — server: epoll
- **Description:** Peer-to-peer cluster mode marker (experimental).
- **Used in:**
  - `server/epoll/BaseClassSwitch.hpp`

### `CATCHCHALLENGER_CLASS_QT`
- **Scope:** global (client + server) — server: base,epoll,login,master,qt; client: qtcpu800x600,qtopengl
- **Description:** Qt-based client (graphical, not CLI).
- **Used in:**
  - `server/MainWindow.cpp`
  - `server/master/EpollServerLoginMaster.cpp`
  - `server/master/CharactersGroup.cpp`
  - `server/qt/catchchallenger-server-qtheader.pri`
  - `server/qt/catchchallenger-server-qt.pri`
  - `server/epoll/main-epoll2.cpp`
  - `server/epoll/BaseClassSwitch.hpp`
  - `server/login/EpollServerLoginSlave.cpp`
  - *(…and 20 more)*

### `CATCHCHALLENGER_CLASS_STATS`
- **Scope:** global (server + tools) — server: epoll; tools: stats
- **Description:** Stats-collector tool (game-server clone gathering metrics).
- **Used in:**
  - `server/epoll/EpollUnixSocketServer.cpp`
  - `server/epoll/BaseClassSwitch.hpp`
  - `tools/stats/stats.pro`


## Database

### `CATCHCHALLENGER_DB_BLACKHOLE`
- **Scope:** server-only — server: base,crafting,epoll,fight,gateway
- **Description:** Discard all writes (testing/stateless — used by gateway).
- **Used in:**
  - `server/fight/BaseServerFight.cpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - `server/fight/LocalClientHandlerFightDatabase.cpp`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - `server/crafting/LocalClientHandlerCrafting.cpp`
  - `server/gateway/gateway.pro`
  - *(…and 41 more)*

### `CATCHCHALLENGER_DB_FILE`
- **Scope:** server-only (+ general/) — server: base,crafting,epoll,fight
- **Description:** File-based DB (flat-file format, no SQL layer). When defined, SQL helpers are excluded from compilation.
- **Used in:**
  - `general/base/GeneralStructures.hpp`
  - `server/catchchallenger-serverheader.pri`
  - `server/fight/BaseServerFight.cpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - `server/fight/LocalClientHandlerFightDatabase.cpp`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - *(…and 43 more)*

### `CATCHCHALLENGER_DB_MYSQL`
- **Scope:** server-only — server: base,crafting,epoll,fight,login,master
- **Description:** MySQL/MariaDB backend (alternative to PostgreSQL).
- **Used in:**
  - `server/fight/BaseServerFight.cpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - `server/fight/LocalClientHandlerFightDatabase.cpp`
  - `server/catchchallenger-server-cli.pro`
  - `server/MainWindow.cpp`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - *(…and 52 more)*

### `CATCHCHALLENGER_DB_POSTGRESQL`
- **Scope:** server-only — server: base,crafting,epoll,fight,login,master
- **Description:** PostgreSQL backend (recommended for production). Links against `libpq`.
- **Used in:**
  - `server/fight/BaseServerFight.cpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - `server/fight/LocalClientHandlerFightDatabase.cpp`
  - `server/catchchallenger-server-cli.pro`
  - `server/MainWindow.cpp`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - *(…and 53 more)*

### `CATCHCHALLENGER_DB_PREPAREDSTATEMENT`
- **Scope:** server-only — server: base,epoll,login,master
- **Description:** Use prepared statements for SQL queries.
- **Used in:**
  - `server/master/master.pro`
  - `server/epoll/db/EpollPostgresql.hpp`
  - `server/epoll/db/EpollPostgresql.cpp`
  - `server/epoll/catchchallenger-server-cli-epoll.pro`
  - `server/login/EpollServerLoginSlave.cpp`
  - `server/login/login.pro`
  - `server/base/PreparedDBQueryServer.cpp`
  - `server/base/DatabaseBase.hpp`
  - *(…and 10 more)*

### `CATCHCHALLENGER_DB_SQLITE`
- **Scope:** global (client + server) — server: base,crafting,epoll,fight,login,master,qt; client: qtcpu800x600,qtopengl
- **Description:** SQLite backend (single-player or development).
- **Used in:**
  - `server/fight/BaseServerFight.cpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - `server/fight/LocalClientHandlerFightDatabase.cpp`
  - `server/catchchallenger-server-cli.pro`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - `server/crafting/LocalClientHandlerCrafting.cpp`
  - *(…and 48 more)*

### `CATCHCHALLENGER_MAXBDQUERIES`
- **Scope:** server-only — server: epoll,qt
- **Description:** Max concurrent DB queries (Qt: `255`, Epoll: `1024`).
- **Used in:**
  - `server/qt/db/QtDatabase.hpp`
  - `server/qt/db/QtDatabase.cpp`
  - `server/epoll/db/EpollMySQL.hpp`
  - `server/epoll/db/EpollMySQL.cpp`
  - `server/epoll/db/EpollPostgresql.hpp`
  - `server/epoll/db/EpollPostgresql.cpp`


## Database blob versioning / safety

### `CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION`
- **Scope:** server-only — server: base,login
- **Description:** Common blob format version (`0`).
- **Used in:**
  - `server/login/EpollServerLoginSlave.cpp`
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_DATABASE_SERVER_BLOBVERSION`
- **Scope:** server-only — server: base
- **Description:** Server-specific blob format version (`0`).
- **Used in:**
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_TRUSTINTODATABASENUMBERRETURN`
- **Scope:** server-only — server: base,epoll
- **Description:** Trust DB return values without validation (perf vs safety).
- **Used in:**
  - `server/epoll/db/EpollPostgresql.cpp`
  - `server/base/DatabaseFunction.cpp`
  - `server/base/VariableServer.hpp`


## Datapack / parser

### `CATCHCHALLENGER_CACHE_HPS`
- **Scope:** global (client + server) — server: base,epoll,game-server-alone,login,master; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Use the HPS binary serializer for the datapack cache.
- **Used in:**
  - `general/base/CommonDatapack.hpp`
  - `general/base/CommonMap/ItemOnMap.hpp`
  - `general/base/CommonMap/Teleporter.hpp`
  - `general/base/CommonDatapackServerSpec.hpp`
  - `general/base/GeneralStructures.hpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - `server/master/main-epoll-login-master.cpp`
  - `server/epoll/filedb-converter/filedb-converter.pro`
  - *(…and 24 more)*

### `CATCHCHALLENGER_CHECK_MAINDATAPACKCODE`
- **Scope:** global (client + server) — server: base,gateway; client: libcatchchallenger
- **Description:** Regex pattern for main-datapack-code validation (`^[a-z0-9]+$`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/gateway/LinkToGameServerStaticVar.cpp`
  - `server/base/BaseServer/BaseServerLoadDatapack.cpp`
  - `server/base/BaseServer/BaseServerSettings.cpp`
  - `client/libcatchchallenger/Api_protocol_loadchar.cpp`

### `CATCHCHALLENGER_CHECK_SUBDATAPACKCODE`
- **Scope:** global (client + server) — server: base; client: libcatchchallenger
- **Description:** Regex pattern for sub-datapack-code validation (`^[a-z0-9]+$`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/base/BaseServer/BaseServerSettings.cpp`
  - `client/libcatchchallenger/Api_protocol_loadchar.cpp`

### `CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK`
- **Scope:** server-only — server: base,epoll
- **Description:** Dump cached datapack trees to disk for debugging.
- **Used in:**
  - `server/epoll/catchchallenger-server-filedb.pro`
  - `server/base/BaseServer/BaseServerLoadMap.cpp`

### `CATCHCHALLENGER_DYNAMIC_MAP_LIST`
- **Scope:** server-only — server: base,qt
- **Description:** Maps are loaded dynamically (1) vs. static.
- **Used in:**
  - `server/catchchallenger-serverheader.pri`
  - `server/qt/QtClientList.cpp`
  - `server/qt/QtClientList.hpp`
  - `server/base/MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp`

### `CATCHCHALLENGER_EXTENSION_ALLOWED`
- **Scope:** global (client + server) — server: base,gateway; client: libcatchchallenger,libqtcatchchallenger
- **Description:** Whitelist of file extensions allowed in datapack (`tmx;xml;tsx;js;png;jpg;gif;ogg;opus`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/gateway/EpollServerLoginSlave.cpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/ClientLoad/ClientHeavyLoad.cpp`
  - `server/base/BaseServer/BaseServerLoadDatapack.cpp`
  - `server/base/BaseServer/BaseServerMasterSendDatapack.cpp`
  - `client/libqtcatchchallenger/Api_client_real_main.cpp`
  - *(…and 5 more)*

### `CATCHCHALLENGER_EXTENSION_COMPRESSED`
- **Scope:** server-only — server: base,gateway
- **Description:** Extensions to compress in datapack (`tmx;xml;tsx;js`).
- **Used in:**
  - `server/gateway/EpollServerLoginSlave.cpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/ClientLoad/ClientHeavyLoad.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/BaseServer/BaseServerLoadDatapack.cpp`
  - `server/base/BaseServer/BaseServerMasterSendDatapack.cpp`

### `CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER`
- **Scope:** server-only — server: base
- **Description:** Player-placed plants (farming feature).
- **Used in:**
  - `server/base/NormalServerGlobal.cpp`
  - `server/base/Client.hpp`
  - `server/base/BaseServer/BaseServerLoadProfile.cpp`

### `CATCHCHALLENGER_NOXML`
- **Scope:** server-only (+ general/) — server: base,epoll,fight
- **Description:** Disable runtime XML parsing — server uses the binary cache only (`datapack-cache.bin`). When set, all XML loaders, TinyXMLSettings, Map_loader, DatapackGeneralLoader, FightLoader{Buff,Monster,Skill}, and tinyXML2 sources are excluded.
- **Used in:**
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoader.hpp`
  - `general/fight/CommonFightEngineTurn.cpp`
  - `general/tinyXML2/tinyxml2.cpp`
  - `general/tinyXML2/tinyxml2c.cpp`
  - `general/tinyXML2/tinyxml2b.cpp`
  - `general/tinyXML2/tinyxml2.hpp`
  - `general/tinyXML2/customtinyxml2.hpp`
  - *(…and 32 more)*

### `CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR`
- **Scope:** server-only (+ general/) — server: base,epoll,game-server-alone,gateway
- **Description:** Game server doesn't serve datapacks directly — clients must use a mirror. Also affects packet/file size limits.
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/game-server-alone/game-server-alone.pro`
  - `server/gateway/EpollServerLoginSlave.cpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/epoll/catchchallenger-server-filedb.pro`
  - *(…and 14 more)*

### `CATCHCHALLENGER_XLMPARSER_TINYXML2`
- **Scope:** server-only — server: epoll
- **Description:** Use TinyXML2 as the XML parser when XML is enabled.
- **Used in:**
  - `server/epoll/catchchallenger-server-epoll.pri`

### `TILED_LIBRARY`
- **Scope:** client-only — client: libqtcatchchallenger
- **Description:** Use the bundled Tiled map-editor library.
- **Used in:**
  - `client/libqtcatchchallenger/libtiled.pri`

### `TILED_ZLIB`
- **Scope:** global (server + tools) — tools: map-procedural-generation-terrain
- **Description:** zlib-compressed map layers in the Tiled loader.
- **Used in:**
  - `general/base/Map_loader.cpp`
  - `general/base/Map_loaderMain.cpp`
  - `general/base/Map_loader.hpp`
  - `server/catchchallenger-serverheader.pri`
  - `tools/map-procedural-generation-terrain/map-procedural-generation-terrain.pri`

### `TILED_ZSTD_SUPPORT`
- **Scope:** client-only — client: libqtcatchchallenger
- **Description:** zstd-compressed map layers in the Tiled loader.
- **Used in:**
  - `client/libqtcatchchallenger/libtiled.pri`

### `TIXML_USE_STL`
- **Scope:** server-only — server: login
- **Description:** Use STL containers in TinyXML.
- **Used in:**
  - `server/login/login.pro`


## Datapack size and compression

### `CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB`
- **Scope:** server-only — server: base,gateway,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/Client.hpp`

### `CATCHCHALLENGER_SERVER_DATAPACK_LZ4_COMPRESSEDFILEPURGE_KB`
- **Scope:** server-only — server: base
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB`
- **Scope:** server-only — server: base,gateway,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/Client.hpp`

### `CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB`
- **Scope:** server-only — server: base,gateway,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/Client.hpp`

### `CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB`
- **Scope:** server-only — server: base,gateway,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/base/VariableServer.hpp`
  - `server/base/Client.hpp`

### `CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB`
- **Scope:** server-only — server: base,gateway,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/Client.hpp`


## Compression / vendored libs

### `BLAKE3_NO_SSE2`
- **Scope:** server-only — server: gateway,login,master
- **Description:** Disable BLAKE3 SSE2 SIMD code path (portability).
- **Used in:**
  - `server/gateway/gateway.pro`
  - `server/master/master.pro`
  - `server/login/login.pro`

### `EXTERNALLIBZSTD`
- **Scope:** server-only (+ general/) — server: epoll
- **Description:** Link against system libzstd (servers) instead of bundled zstd. Clients embed zstd to avoid the runtime dependency.
- **Used in:**
  - `general/general.pri`
  - `server/epoll/catchchallenger-server-filedb.pro`
  - `server/epoll/catchchallenger-server-test.pro`
  - `server/epoll/catchchallenger-server-cli-epoll.pro`

### `OP_DISABLE_HTTP`
- **Scope:** client-only — client: libqtcatchchallenger
- **Description:** Disable HTTP support in libopusfile.
- **Used in:**
  - `client/libqtcatchchallenger/libopusfile.pri`

### `XXH_INLINE_ALL`
- **Scope:** global (general/ only)
- **Description:** Inline xxHash functions for performance.
- **Used in:**
  - `general/libxxhash.pri`

### `ZSTD_MULTITHREAD`
- **Scope:** global (general/ only)
- **Description:** Enable multi-threaded zstd (set to 0 for consistency).
- **Used in:**
  - `general/libzstd.pri`


## Game mode / client features

### `CATCHCHALLENGER_CLIENT_INSTANT_SHOP`
- **Scope:** client-only — client: libcatchchallenger,qtcpu800x600
- **Description:** Instant-shop feature toggle.
- **Used in:**
  - `client/qtcpu800x600/base/interface/BaseWindowBot.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`

### `CATCHCHALLENGER_CLIENT_MAP_CACHE_SIZE`
- **Scope:** client-only — client: libcatchchallenger,libqtcatchchallenger
- **Description:** Maximum number of maps cached client-side (`70`).
- **Used in:**
  - `client/libqtcatchchallenger/maprender/MapVisualiser-map.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`

### `CATCHCHALLENGER_CLIENT_MAP_CACHE_TIMEOUT`
- **Scope:** client-only — client: libcatchchallenger,libqtcatchchallenger
- **Description:** Timeout (seconds) before cached maps are invalidated (`15*60`).
- **Used in:**
  - `client/libqtcatchchallenger/maprender/MapVisualiser-map.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`

### `CATCHCHALLENGER_CLIENT_MUSIC_LOADING`
- **Scope:** client-only — client: libcatchchallenger,qtcpu800x600
- **Description:** Loading music asset path (`/music/loading.opus`).
- **Used in:**
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`

### `CATCHCHALLENGER_MULTI`
- **Scope:** global (client + server) — server: qt; client: libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Multi-player networking mode.
- **Used in:**
  - `server/qt/QtServer.hpp`
  - `server/qt/QtServer.cpp`
  - `client/qtcpu800x600/base/SslCert.h`
  - `client/qtcpu800x600/base/multi.pri`
  - `client/qtcpu800x600/base/interface/BaseWindow.cpp`
  - `client/libqtcatchchallenger/qtcatchchallengerclient.pro`
  - `client/libqtcatchchallenger/Api_protocol_Qt.cpp`
  - `client/qtopengl/client.pri`
  - *(…and 2 more)*

### `CATCHCHALLENGER_NOAUDIO`
- **Scope:** global (client + tools) — client: qtcpu800x600,qtopengl; tools: bot-actions,bot-test-connect-to-gameserver-cli,datapack-downloader-cli,datapack-explorer-generator-cli
- **Description:** Disable audio (used in headless / WASM builds).
- **Used in:**
  - `tools/datapack-downloader-cli/datapack-downloader.pro`
  - `tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro`
  - `tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro`
  - `tools/bot-actions/bot-actions.pro`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/ultimate/mainwindow.h`
  - `client/qtcpu800x600/qtcpu800x600.pro`
  - *(…and 19 more)*

### `CATCHCHALLENGER_SOLO`
- **Scope:** global (client + server) — server: base,qt; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Single-player offline mode.
- **Used in:**
  - `general/base/ConnectedSocket.hpp`
  - `server/qt/InternalServer.cpp`
  - `server/qt/QtServer.hpp`
  - `server/qt/db/QtDatabaseMySQL.cpp`
  - `server/qt/db/QtDatabasePostgreSQL.cpp`
  - `server/qt/QtServer.cpp`
  - `server/qt/QFakeServer.hpp`
  - `server/qt/NormalServer.cpp`
  - *(…and 27 more)*

### `CATCHCHALLENGER_VERSION_SINGLESERVER`
- **Scope:** client-only (+ general/) — client: libqtcatchchallenger,qtcpu800x600
- **Description:** Single-server (no federation/cluster) deployment.
- **Used in:**
  - `general/fight/CommonFightEngineWild.cpp`
  - `general/base/GeneralStructures.hpp`
  - `general/base/FacilityLib.cpp`
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/libqtcatchchallenger/Api_protocol_Qt.cpp`

### `CATCHCHALLENGER_VERSION_SOLO`
- **Scope:** client-only — client: libqtcatchchallenger,qtcpu800x600
- **Description:** Solo-version branding marker.
- **Used in:**
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/base/SslCert.h`
  - `client/libqtcatchchallenger/Api_protocol_Qt.cpp`

### `CATCHCHALLENGER_VERSION_ULTIMATE`
- **Scope:** client-only — client: qtcpu800x600
- **Description:** “Ultimate” client features (extra crafting, shops, UI).
- **Used in:**
  - `client/qtcpu800x600/fight/interface/BaseWindowFight.cpp`
  - `client/qtcpu800x600/crafting/interface/BaseWindowCrafting.cpp`
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/ultimate/specific.pri`

### `NOSINGLEPLAYER`
- **Scope:** global (client + server) — server: base,qt; client: libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Forbid single-player (multiplayer-only client, e.g. WASM).
- **Used in:**
  - `server/qt/QtServer.hpp`
  - `server/qt/QtServer.cpp`
  - `server/base/Client.cpp`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/ultimate/catchchallenger-ultimate.pro`
  - `client/qtcpu800x600/qtcpu800x600.pro`
  - `client/qtcpu800x600/base/client.pri`
  - *(…and 13 more)*


## Client URLs

### `CATCHCHALLENGER_RSS_URL`
- **Scope:** client-only — client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600
- **Description:** News RSS feed URL.
- **Used in:**
  - `client/qtcpu800x600/base/FeedNews.cpp`
  - `client/qtcpu800x600/base/RssNews.cpp`
  - `client/libqtcatchchallenger/FeedNews.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`

### `CATCHCHALLENGER_SERVER_LIST_URL`
- **Scope:** client-only — client: libcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Server-list XML URL (HTTPS by default; HTTP fallback when GCC < 8).
- **Used in:**
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtopengl/foreground/Multi.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`

### `CATCHCHALLENGER_UPDATER_URL`
- **Scope:** client-only — client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600
- **Description:** Updater manifest URL.
- **Used in:**
  - `client/qtcpu800x600/base/InternetUpdater.cpp`
  - `client/libqtcatchchallenger/InternetUpdater.cpp`
  - `client/libcatchchallenger/ClientVariable.hpp`


## Platform metadata

### `CATCHCHALLENGER_GITCOMMIT`
- **Scope:** client-only (+ general/) — client: qtcpu800x600
- **Description:** Git commit hash (optionally injected at build time).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/base/solo/SoloWindow.cpp`

### `CATCHCHALLENGER_PLATFORM_CODE`
- **Scope:** client-only — client: libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Updater identifier (e.g. ‘linux-x86_64-pc’).
- **Used in:**
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/base/PlatformMacro.h`
  - `client/qtcpu800x600/base/InternetUpdater.cpp`
  - `client/libqtcatchchallenger/PlatformMacro.hpp`
  - `client/libqtcatchchallenger/InternetUpdater.cpp`
  - `client/qtopengl/foreground/Multi.cpp`

### `CATCHCHALLENGER_PLATFORM_NAME`
- **Scope:** client-only — client: libqtcatchchallenger,qtcpu800x600
- **Description:** Human-readable platform name (e.g. ‘Linux 64Bits’).
- **Used in:**
  - `client/qtcpu800x600/base/PlatformMacro.h`
  - `client/libqtcatchchallenger/PlatformMacro.hpp`

### `CATCHCHALLENGER_VERSION_PRIVATE`
- **Scope:** global (general/ only)
- **Description:** Internal version string.
- **Used in:**
  - `general/base/Version.cpp`
  - `general/base/VersionPrivate.hpp`


## Network protocol

### `NOTCPSOCKET`
- **Scope:** global (client + server) — server: qt; client: libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Disable raw TCP (browser/WASM builds).
- **Used in:**
  - `general/base/ConnectedSocket.cpp`
  - `general/base/ConnectedSocket.hpp`
  - `server/qt/QtServer.hpp`
  - `server/qt/QtServer.cpp`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/qtcpu800x600/ultimate/mainwindow.cpp`
  - `client/qtcpu800x600/ultimate/mainwindow.h`
  - `client/qtcpu800x600/ultimate/AddServer.cpp`
  - *(…and 16 more)*

### `NOWEBSOCKET`
- **Scope:** global (client + server + tools) — server: qt; client: libqtcatchchallenger,qtcpu800x600,qtopengl; tools: bot-test-connect-to-gameserver-cli,datapack-explorer-generator-cli,map2png,stats
- **Description:** Disable WebSocket support (raw TCP only).
- **Used in:**
  - `general/base/ConnectedSocket.cpp`
  - `general/base/ConnectedSocket.hpp`
  - `server/catchchallenger-server-normal.pri`
  - `server/qt/catchchallenger-server-qt.pri`
  - `tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro`
  - `tools/map2png/map2png.pro`
  - `tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro`
  - `tools/stats/stats.pro`
  - *(…and 13 more)*

### `SERVERSSL`
- **Scope:** global (server + tools) — server: epoll,game-server-alone,gateway,login,master; tools: stats
- **Description:** Enable TLS on server sockets.
- **Used in:**
  - `general/base/ProtocolParsingGeneral.cpp`
  - `server/game-server-alone/LinkToMaster.cpp`
  - `server/game-server-alone/LinkToMaster.hpp`
  - `server/gateway/EpollServerLoginSlave.cpp`
  - `server/gateway/EpollClientLoginSlave.cpp`
  - `server/gateway/LinkToGameServer.hpp`
  - `server/gateway/LinkToGameServer.cpp`
  - `server/gateway/EpollServerLoginSlave.hpp`
  - *(…and 41 more)*


## Buffer / packet limits

### `CATCHCHALLENGER_BIGBUFFERSIZE`
- **Scope:** server-only (+ general/) — server: base,gateway,login
- **Description:** Large transfer buffer (256 KiB normal, 16 MiB when ONLYBYMIRROR).
- **Used in:**
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/base/Client.hpp`

### `CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER`
- **Scope:** server-only (+ general/) — server: base
- **Description:** Per-player large buffer variant.
- **Used in:**
  - `general/base/ProtocolParsing.hpp`
  - `server/base/MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp`

### `CATCHCHALLENGER_COMMONBUFFERSIZE`
- **Scope:** global (general/ only)
- **Description:** Standard network buffer (`4096`).
- **Used in:**
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsing.hpp`

### `CATCHCHALLENGER_COMPRESSBUFFERSIZE`
- **Scope:** global (general/ only)
- **Description:** (De)compression buffer (`16*1024*1024`).
- **Used in:**
  - `general/base/CompressionProtocol.hpp`

### `CATCHCHALLENGER_MAXPROTOCOLQUERY`
- **Scope:** global (server + tools) — server: base,game-server-alone,login,master; tools: stats
- **Description:** Concurrent in-flight protocol queries per client (`16`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsingOutput.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/game-server-alone/LinkToMaster.cpp`
  - `server/master/EpollClientLoginMaster.cpp`
  - `server/login/LinkToMasterStaticVar.cpp`
  - `server/login/LinkToMaster.cpp`
  - *(…and 4 more)*

### `CATCHCHALLENGER_MAX_FILE_SIZE`
- **Scope:** server-only (+ general/) — server: base,gateway
- **Description:** Max file per transmission (`16K` mirror / `8M` normal).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/DatapackDownloader_main.cpp`
  - `server/gateway/DatapackDownloader_sub.cpp`
  - `server/gateway/DatapackDownloaderBase.cpp`
  - `server/base/ClientLoad/ClientHeavyLoad.cpp`
  - `server/base/BaseServer/BaseServerMasterSendDatapack.cpp`

### `CATCHCHALLENGER_MAX_PACKET_SIZE`
- **Scope:** server-only (+ general/) — server: base,gateway
- **Description:** Max packet (`256K` mirror / `1M` normal).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingOutput.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/base/ClientLoad/ClientHeavyLoadMirror.cpp`
  - `server/base/BaseServer/BaseServerLoadDatapack.cpp`

### `CATCHCHALLENGER_MAX_UNCOMPRESSED_FILE_SIZE`
- **Scope:** client-only (+ general/) — client: libcatchchallenger
- **Description:** Max uncompressed file size in datapack (`256M`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `client/libcatchchallenger/ZstdDecode.cpp`

### `CATCHCHALLENGER_MIN_PACKET_SIZE`
- **Scope:** global (general/ only)
- **Description:** Minimum packet size threshold (`128`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`


## DDoS / rate-limiting

### `CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE`
- **Scope:** server-only — server: gateway,login
- **Description:** Samples averaged for the DDoS metric (`8`).
- **Used in:**
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlave.hpp`

### `CATCHCHALLENGER_DDOS_COMPUTERINTERVAL`
- **Scope:** server-only — server: gateway,login
- **Description:** Seconds between DDoS evaluations (`5`).
- **Used in:**
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/gateway/main-epoll-gateway.cpp`
  - `server/login/main-epoll-login-slave.cpp`
  - `server/login/EpollClientLoginSlave.hpp`

### `CATCHCHALLENGER_DDOS_FILTER`
- **Scope:** server-only — server: base,epoll,qt
- **Description:** Enable DDoS detection logic.
- **Used in:**
  - `server/MainWindow.cpp`
  - `server/qt/ProcessController.cpp`
  - `server/epoll/main-epoll2.cpp`
  - `server/epoll/timer/TimerDdos.cpp`
  - `server/epoll/ClientWithMapEpoll.cpp`
  - `server/base/ClientNetworkRead.cpp`
  - `server/base/NormalServerGlobal.cpp`
  - `server/base/ClientNetworkReadWithoutSender.cpp`
  - *(…and 8 more)*

### `CATCHCHALLENGER_DDOS_KICKLIMITCHAT`
- **Scope:** server-only — server: gateway,login
- **Description:** Chat msgs per interval before kick (`15`).
- **Used in:**
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/gateway/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlaveProtocolParsing.cpp`

### `CATCHCHALLENGER_DDOS_KICKLIMITMOVE`
- **Scope:** server-only — server: gateway,login
- **Description:** Movement msgs per interval before kick (`140`).
- **Used in:**
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/gateway/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlaveProtocolParsing.cpp`

### `CATCHCHALLENGER_DDOS_KICKLIMITOTHER`
- **Scope:** server-only — server: gateway,login
- **Description:** Other actions per interval before kick (`45`).
- **Used in:**
  - `server/gateway/EpollClientLoginSlave.hpp`
  - `server/gateway/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/login/EpollClientLoginSlave.hpp`
  - `server/login/EpollClientLoginSlaveProtocolParsing.cpp`

### `CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE`
- **Scope:** server-only — server: base,epoll
- **Description:** Max DDoS score before disconnect (`6`).
- **Used in:**
  - `server/epoll/timer/TimerDdos.cpp`
  - `server/base/ClientNetworkReadWithoutSender.cpp`
  - `server/base/MapServer.hpp`
  - `server/base/MapServer.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/BaseServer/BaseServerLoadMap.cpp`


## Server timing / RNG / cluster IDs

### `CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT`
- **Scope:** global (general/ only)
- **Description:** Minimum RNG roll to trigger an encounter (`7`).
- **Used in:**
  - `general/fight/CommonFightEngine.cpp`
  - `general/base/GeneralVariable.hpp`

### `CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK`
- **Scope:** server-only (+ general/) — server: game-server-alone,master
- **Description:** Max clan ID block (`5`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - `server/master/EpollClientLoginMasterProtocolParsing.cpp`
  - `server/master/EpollClientLoginMaster.cpp`

### `CATCHCHALLENGER_SERVER_MAXIDBLOCK`
- **Scope:** server-only (+ general/) — server: game-server-alone,login,master
- **Description:** Max player ID block in cluster (`50`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingGeneral.cpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - `server/master/EpollClientLoginMasterProtocolParsing.cpp`
  - `server/master/EpollClientLoginMaster.cpp`
  - `server/login/LinkToMasterProtocolParsingReply.cpp`

### `CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION`
- **Scope:** server-only — server: base,epoll,login
- **Description:** Max pending connections before authentication (`50`).
- **Used in:**
  - `server/epoll/main-epoll.cpp`
  - `server/login/main-epoll-login-slave.cpp`
  - `server/login/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/base/ClientNetworkRead.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/BaseServer/BaseServerLogin.hpp`

### `CATCHCHALLENGER_SERVER_MINCLANIDBLOCK`
- **Scope:** server-only (+ general/) — server: base
- **Description:** Min clan ID block (`1`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/base/Client.cpp`

### `CATCHCHALLENGER_SERVER_MINIDBLOCK`
- **Scope:** server-only (+ general/) — server: base,login
- **Description:** Min player ID block in cluster (`20`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/login/CharactersGroupClient.cpp`
  - `server/login/EpollClientLoginSlaveHeavyLoad.cpp`
  - `server/base/Client.cpp`

### `CATCHCHALLENGER_SERVER_MIN_RANDOM_LIST_SIZE`
- **Scope:** server-only — server: base,fight
- **Description:** Minimum random list size (`32`).
- **Used in:**
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_NORMAL_SPEED`
- **Scope:** global (client + server) — server: base; client: libqtcatchchallenger
- **Description:** Standard server turn duration (`250` ms).
- **Used in:**
  - `server/base/VariableServer.hpp`
  - `client/libqtcatchchallenger/maprender/MapControllerMPAPI.cpp`

### `CATCHCHALLENGER_SERVER_OWNER_TIMEOUT`
- **Scope:** server-only — server: base
- **Description:** Inactive connection timeout (`60*60*24` s).
- **Used in:**
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE`
- **Scope:** server-only — server: base,fight
- **Description:** Internal random pool size (`4096`).
- **Used in:**
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/BaseServer/BaseServerLoad.cpp`

### `CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE`
- **Scope:** server-only — server: base,fight
- **Description:** Exposed random pool size (`255`).
- **Used in:**
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/base/VariableServer.hpp`


## Map visibility / movement

### `CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE`
- **Scope:** server-only — server: base
- **Description:** Items dropped on map can collide with movement.
- **Used in:**
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_MAP_DROP_STOP_MOVE`
- **Scope:** server-only — server: base
- **Description:** Items fully block movement.
- **Used in:**
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT`
- **Scope:** server-only — server: base,epoll
- **Description:** ms between movement broadcasts (`150`).
- **Used in:**
  - `server/epoll/BaseServerEpoll.cpp`
  - `server/epoll/main-epoll.cpp`
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR`
- **Scope:** server-only — server: base
- **Description:** Clear-on-move visibility refresh.
- **Used in:**
  - `server/base/VariableServer.hpp`


## Crypto / token

### `CATCHCHALLENGER_HASH_SIZE`
- **Scope:** global (client + server + tools) — server: base,game-server-alone,gateway,login,master; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600; tools: stats
- **Description:** BLAKE3 hash output length (`32` bytes).
- **Used in:**
  - `general/base/ProtocolParsingGeneral.cpp`
  - `general/base/CatchChallenger_Hash.hpp`
  - `general/base/CatchChallenger_Hash.cpp`
  - `server/game-server-alone/LinkToMaster.cpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - `server/gateway/LinkToGameServerProtocolParsing.cpp`
  - `server/master/EpollClientLoginMasterProtocolParsing.cpp`
  - `server/login/LinkToMasterProtocolParsingReply.cpp`
  - *(…and 21 more)*

### `CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER`
- **Scope:** global (client + server) — server: base,game-server-alone,gateway,login,master; client: libcatchchallenger
- **Description:** Game-server connect-token length (`32`).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingGeneral.cpp`
  - `server/game-server-alone/LinkToMasterStaticVar.cpp`
  - `server/game-server-alone/LinkToMaster.hpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - `server/gateway/LinkToGameServerProtocolParsing.cpp`
  - `server/gateway/LinkToGameServer.hpp`
  - `server/master/EpollClientLoginMaster.hpp`
  - *(…and 10 more)*


## Game balance

### `CATCHCHALLENGER_MONSTER_LEVEL_MAX`
- **Scope:** global (client + server) — server: fight; client: qtcpu800x600,qtopengl
- **Description:** Maximum monster level (`100`).
- **Used in:**
  - `general/fight/CommonFightEngineBase.cpp`
  - `general/fight/CommonFightEngineEnd.cpp`
  - `general/fight/CommonFightEngineTurn.cpp`
  - `general/fight/CommonFightEngine.cpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/Map_loader.cpp`
  - `general/base/GeneralVariable.hpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.cpp`
  - *(…and 4 more)*

### `CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER`
- **Scope:** global (general/ only)
- **Description:** Wild monster skill slots (`5`).
- **Used in:**
  - `general/fight/CommonFightEngineBase.cpp`
  - `general/fight/CommonFightEngineWild.cpp`
  - `general/base/GeneralVariable.hpp`

### `CATCHCHALLENGER_SERVER_FACTORY_PRICE_CHANGE`
- **Scope:** server-only — server: base
- **Description:** Shop factory price change percentage (`20`).
- **Used in:**
  - `server/base/VariableServer.hpp`


## Type widths

### `CATCHCHALLENGER_TYPE_BOTID`
- **Scope:** global (client + server + tools) — server: base,epoll; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: bot-actions,libbot
- **Description:** Bot/NPC ID width (`uint8_t`).
- **Used in:**
  - `general/fight/CommonFightEngineEnd.cpp`
  - `general/fight/CommonFightEngine.hpp`
  - `general/fight/FightLoader.hpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp`
  - `general/base/GeneralStructures.hpp`
  - `general/base/GeneralType.hpp`
  - `server/epoll/filedb-converter/FiledbConverter.cpp`
  - `server/base/ClientEvents/LocalClientHandlerDatabaseSync.cpp`
  - *(…and 24 more)*

### `CATCHCHALLENGER_TYPE_BUFF`
- **Scope:** global (client + tools) — client: libqtcatchchallenger; tools: datapack-explorer-generator-cli
- **Description:** Buff ID width (`uint8_t`).
- **Used in:**
  - `general/fight/FightLoaderBuff.cpp`
  - `general/fight/FightLoaderSkill.cpp`
  - `general/fight/FightLoader.hpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/CommonDatapack.hpp`
  - `general/base/CommonDatapack.cpp`
  - `general/base/GeneralType.hpp`
  - `tools/datapack-explorer-generator-cli/GeneratorBuffs.cpp`
  - *(…and 2 more)*

### `CATCHCHALLENGER_TYPE_CRAFTINGRECIPE`
- **Scope:** global (general/ only)
- **Description:** Crafting recipe ID width (`uint8_t`).
- **Used in:**
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderCrafting.cpp`
  - `general/base/CommonDatapack.hpp`
  - `general/base/CommonDatapack.cpp`
  - `general/base/GeneralType.hpp`

### `CATCHCHALLENGER_TYPE_ITEM`
- **Scope:** global (client + server + tools) — server: base,crafting,epoll,login; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: bot-actions,datapack-explorer-generator-cli,libbot
- **Description:** Item ID width (`uint16_t`).
- **Used in:**
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoader.hpp`
  - `general/fight/CommonFightEngineWild.cpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/Map_loader.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderCrafting.cpp`
  - *(…and 38 more)*

### `CATCHCHALLENGER_TYPE_ITEM_QUANTITY`
- **Scope:** global (client + server) — server: base,crafting,epoll; client: libqtcatchchallenger
- **Description:** Item quantity width (`uint32_t`).
- **Used in:**
  - `general/fight/CommonFightEngineWild.cpp`
  - `general/base/GeneralStructures.hpp`
  - `general/base/GeneralType.hpp`
  - `server/crafting/ClientHeavyLoadCrafting.cpp`
  - `server/epoll/filedb-converter/FiledbConverter.cpp`
  - `server/base/ClientEvents/LocalClientHandlerDatabaseSync.cpp`
  - `server/base/ClientEvents/LocalClientHandlerShop.cpp`
  - `server/base/Client.hpp`
  - *(…and 4 more)*

### `CATCHCHALLENGER_TYPE_MAPID`
- **Scope:** global (client + server + tools) — server: base,crafting,epoll,fight; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: bot-actions,datapack-explorer-generator-cli,libbot
- **Description:** Map ID width (`uint16_t`).
- **Used in:**
  - `general/fight/CommonFightEngineEnd.cpp`
  - `general/fight/CommonFightEngine.hpp`
  - `general/base/Map_loader.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/CommonDatapackServerSpec.cpp`
  - `general/base/CommonMap/Map_Border.hpp`
  - *(…and 97 more)*

### `CATCHCHALLENGER_TYPE_MONSTER`
- **Scope:** global (client + tools) — client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: datapack-explorer-generator-cli
- **Description:** Monster species ID width (`uint16_t`).
- **Used in:**
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoader.hpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/CommonDatapackServerSpec.cpp`
  - `general/base/CommonDatapack.hpp`
  - `general/base/GeneralStructures.hpp`
  - `general/base/CommonDatapack.cpp`
  - *(…and 15 more)*

### `CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE`
- **Scope:** global (general/ only)
- **Description:** Party monster count width (`uint8_t`).
- **Used in:**
  - `general/base/GeneralType.hpp`

### `CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE_WAREHOUSE`
- **Scope:** global (general/ only)
- **Description:** Warehouse monster count width (`uint16_t`).
- **Used in:**
  - `general/base/GeneralType.hpp`

### `CATCHCHALLENGER_TYPE_PLANT`
- **Scope:** global (client + server) — server: base,crafting; client: libcatchchallenger,libqtcatchchallenger
- **Description:** Plant ID width (`uint8_t`).
- **Used in:**
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/CommonDatapack.hpp`
  - `general/base/CommonDatapack.cpp`
  - `general/base/GeneralType.hpp`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - `server/crafting/LocalClientHandlerCrafting.cpp`
  - `server/base/Client.hpp`
  - `client/libqtcatchchallenger/Api_protocol_Qt.hpp`
  - *(…and 4 more)*

### `CATCHCHALLENGER_TYPE_QUEST`
- **Scope:** global (client + server + tools) — server: base,epoll; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: datapack-explorer-generator-cli,libbot
- **Description:** Quest ID width (`uint8_t`).
- **Used in:**
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoader.hpp`
  - `general/base/CommonDatapackServerSpec.cpp`
  - `general/base/CommonDatapackServerSpec.hpp`
  - `general/base/GeneralStructures.hpp`
  - `general/base/GeneralType.hpp`
  - `server/epoll/filedb-converter/FiledbConverter.cpp`
  - `server/base/ClientEvents/LocalClientHandlerQuest.cpp`
  - *(…and 18 more)*

### `CATCHCHALLENGER_TYPE_REPUTATION`
- **Scope:** global (general/ only)
- **Description:** Reputation ID width (`uint8_t`).
- **Used in:**
  - `general/base/CommonDatapack.hpp`
  - `general/base/GeneralType.hpp`

### `CATCHCHALLENGER_TYPE_SKILL`
- **Scope:** global (client + tools) — client: libcatchchallenger,libqtcatchchallenger; tools: datapack-explorer-generator-cli
- **Description:** Skill ID width (`uint16_t`).
- **Used in:**
  - `general/fight/FightLoader.cpp`
  - `general/fight/FightLoaderSkill.cpp`
  - `general/fight/FightLoader.hpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/CommonDatapack.hpp`
  - `general/base/GeneralStructures.hpp`
  - `general/base/CommonDatapack.cpp`
  - `general/base/GeneralType.hpp`
  - *(…and 5 more)*

### `CATCHCHALLENGER_TYPE_TELEPORTERID`
- **Scope:** global (general/ only)
- **Description:** Teleporter ID width (`uint16_t`).
- **Used in:**
  - `general/base/GeneralType.hpp`


## Tools / bot / bench

### `BOTACTIONS`
- **Scope:** tools-only — tools: bot-actions,libbot
- **Description:** Player-action bot tool.
- **Used in:**
  - `tools/libbot/MultipleBotConnectionImplForGui.cpp`
  - `tools/bot-actions/bot-actions.pro`

### `BOTTESTCONNECT`
- **Scope:** global (client + tools) — client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600; tools: bot-test-connect-to-gameserver-cli,libbot
- **Description:** Bot connects directly to the game server (skips gateway/login).
- **Used in:**
  - `general/base/Map_loader.cpp`
  - `general/base/CommonDatapackServerSpec.cpp`
  - `general/base/CommonDatapack.cpp`
  - `tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro`
  - `tools/libbot/MultipleBotConnection.cpp`
  - `tools/libbot/MultipleBotConnectionImplForGui.cpp`
  - `client/qtcpu800x600/base/SslCert.h`
  - `client/libqtcatchchallenger/Api_protocol_Qt.cpp`
  - *(…and 1 more)*

### `CATCHCHALLENGER_BOT`
- **Scope:** global (client + tools) — client: libqtcatchchallenger,qtopengl; tools: bot-actions,bot-test-connect-to-gameserver-cli
- **Description:** Bot/automated client (no UI).
- **Used in:**
  - `tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro`
  - `tools/bot-actions/bot-actions.pro`
  - `client/libqtcatchchallenger/Api_protocol_Qt.cpp`
  - `client/libqtcatchchallenger/maprender/MapVisualiserThread.cpp`
  - `client/libqtcatchchallenger/QtDatapackClientLoader.cpp`
  - `client/qtopengl/LanguagesSelect.cpp`
  - `client/qtopengl/above/AddCharacter.cpp`
  - `client/qtopengl/foreground/Multi.cpp`
  - *(…and 2 more)*

### `MAPPROCEDURALGENFULL`
- **Scope:** tools-only — tools: map-procedural-generation
- **Description:** Full procedural map generation.
- **Used in:**
  - `tools/map-procedural-generation/map-procedural-generation.pro`

### `MAXIMIZEPERFORMANCEOVERDATABASESIZE`
- **Scope:** global (server + tools) — server: base,crafting,fight; tools: bot-actions,map2png
- **Description:** Trade larger DB size for speed (used in tools and game-server).
- **Used in:**
  - `general/fight/CommonFightEngine.hpp`
  - `general/fight/CommonFightEngineWild.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - `server/fight/LocalClientHandlerFightDatabase.cpp`
  - `server/crafting/ClientLocalBroadcastCrafting.cpp`
  - `server/crafting/LocalClientHandlerCrafting.cpp`
  - `server/base/ClientEvents/LocalClientHandlerDatabaseSync.cpp`
  - `server/base/ClientEvents/LocalClientHandlerQuest.cpp`
  - *(…and 3 more)*

### `NOTHREADS`
- **Scope:** global (client + server + tools) — server: qt; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: datapack-explorer-generator-cli,map2png
- **Description:** Single-threaded build (WASM/embedded).
- **Used in:**
  - `server/qt/InternalServer.cpp`
  - `server/qt/QtServer.hpp`
  - `tools/map2png/MapDoor.cpp`
  - `tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/qtcpu800x600/qtcpu800x600.pro`
  - `client/qtcpu800x600/base/client.pri`
  - `client/libqtcatchchallenger/Api_client_real_main.cpp`
  - *(…and 25 more)*

### `ONLYMAPRENDER`
- **Scope:** global (client + server + tools) — server: qt; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600; tools: datapack-explorer-generator-cli,map-procedural-generation-terrain,map2png
- **Description:** Map rendering only — skip game logic.
- **Used in:**
  - `general/base/Map_loader.cpp`
  - `general/base/ConnectedSocket.cpp`
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsingGeneral.cpp`
  - `general/base/ConnectedSocket.hpp`
  - `general/base/ProtocolParsingCheck.cpp`
  - `general/base/ProtocolParsingOutput.cpp`
  - `general/base/ProtocolParsingCheck.hpp`
  - *(…and 30 more)*

### `SERVERBENCHMARK`
- **Scope:** server-only — server: base,epoll
- **Description:** Benchmark instrumentation in epoll server.
- **Used in:**
  - `server/epoll/catchchallenger-server-filedb.pro`
  - `server/epoll/EpollUnixSocketClientFinal.hpp`
  - `server/epoll/main-epoll.cpp`
  - `server/epoll/catchchallenger-server-test.pro`
  - `server/epoll/EpollUnixSocketClientFinal.cpp`
  - `server/epoll/catchchallenger-server-cli-epoll.pro`
  - `server/base/ClientLoad/ClientHeavyLoadSelectCharCommon.cpp`
  - `server/base/ClientLoad/ClientHeavyLoadSelectCharFinal.cpp`
  - *(…and 1 more)*


## Build / hardening

### `CATCHCHALLENGER_EXTRA_CHECK`
- **Scope:** global (client + server + tools) — server: base,epoll,fight,game-server-alone,gateway,login,master,qt; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: bot-actions,libbot,map-procedural-generation
- **Description:** Adds extra runtime checks (auto-defined unless RELEASE).
- **Used in:**
  - `general/fight/CommonFightEngineSkill.cpp`
  - `general/fight/CommonFightEngineEnd.cpp`
  - `general/fight/CommonFightEngineWild.cpp`
  - `general/fight/CommonFightEngineTurn.cpp`
  - `general/fight/CommonFightEngine.cpp`
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/Map_loader.cpp`
  - `general/base/GeneralVariable.hpp`
  - *(…and 118 more)*

### `CATCHCHALLENGER_HARDENED`
- **Scope:** server-only — server: epoll
- **Description:** Extra security checks/protections.
- **Used in:**
  - `server/epoll/catchchallenger-server-filedb.pro`
  - `server/epoll/catchchallenger-server-cli-epoll.pro`

### `CATCHCHALLENGER_LIMIT_254CONNECTED`
- **Scope:** server-only — server: epoll
- **Description:** Cap simultaneous connections at 254 (constrained deployments).
- **Used in:**
  - `server/epoll/catchchallenger-server-filedb.pro`

### `CATCHCHALLENGER_RELEASE`
- **Scope:** global (general/ only)
- **Description:** Release build (suppresses EXTRA_CHECK).
- **Used in:**
  - `general/base/GeneralVariable.hpp`

### `CATCHCHALLENGER_SERVER_EXTRA_CHECK`
- **Scope:** server-only (+ general/) — server: base,fight
- **Description:** Server-only extra assertions.
- **Used in:**
  - `general/fight/CommonFightEngineTurn.cpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/base/MapManagement/MapBasicMove.cpp`
  - `server/base/VariableServer.hpp`

### `USING_PCH`
- **Scope:** global (client + server + tools) — server: login; client: libqtcatchchallenger,qtcpu800x600,qtopengl; tools: stats
- **Description:** Enable precompiled headers.
- **Used in:**
  - `server/login/login.pro`
  - `tools/stats/stats.pro`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/libqtcatchchallenger/qtcatchchallengerclient.pro`
  - `client/qtopengl/catchchallenger-qtopengl-uselib.pro`


## Library export markers

### `CATCHCHALLENGERLIB`
- **Scope:** client-only — client: libcatchchallenger,libqtcatchchallenger
- **Description:** This is a library build (not an executable).
- **Used in:**
  - `client/libqtcatchchallenger/libqt.pri`
  - `client/libcatchchallenger/catchchallengerclient.pro`

### `LIBEXPORT`
- **Scope:** client-only (+ general/) — client: libcatchchallenger,libqtcatchchallenger
- **Description:** DLL export marker (Windows).
- **Used in:**
  - `general/base/lib.h`
  - `client/libqtcatchchallenger/qtcatchchallengerclient.pro`
  - `client/libcatchchallenger/catchchallengerclient.pro`

### `LIBIMPORT`
- **Scope:** client-only (+ general/) — client: qtcpu800x600,qtopengl
- **Description:** DLL import marker (Windows).
- **Used in:**
  - `general/base/lib.h`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/qtopengl/catchchallenger-qtopengl-uselib.pro`


## Protocol debug

### `DEBUG_PROTOCOLPARSING_RAW_NETWORK`
- **Scope:** global (client + server + tools) — server: gateway; tools: stats
- **Description:** Log raw network bytes (hex dumps).
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsingOutput.cpp`
  - `server/gateway/LinkToGameServerProtocolParsing.cpp`
  - `server/gateway/LinkToGameServer.cpp`
  - `server/gateway/gateway.pro`
  - `tools/stats/stats.pro`
  - `client/catchchallenger.pro`

### `PROTOCOLPARSINGDEBUG`
- **Scope:** global (client + server + tools) — server: epoll; client: qtcpu800x600,qtopengl; tools: stats
- **Description:** Verbose log of every protocol message parsed.
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsingOutput.cpp`
  - `server/epoll/catchchallenger-server-filedb.pro`
  - `server/epoll/main-epoll.cpp`
  - `tools/stats/stats.pro`
  - `client/qtcpu800x600/qtcpu800x600-uselib.pro`
  - `client/qtcpu800x600/qtcpu800x600.pro`
  - *(…and 2 more)*


## Debug switches

### `CATCHCHALLENGER_ABORTIFERROR`
- **Scope:** global (client + tools) — client: libcatchchallenger; tools: bot-actions
- **Description:** Abort on protocol parsing error (strict mode, used by bot tools).
- **Used in:**
  - `general/base/ProtocolParsingInput.cpp`
  - `tools/bot-actions/bot-actions.pro`
  - `client/libcatchchallenger/Api_protocol.cpp`

### `CATCHCHALLENGER_DEBUG_FIGHT`
- **Scope:** global (client + server) — server: base,fight; client: libqtcatchchallenger,qtcpu800x600,qtopengl
- **Description:** Fight engine turn-by-turn logging.
- **Used in:**
  - `general/fight/CommonFightEngineBuff.cpp`
  - `general/fight/CommonFightEngineEnd.cpp`
  - `general/fight/CommonFightEngineWild.cpp`
  - `general/fight/CommonFightEngineTurn.cpp`
  - `general/base/GeneralVariable.hpp`
  - `server/fight/LocalClientHandlerFight.cpp`
  - `server/fight/ClientHeavyLoadFight.cpp`
  - `server/fight/LocalClientHandlerFightWild.cpp`
  - *(…and 7 more)*

### `CATCHCHALLENGER_DEBUG_FIGHT_BOT`
- **Scope:** global (server + tools) — server: base; tools: libbot
- **Description:** Bot fight behaviour logging.
- **Used in:**
  - `server/base/VariableServer.hpp`
  - `tools/libbot/actions/ActionsBot.cpp`

### `CATCHCHALLENGER_DEBUG_SERVERLIST`
- **Scope:** server-only — server: login
- **Description:** Server-list download debugging.
- **Used in:**
  - `server/login/CharactersGroupForLogin.hpp`
  - `server/login/VariableLoginServer.hpp`
  - `server/login/CharactersGroupForLogin.cpp`
  - `server/login/LinkToMasterProtocolParsingMessage.cpp`

### `CATCHCHALLENGER_SERVER_DEBUG_COMMAND`
- **Scope:** server-only — server: base
- **Description:** Enable admin debug commands.
- **Used in:**
  - `server/base/GameServerVariables.hpp`
  - `server/base/ClientNetworkReadMessage.cpp`
  - `server/base/ClientBroadCast.cpp`

### `DEBUG_MESSAGE_CLIENT_SQL`
- **Scope:** global (client + server) — server: base,epoll,login; client: qtcpu800x600
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/epoll/db/EpollMySQL.cpp`
  - `server/epoll/db/EpollPostgresql.hpp`
  - `server/epoll/db/EpollPostgresql.cpp`
  - `server/login/CharactersGroupClient.cpp`
  - `server/login/EpollClientLoginSlaveHeavyLoad.cpp`
  - `server/base/ClientLoad/ClientHeavyLoad.cpp`
  - `server/base/VariableServer.hpp`
  - `server/base/SqlFunction.cpp`
  - *(…and 3 more)*


## Other

### `CATCHCHALLENGERSERVERDROPIFCLENT`
- **Scope:** global (server + tools) — server: base,epoll,game-server-alone,gateway,login,master; tools: stats
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/base/CompressionProtocol.hpp`
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsingGeneral.cpp`
  - `general/base/CompressionProtocol.cpp`
  - `general/base/ProtocolParsingCheck.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/game-server-alone/LinkToMaster.cpp`
  - `server/game-server-alone/LinkToMasterProtocolParsing.cpp`
  - *(…and 12 more)*

### `CATCHCHALLENGER_CLIENT`
- **Scope:** global (client + server + tools) — server: base; client: libcatchchallenger,libqtcatchchallenger,qtcpu800x600,qtopengl; tools: datapack-explorer-generator-cli
- **Description:** Indicates a client build.
- **Used in:**
  - `general/fight/FightLoaderMonster.cpp`
  - `general/base/CommonDatapackServerSpec.cpp`
  - `general/base/CommonDatapack.hpp`
  - `general/base/CommonDatapackServerSpec.hpp`
  - `general/base/GeneralStructures.hpp`
  - `server/base/ClientNetworkRead.cpp`
  - `server/base/PreparedDBQuery.hpp`
  - `server/base/PreparedDBQueryServer.cpp`
  - *(…and 14 more)*

### `CATCHCHALLENGER_GAMESERVER_EVENTSTARTONLOCALTIME`
- **Scope:** server-only — server: base
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/base/VariableServer.hpp`
  - `server/base/BaseServer/BaseServerLoad.cpp`

### `CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER`
- **Scope:** server-only (+ general/) — server: game-server-alone,gateway,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/game-server-alone/LinkToMasterStaticVar.cpp`
  - `server/gateway/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/login/EpollClientLoginSlaveProtocolParsing.cpp`

### `CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT`
- **Scope:** server-only (+ general/) — server: base,crafting,gateway,login,master
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/base/ProtocolParsingInput.cpp`
  - `general/base/ProtocolParsing.hpp`
  - `server/crafting/LocalClientHandlerCrafting.cpp`
  - `server/gateway/LinkToGameServerProtocolParsing.cpp`
  - `server/gateway/EpollClientLoginSlaveDatapack.cpp`
  - `server/gateway/LinkToGameServer.cpp`
  - `server/master/EpollClientLoginMasterProtocolParsing.cpp`
  - `server/master/EpollClientLoginMaster.cpp`
  - *(…and 15 more)*

### `CATCHCHALLENGER_SAVEGAME_VERSION`
- **Scope:** global (general/ only)
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/base/SavegameVersion.hpp`

### `CATCHCHALLENGER_TOKENSIZE`
- **Scope:** server-only (+ general/) — server: base,login
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/base/GeneralVariable.hpp`
  - `server/login/EpollClientLoginSlaveProtocolParsing.cpp`
  - `server/base/ClientNetworkRead.cpp`
  - `server/base/VariableServer.hpp`

### `CATCHCHALLENGER_XMLATTRIBUTETOSTRING`
- **Scope:** global (general/ only)
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/fight/FightLoaderMonster.cpp`

### `CATCHCHALLENGER_XMLDOCUMENT`
- **Scope:** tools-only — tools: libbot
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `tools/libbot/actions/ActionsBot.cpp`

### `CATCHCHALLENGER_XMLELENTATLINE`
- **Scope:** global (general/ only)
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `general/fight/FightLoaderBuff.cpp`
  - `general/fight/FightLoaderSkill.cpp`
  - `general/base/Map_loader.cpp`
  - `general/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp`

### `HPS_VECTOR_RAW_BINARY`
- **Scope:** server-only — server: epoll
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `server/epoll/catchchallenger-server-filedb.pro`

### `OPENGL`
- **Scope:** client-only — client: qtopengl
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `client/qtopengl/catchchallenger-qtopengl.pro`
  - `client/qtopengl/LanguagesSelect.cpp`
  - `client/qtopengl/catchchallenger-qtopengl-uselib.pro`

### `OPUS_BUILD`
- **Scope:** client-only — client: libqtcatchchallenger
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `client/libqtcatchchallenger/libopus.pri`

### `SRC_DIR`
- **Scope:** tools-only — tools: datapack-explorer-generator-cli
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro`
  - `tools/datapack-explorer-generator-cli/GeneratorMaps.cpp`
  - `tools/datapack-explorer-generator-cli/main.cpp`

### `USE_ALLOCA`
- **Scope:** client-only — client: libqtcatchchallenger
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `client/libqtcatchchallenger/libopus.pri`

### `VAR_ARRAYS`
- **Scope:** client-only — client: libqtcatchchallenger
- **Description:** Purpose not documented; usage details below.
- **Used in:**
  - `client/libqtcatchchallenger/libopus.pri`

