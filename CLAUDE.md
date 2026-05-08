You are expert into C, C++ (preferred) and Qt. You like KISS ("Keep it simple, stupid").
Explain code where is not clear. Try compile with make -j32 after changes to see if remain compiling else fix util it compile. Be compatible from C++11 to C++23.

* **CMake project layout — one binary per CMakeLists.txt.**
  There is **NO root `CMakeLists.txt`** and no global "build everything" project. Each binary has its own self-contained `CMakeLists.txt` that compiles **exactly one** executable. Library subdirs are INTERFACE libs that the binaries pull in via `add_subdirectory`.

  **Library subdirs** (no binary, just INTERFACE libs):
  - `general/` → `catchchallenger_general` / `catchchallenger_general_minimal`
  - `client/libcatchchallenger/` → `catchchallenger_client_lib`
  - `client/libqtcatchchallenger/` → `catchchallenger_qt_lib` (+ vendored `libogg`, `libopus`, `libopusfile` when audio)
  - `client/libqtcatchchallenger/libtiled/` → `catchchallenger_tiled`
  - `server/base/` → `catchchallenger_server_base` / `catchchallenger_server_db_minimal` / `catchchallenger_server_sql`
  - `server/qt/` → `catchchallenger_qfakesocket` + `catchchallenger_server_qt`

  **Binary subdirs** (each `CMakeLists.txt` = exactly one executable):
  - `server/CMakeLists.txt` → `catchchallenger-server-gui` (Qt admin server)
  - `server/epoll/CMakeLists.txt` → `catchchallenger-server-cli-epoll`
  - `server/epoll/filedb-converter/CMakeLists.txt` → `filedb-converter`
  - `server/login/CMakeLists.txt` → `catchchallenger-server-login` (when its sources are ported)
  - `server/master/CMakeLists.txt` → `catchchallenger-server-master`
  - `server/gateway/CMakeLists.txt` → `catchchallenger-gateway`
  - `server/game-server-alone/CMakeLists.txt` → `catchchallenger-game-server-alone`
  - `client/CMakeLists.txt` → `catchchallenger` (qtopengl, with embedded server for solo)
  - `client/qtcpu800x600/CMakeLists.txt` → `catchchallenger800x600`
  - `tools/<x>/CMakeLists.txt` → `<x>` (one binary, e.g. `tools/map2png/CMakeLists.txt` → `map2png`)

  **Common setup** lives in `general/CCCommon.cmake`. **Reusable lib fragments** live in `*.cmake` files (one per `*.pri`-style source group). Every binary's `CMakeLists.txt` follows this skeleton:
  ```cmake
  cmake_minimum_required(VERSION 3.20)
  project(<name> LANGUAGES C CXX)
  include(${CMAKE_CURRENT_LIST_DIR}/<rel>/general/CCCommon.cmake)
  find_package(Qt6 …)
  include(${CC_REPO_ROOT}/general/general.cmake)
  include(${CC_REPO_ROOT}/<dep1>.cmake)   # lib.cmake, libqt.cmake, catchchallenger-server.cmake, …
  add_executable(<name> …)
  target_link_libraries(<name> PRIVATE catchchallenger_general …)
  ```

  **`*.pri` ↔ `*.cmake` 1:1 mapping** (mirrors the qmake build):
  | .pri (qmake) | .cmake (CMake) |
  |---|---|
  | `general/general.pri` | `general/general.cmake` |
  | `client/libcatchchallenger/lib.pri` | `client/libcatchchallenger/lib.cmake` |
  | `client/libqtcatchchallenger/libqt.pri` | `client/libqtcatchchallenger/libqt.cmake` |
  | `client/libqtcatchchallenger/libtiled.pri` | `client/libqtcatchchallenger/libtiled.cmake` |
  | `server/catchchallenger-server.pri` | `server/catchchallenger-server.cmake` |
  | `server/qt/catchchallenger-server-qt.pri` | `server/qt/catchchallenger-server-qt.cmake` |
  | `tools/map-procedural-generation-terrain/map-procedural-generation-terrain.pri` | same path with `.cmake` |
  | `client/libqtcatchchallenger/audio.cmake` (no `.pri` equivalent — replaces `libogg.pri` + `libopus.pri` + `libopusfile.pri`) | |

  Each `*.cmake` is an `include()` fragment (NOT a CMakeLists.txt). It declares its INTERFACE/STATIC libs guarded by `if(NOT TARGET …)` so re-includes from multiple binaries are idempotent.

  Why no root: a root `CMakeLists.txt` with `--target all` would try to build every binary in one configure, but each binary needs different PRIVATE defines (`CATCHCHALLENGER_CLASS_QT` vs `_ALLINONESERVER` vs `_ONLYGAMESERVER`, `CATCHCHALLENGER_DB_FILE` vs `_DB_SQLITE`, etc.) — INTERFACE libs propagate sources without compile flags, so the same `server_base` lib produces correct objects for every binary that links it. A unified `--target all` would force a lowest-common-denominator config that's wrong for everyone. The root file is also gone so external projects can `add_subdirectory(general)` (etc.) directly to reuse the libs without inheriting our top-level toggles.

  **Test scripts pin to specific subdirs.** `testing*.py` already invokes `cmake -S <subdir>` for each binary it builds (`testingclient.py` builds `client/qtopengl/`, `testingserver.py` builds `server/epoll/`, etc.). Don't add `cmake -S .` invocations — there is no root project.

* **Build out-of-the-box on:**
  * a basic VPS CLI with vanilla `cmake` only — `cmake -S server -B build/server && cmake --build build/server` (or any other binary subdir) works on stock Linux with no extra packages (no ninja/mold/lld/ccache).
  * Qt Creator on standard Linux/Windows/macOS — opening any binary's `CMakeLists.txt` configures and produces that one executable.

  Optional accelerators (ninja, mold, lld, ccache, LTO, sanitizers, custom DB backends) must NEVER be hard-coded as required. Build picks them up when present, falls back otherwise. testing*.py toggles them dynamically per host. A build node with only `gcc` (no clang) must still produce a working binary; opt-in flags gate themselves on per-node config, never hard requires.
* prefer not use template, hard to debug
* prefer not use auto, poorly detected into qt creator
* prefer while over for
* never use in-class member variable initialization in .hpp; always init in constructor initializer list in .cpp
* never use Qt5 or lower, only Qt6+; if you need qmake use /usr/bin/qmake6
* never try install/update packages on any system (request it from user)
* never use symlinks in the repository
* **separate compilation artifacts into their own folder.** CMake build dirs go OUT of source tree. Generated files (`CMakeCache.txt`, `*.o`, `moc_*`, `qrc_*`, `ui_*.h`, `Makefile`, `__pycache__/`) must NEVER appear next to checked-in `.cpp/.hpp/.pro/CMakeLists.txt`. Fix the tool, don't `.gitignore` artefacts.
* never change `/home/user/Desktop/CatchChallenger/working/test/*.png` reference images. Fix the renderer/determinism, not the reference. Only project owner updates these PNGs.
* forbid adding new Qt modules to .pro/.pri files; reuse only modules already listed
* forbid the Qt `quick` module (no QML/QtQuick); UI is widgets/OpenGL only
* embedded third-party libs — vendored as-is, do NOT modify sources/build flags:
  - general/blake3, general/hps, general/libxxhash, general/libzstd
  - client/libqtcatchchallenger/libogg, libopus, libopusfile, libtiled
* **Prefer fix out of vendor code.** When a bug surfaces in or near a
  vendored lib (libtiled, libogg/opus/opusfile, libxxhash, libzstd, hps,
  blake3), the fix lives in OUR code that calls the vendor — guard the
  call site, hold a stronger reference, validate inputs before the
  call, fix the surrounding object lifetime — never in the vendor lib
  itself.  Patching vendor sources forks them away from upstream and
  makes future re-imports impossible to merge cleanly.  If the only
  possible fix really is in the vendor, file a bug upstream and route
  around it on our side until they land it.
* **No SQL joins.** Fetch related rows with separate `mysqli_query()` calls in PHP loops.
* **Prefer nested `if`/`else` over `continue`.** Don't use guard `continue;` to skip iterations — wrap rest of body in `if(...) { ... }`. Yes, this nests deeply; that is the style here.
* **Logging in .cpp files.** Never `fprintf`/`printf`. Use `qDebug()`/`qWarning()`/`qCritical()` when function/file is mostly Qt; `std::cout`/`std::cerr << ... << std::endl` when mostly plain C++ (general/, server/, libcatchchallenger/, ProtocolParsing*, FacilityLibGeneral, etc.).
* **Revert changes that fix nothing.** Only changes that demonstrably move [FAIL] to [PASS] stay. Don't accumulate "maybe-this-helps" patches.

# Project Overview
CatchChallenger is a Pokémon-like MMORPG engine in C++/Qt. Designed for extremely constrained hardware (i486 66MHz, Geode LX800, MIPS2, RISC-V, RPI1). Epoll server handles hundreds of players with 10-20MB RAM base + 1-4KB per player.

# Architecture
```
general/          - Shared (client & server)
  base/           - Core data, protocol, datapack, map loader
  fight/          - Battle engine (CommonFightEngine, turn-based)
  tinyXML2/       - XML parsing
  blake3/         - Hashing
  hps/            - Binary serialization (HPS cache)
client/
  libcatchchallenger/      - Core C++ lib (no Qt dep)
  libqtcatchchallenger/    - Qt-specific (images, sound)
  qtcpu800x600/            - Widget 800x600 fixed UI
  qtopengl/                - OpenGL responsive UI (smartphones)
server/
  base/           - Common server (Client.hpp, BaseServer, MapManagement)
  epoll/          - Linux epoll single-threaded
  login/          - Login (auth, server list, DDoS proxy)
  master/         - Master (cluster, CharactersGroup)
  gateway/        - Gateway/proxy
  game-server-alone/ - Standalone (connects to master)
  fight/          - Server-side battle
  crafting/       - Crafting
  databases/      - SQL schemas (MySQL, PostgreSQL, SQLite, file-based)
tools/            - Bot testing, map2png, datapack tools, stats
```

# Cluster Architecture
Login Server (auth + server list) → Client gets token → Game Server (validates token). Master coordinates Game Servers, tracks character locks.

## Per-server-type configuration

Every server binary (`server/{login,master,gateway,game-server-alone,epoll}`) has its **own** `server-properties.xml` schema. Common `<configuration>` root + same `TinyXMLSettings` reader (`server/base/TinyXMLSettings.hpp`, mimics QSettings), but **keys, names, groups differ per-server-type**. Don't assume a key in one exists in another.

Game server (`server/epoll`, `server/game-server-alone`) — `NormalServerGlobal::checkSettingsFile` defines:
- ROOT: `server-port`, `server-ip`, `pvp`, `automatic_account_creation`, `max-players`, `character_delete_time`, `min_character`, `max_character`, `max_pseudo_size`, `maxPlayerMonsters`, `maxWarehousePlayerMonsters`, `everyBodyIsRoot`, `teleportIfMapNotFoundOrOutOfMap`, `sendPlayerNumber`, `tolerantMode`, `compression`, `compressionLevel`.
- `<master>` group: `external-server-ip`, `external-server-port` (only when `CATCHCHALLENGER_CLASS_ONLYGAMESERVER`).
- Groups `<MapVisibilityAlgorithm>`, `<DDOS>`, `<mapVisibility>`, `<rates>`, `<chat>`, `<programmedEvent>` (with `<day><day>`, `<day><night>`), `<db>`, `<db-login>`, `<db-base>`, `<db-common>`, `<db-server>`, `<city>`, `<content>`.

Login (`server/login`), Master (`server/master`), Gateway (`server/gateway`) — own keysets via their own `*GlobalLogin::checkSettingsFile` etc.

Implications:
* `_TEST_SERVER_PROPERTIES_SEED` templates in `testingclient.py` / `testingbots.py` / `testinghttp.py` / `testingmulti.py` / `testingserver.py` are for the **game server** only. Don't reuse for login/master/gateway.
* Game-server keys read from XML **root**, NOT inside `<general>` wrapper. Wrapping `server-port` in `<general>` makes server fall back to random-port default at `NormalServerGlobal.cpp:106`.
* New non-game-server tests need their own seed template based on that server's `*GlobalLogin::checkSettingsFile`.

# Key Class Hierarchies
**Server Client** (`server/base/Client.hpp`): inherits ProtocolParsingInputOutput + CommonFightEngine + ClientMapManagement + ClientBase. States: Free → None → ProtocolGood → LoginInProgress → Logged → CharacterSelecting → CharacterSelected.

**Client Protocol** (`client/libcatchchallenger/Api_protocol.hpp`): Api_protocol inherits ProtocolParsingInputOutput + MoveOnTheMap.

**Protocol**: ProtocolParsing → ProtocolParsingBase → ProtocolParsingInputOutput

**Datapack**: CommonDatapack singleton (CommonDatapack::commonDatapack).

**Fight**: CommonFightEngineBase → CommonFightEngine.

# Key Type Definitions (general/base/GeneralType.hpp)
* CATCHCHALLENGER_TYPE_ITEM/MONSTER/SKILL = uint16_t
* CATCHCHALLENGER_TYPE_BUFF = uint8_t
* CATCHCHALLENGER_TYPE_MAPID = uint16_t
* COORD_TYPE = uint8_t (0-127)
* PLAYER_INDEX_FOR_CONNECTED = uint16_t (max 65536)
* CLAN_ID_TYPE = uint32_t

# Key Data Structures (general/base/GeneralStructures.hpp)
* Player_public_informations: name, type, skin, following monster
* Player_private_and_public_informations: cash, items, quests, reputation, monsters, plants, clans
* PublicPlayerMonster: species, level, HP, gender, buffs, catch
* PlayerMonster: + XP, egg steps, skills, character origin
* flat_simplified_map: 1D collision (x + y*width); 0=walkable, 254=blocked, 250-253=ledges

# Conditional Compilation Defines
DB: CATCHCHALLENGER_DB_MYSQL/POSTGRESQL/SQLITE/FILE/BLACKHOLE
Server mode: CATCHCHALLENGER_CLASS_ALLINONESERVER/ONLYGAMESERVER/LOGIN/MASTER/GATEWAY
Epoll: CATCHCHALLENGER_SERVER
Cache: CATCHCHALLENGER_CACHE_HPS, CATCHCHALLENGER_NOXML

# Build System
Qt qmake (.pro/.pri). Targets: catchchallenger-server-gui.pro, catchchallenger-server-cli.pro, server/epoll/, client/qtcpu800x600/, client/qtopengl/.

# Source-control rhythm

`/home/user/Desktop/CatchChallenger/working/` is the active workspace; `/home/user/Desktop/CatchChallenger/git/` is the git checkout. `sync.sh` rsyncs working → git, then **commit and push frequently** — every meaningful change. Don't batch a week of edits into one commit.

Workflow per change:
1. Edit under `working/`.
2. `cd /home/user/Desktop/CatchChallenger && bash sync.sh` (handles excludes — relies on `git/.gitignore` for build artefacts).
3. `cd git && git add -A && git status` — verify only real changes appear; investigate any artefact that leaks past `.gitignore` and add the pattern there, don't `git add` it.
4. `git commit -m "[Tag] short summary"` (`[Fix]`, `[Add]`, `[Improvement]`, `[Clean]` — match existing log style).
5. `git push` — don't accumulate local-only commits.

If `git status` shows build dirs / `.qtcreator/` / `__pycache__/` / `.claude/` / `*.pro.user` / generated `Makefile`, the right fix is to extend `git/.gitignore`, never to commit-include or `git rm --cached` ad-hoc. The .gitignore is the single source of truth for "what's an artefact"; `sync.sh`'s rsync excludes are belt-and-braces only.

# Network Protocol
* Packet: header byte, with/without reply, both directions
* Compression: Zstandard (level 1-9)
* 4 separate DBs: login (accounts), common (shared, characters), server (server-specific)

# Server Initialization
BaseServer phases (preload_N_*): datapack → skins → DB → monsters/skills/buffs → DDoS → events → zones → maps → profiles → visibility → players → dictionary → industries → city capture → RNG → finalize.

## Client server-selection CLI flags — pick exactly ONE

Both clients accept three mutually exclusive ways to point at a server. Exactly one group per launch; passing two+ is rejected (clears all three with `std::cerr` warning).

* **From server list** — `--server NAME`. Selects existing entry (TCP or WebSocket per entry). Always available.
* **TCP direct** — `--host HOST --port PORT` (both required). Only when `CATCHCHALLENGER_NO_TCPSOCKET` not defined.
* **WebSocket direct** — `--url URL` (`ws://...` / `wss://...`). Only when `CATCHCHALLENGER_NO_WEBSOCKET` not defined.

When adding a client front-end:
1. Parse all three groups; only one may set state.
2. Reject + clear all three on conflict.
3. Document in `client/dev.md` and update `--help`.

No `--name` flag — earlier alias of `--server` was removed.

# Game Features
Turn-based monster battles, crafting, quests, reputation, plants, city capture (TvT), day/night events, DDoS protection.

# Performance Design
* 8-bit map coords, 16-bit type IDs
* Single-threaded epoll
* Visibility culling
* ACK-based movement flow control
* HPS binary cache for instant startup
* Prepared statement caching

# Naming Conventions
* Classes: PascalCase
* Methods: camelCase
* Member vars: snake_case_
* Enum values: PascalCase

# Error Handling
* Always check return errors
* Error path: always `std::cerr` unless spammy or critical perf

# Code Organization
* Headers: #ifndef/#define/#endif guards
* Namespace: CatchChallenger
* Includes: relative paths with `../../`

# Code Style
* Indentation: 4 spaces
* Braces: K&R same line
* Initializer lists in constructors
* STL: std::unordered_set, std::vector, std::string

# Key Patterns
* Static decl: `ClassName *ClassName::variable=NULL;` in .cpp
* Inline enum: `enum Name : std::uint8_t { };`

# test

## Datapack `map/main/test/` — intentional bugs as fixtures

The `test/` maincode under `datapack/map/main/test/` is **NOT clean** — intentionally broken to exercise defensive parsing. **Do NOT "fix" these issues.** Expected messages:

* `city.xml`: `Missing attribute: child->Name(): monster`; `map have empty monster layer: ... type:lava total luck is not egal to 100 (0)`.
* `gym.tmx`: `Missing "object" properties (item) for the bot:...`; `Missing map,x or y, type: teleport on it`; `wrong teleporter on the map: gym(1,19), to gym(120,120) because the tp is out of range`.

When adding new validation, add a matching broken fixture. Absence of message (or crash on load) = real failure.

* Run `test/all.sh` for all testing*.py sequentially. `--continue` continues on failure.
* **Fix as many bugs per iteration as possible.** Full run is long; scan `/mnt/data/perso/tmpfs/failed.json` for ALL failures, group by root cause, fix all in this turn.
* **Keep host CPU busy.** Always overlap remote compile threads with local work; prefetch next-phase data; pre-warm ccache. At any minute, host should have cc1plus running OR a binary actively in test phase.
* **Run testing*.py scripts in parallel where they don't conflict.** Group by mutable resources:
  - `testing-cpu/` — testingclient solo only
  - `testing-gl/` — testingclient solo only
  - `testing-filedb/` + port `61917` + `database/` — testingclient multi, testingbots, testingserver, testinghttp, testingmulti (sequential among themselves)
  - `testing-multi-{cpu,gl}/` + port `61917` — testingmulti only
  - Cross-compile scripts — own toolchains, can run alongside above
  Safe parallel lanes: (1) testingmap2png + testingmap4client; (2) testingcompilation{windows,mac,android}.
* **Compile under `nice -n 19 ionice -c 3`; runtime tests at NORMAL priority.** `NICE_PREFIX_COMPILE = ["nice","-n","19","ionice","-c","3"]` for cmake/make/qmake; `NICE_PREFIX_RUNTIME = []` for start_server/run_client/bot-actions/autosolo. Don't `nice` server/client/bot binaries.
* Do NOT use Monitor to poll testing*.py. Run with run_in_background (timeout up to 12h), wait for completion.
* Compilation on remote (982.vps.confiared.com, 2803:1920::3:440a) and local can parallelize.
* Client valid + on map: `success_marker="MapVisualiserPlayer::mapDisplayedSlot()"`.
* testingserver.py PASS: client console shows `CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer is now: \n` AND `CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase is now: \n`.
* testinghttp.py PASS: NOT have those messages.
* Server startup OK: console shows `correctly bind:`.
* NOXML: generate `datapack-cache.bin` via server `--save`, then recompile with CATCHCHALLENGER_NOXML. Server checks `datapack-cache.bin` mtime matches `server-properties.xml`. Sync mtime if XML modified. Clean cache after each test.
* `datapack-cache.bin` is NOT endianless: float/double via raw memcpy (HPS), so LE-built cache won't load on BE.
* Client connects → datapack syncs to server's.
* Resume workflow with `failed.json`:
  1. `test/all.sh --continue`
  2. Failures stored in `/mnt/data/perso/tmpfs/failed.json`
  3. Fix code, `make -j32` until compiles
  4. `test/all.sh --continue` — only re-runs failed cases
  5. Repeat 3-4
  6. `rm -f /mnt/data/perso/tmpfs/failed.json`
  7. Full run again
  8. If failures, repeat
* Force fresh: `rm -f /mnt/data/perso/tmpfs/failed.json`
* Target node: `--node <label>` (repeatable) or `CC_NODE_FILTER=mips-lxc,x86-lxc`. Bypasses per-node `enabled` flag.

## All remote_nodes.json fields are mandatory

Every key in the `_doc` block MUST be present on every node and execution_node entry. No defaults — `remote_build.py` validates at module load and aborts with precise error. Required keys: `remote_build._REQUIRED_NODE_KEYS` and `_REQUIRED_EXEC_NODE_KEYS`.

When adding a field, update three places:
1. `_doc` block in `remote_nodes.json`
2. `_REQUIRED_…` tuple in `remote_build.py`
3. **Every** existing node/execution_node entry

Empty != missing. Write `compile_sql=[]`, `extra_defines=[]` etc. explicitly.

## Database backends in testing*.py — file_db on host, SQL gated to remote_nodes.json

**testing*.py never opens an SQL DB connection.** Host runs only file-db (`CATCHCHALLENGER_DB_FILE`); no remote/exec node has SQL pre-configured. Direct `psycopg2.connect` / `mysql.connector.connect` from testing*.py is forbidden. State wiping for SQL = let server binary do it (drop tables + replay schemas).

**SQL coverage opt-in per node:**

* Compile node: `"compile_sql": ["MySQL", "PostgreSQL", "SQLite"]` — extra build passes. Default `[]` = file-db only.
* Execution node: `"execute_sql": [...]` — extra `start server + connect client` passes; SQL daemon expected on `localhost`, default port, DB **`catchchallenger`**, harness asks server to wipe state (drop tables, replay `server/databases/<backend>/*.sql`). Default `[]` = file-db only.

testingserver.py + execute_sql contract: operator has installed daemon, started it on localhost default port, created DB `catchchallenger`, created user `catchchallenger`/password `catchchallenger` with full DDL+DML, left running.

testingserver.py responsibilities: drop tables in DB; replay `.sql` files; connect via server with standard creds.

testingserver.py MUST NOT: systemctl/service/launch daemon; CREATE/DROP DATABASE; CREATE USER; hard-code different host/port/user. Always `localhost`, default port, `catchchallenger` user/pass/DB.

SQLite: no daemon, no auth — fixed path `<paths.tmpfs_root>/catchchallenger.sqlite3`, `sqlite3` CLI. Operator just needs `sqlite3` on $PATH.

## Some "remote" nodes AND execution nodes are local LXC containers

`mips-lxc` (`2803:1920::2:ff03`), `x86-lxc` (`2803:1920::2:ff04`), `osxcross` (`2803:1920::2:ff08`) are **local LXC payloads** on the same host. Same containers also appear as execution nodes in other compile-node entries. They reach via host's `2803:1920::2:/112` private prefix, look like SSH-reachable nodes, but share host CPU/RAM and live under `/sys/fs/cgroup/lxc.payload.<name>/cgroup.procs`.

Implications:
* Own PID namespace — `ps` on host won't show container's binaries by internal PID. Iterate `/sys/fs/cgroup/lxc.payload.*/cgroup.procs` for host-side pids.
* ccache slot at `<work_dir>/ccache/` on container FS. Read via `ssh <user>@<lxc-ip> 'CCACHE_DIR=<path> ccache -s'`.
* Kill hung processes via SSH (`ssh root@<lxc-ip> kill <pid-inside-container>`) not from host (cross-ns kill privileged).
* Monitor cc1plus across BOTH host and LXC payload cgroups.

## Cross-platform client phases (testingclient.py)

Three optional client phases at the end of `testingclient.py`, all use **local Linux `server-filedb`** as multiplayer server. Server/tools NOT cross-compiled.

**`--host` for multi runs:**
* Clients on host (wine64) use `SERVER_HOST` (localhost/127.0.0.1).
* Clients on remote (qemu mac VM, Android emulator) use this host's LAN IP — `SERVER_HOST_REMOTE = "192.168.158.10"` in `testingclient.py`.

Solo runs (`--autosolo --closewhenonmap`) need no server.

## Environment hygiene for test scripts

Behave as if from fresh post-restart shell:

1. **No global pip installs.** Use venv under workspace dir (Android: `/mnt/data/perso/progs/CatchChallenger-android/venv/`).
2. **Build env from scratch.** Construct fresh `dict` (not `os.environ.copy()`); explicitly set every var. Forward only minimum (`HOME`, `USER`, `LANG`, `LC_*`, `TERM`, `TMPDIR`, `DISPLAY`). Don't inherit `ANDROID_*`, `QT_*`, `JAVA_HOME`, `LD_LIBRARY_PATH`, `PYTHONPATH`, `QT_QPA_*`.
3. **No leakage to parent shell.** Never `os.environ[X] = ...`. Each subprocess gets env via `subprocess.run(..., env=...)`.
4. **Workspace dirs exempt.** `/mnt/data/perso/progs/CatchChallenger-android/` etc. allowed to grow files/caches.

Reference: `android_env()` in `test/testingclient.py`.

* **Windows** — local cross-compile via dedicated MXE at `/mnt/data/perso/progs/catchchallenger-mxe/` (`MXE_TARGETS="x86_64-w64-mingw32.shared"`). Build via cmake+ninja with ccache+mold (when present). Run under `wine64`. Deploy with `windeployqt.exe` + MXE mingw runtime DLLs (libstdc++/libgcc/libwinpthread). Datapack at `<exe_dir>/datapack/internal/`. Both clients tested with `QT_QPA_PLATFORM=offscreen`: autosolo phase + multi phase against local `server-filedb`. Success marker: `MapVisualiserPlayer::mapDisplayedSlot()`. Produces NSIS installer .exe (or .zip fallback), .msi via WiX 3.11 under wine64, Authenticode signatures.

  **MSI/signing tooling at `/mnt/data/perso/progs/msi/`:**
  - `wix3/` — WiX 3.11 binaries
  - `test-codesign.pfx` — self-signed RSA-2048/SHA-256 cert, password `catchchallenger`
  - `osslsigncode` 2.9 (system binary). RFC-3161 timestamp from `http://timestamp.digicert.com`; falls back untimestamped.

* **macOS** — osxcross container `root@2803:1920::2:ff08`, target `darwin20.4`, Qt 6.5.3 at `/root/qt6-macos/6.5.3/macos`. Setup notes: `/mnt/data/perso/lxc/osxcross.txt`. Sources rsynced to `/root/catchchallenger-test/`. Build with osxcross cmake wrapper + ninja+ccache+lld (mold has no Mach-O). After build: `Qt/bin/macdeployqt`. Datapack staged as sibling of `.app`. Code-signing best-effort (ad-hoc `--sign -`). Packaging: portable `.zip` of `.app` + `datapack/`. Self-skips when ssh times out. Compile + package only — no runtime/multi phase (Mach-O can't run on Linux).

* **Android** — local Qt-for-Android cross-compile + local emulator. **Only `client/qtopengl`** (qtcpu800x600 skipped). Tooling under `/mnt/data/perso/progs/CatchChallenger-android/`: `sdk/`, `avd/`, `apk/`, `build/`. Self-skips when VPS unreachable or SDK/adb/emulator/AVD missing. Server/tools NOT cross-compiled.

## Diagnostic-tool runs — clang+sanitizer **and** gcc+valgrind

Two mutually-exclusive run modes. Both wired into every `testing*.py` and `all.sh`, propagated to remote nodes.

1. **Clang + sanitizer** — `--sanitize asan|lsan|msan` — clang-built with sanitizer. ~2x slowdown.
2. **Gcc + valgrind** — `--valgrind memcheck|helgrind|drd` — gcc debug build under valgrind. 10-50x slowdown.

`test/diagnostic.py` is shared helper.

### Clang + sanitizer

* `--sanitize asan` — `-fsanitize=address,undefined`. Default-and-broadest.
* `--sanitize lsan` — `-fsanitize=leak`. Only leaks at exit.
* `--sanitize msan` — `-fsanitize=memory,undefined -fsanitize-memory-track-origins=2`. False-positives on uninstrumented deps. Targeted use only.

All add `-fno-omit-frame-pointer -fno-sanitize-recover=all -O1`. Env: `*_OPTIONS` with `halt_on_error=1`, `abort_on_error=1` (asan/msan), `exitcode=23` (lsan). Build dirs: `build-sanitize-<tool>/`, `build-remote-sanitize-<tool>/`.

### Gcc + valgrind

* `--valgrind memcheck` — `--leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=definite,possible`.
* `--valgrind helgrind` — lock-ordering. Epoll server is single-threaded so should be no-op.
* `--valgrind drd` — alternative race detector.

Scales every timeout by **10x**.

`--sanitize` and `--valgrind` are mutually exclusive.

### Remote nodes

Two opt-in layers:
* **Compile**: `"compilers": ["gcc"]` or `["gcc","clang"]`. `--sanitize` requires `clang`; `--valgrind` requires `gcc`. Missing compiler = skip (no failure).
* **Execution** (mandatory booleans):
  * `sanitizer_gcc` — true opts into `--valgrind`. Effective only if compile node has `gcc`.
  * `sanitizer_clang` — true opts into `--sanitize`. Effective only if compile node has `clang`.

Both default `false` on new nodes. Operator must explicitly verify (`valgrind` or `lib*san` runtime). Harness won't auto-install.

Layers AND together: runs only when (compile has compiler) AND (exec has flag).

## Network — test box ↔ remote / execution nodes

Test box IPv6: `2803:1920::2:10/112`. Remote nodes in same `/112` (mips-lxc=`2803:1920::2:ff03`, x86-lxc=`2803:1920::2:ff04`, pentium-m=`2803:1920::2:ff01`, atom-n455=`2803:1920::2:ff02`).

Firewall (one-way for NEW):
* **NEW: test box → `2803:1920::2:/112` allowed.** OUTPUT chain accepts every dest.
* **NEW: `2803:1920::2:/112` → test box blocked.** Only RELATED/ESTABLISHED return packets pass.

**Remote/exec nodes are receive-only.** SSH keys only test box → remote (not reverse). Always drive ssh/rsync/staging from test box outward.

## Datapack staging cache

Solves dominant per-test cost (5-15s × dozens × per `all.sh`). Staged once at startup, symlinked thereafter.

### Cache layout

* **Local** — `/mnt/data/perso/tmpfs/cc-datapack/<datapack-id>/`. ID = source basename. tmpfs RAM-speed. Persistent; `all.sh` excludes from per-run wipe.
* **Remote** — per-exec-node `datapack_cache` field in `remote_nodes.json`. Default `/home/user/datapack-cache/`. Layout: `<datapack_cache>/<datapack-id>/...`. NEVER wiped — `--delete` keeps in sync.

### Staging step

`test/stage_datapacks.py` runs once at `all.sh` startup, after tmpfs cleanup, before any `testing*.py`. Every datapack from `~/.config/catchchallenger-testing/config.json:paths.datapacks` rsynced (`-art --delete`) in parallel: one local + one ssh-rsync per enabled exec_node × per datapack. Blocks until all finish. Failure → `[ABORT] datapack staging failed`.

**Direction always test-box → remote.** Matches firewall+ssh policy.

### Test-side use

* **Local tests** — `os.symlink(datapack_stage.staged_local(src), <build>/datapack)` instead of `shutil.copytree`. ~5-15s → ~1ms.
* **Remote exec phase** — one SSH round-trip: `mkdir -p <work> && rm -rf <work>/datapack && ln -sfn <datapack_cache>/<id> <work>/datapack`.
* **Maincode pruning dropped.** Cache shared between tests; mustn't mutate. server-properties.xml's `mainDatapackCode` controls which maincode loads.

### When NOT to symlink

Tests that mutate the destination must copy:
* `setup_client_cache_partial` (testinghttp) — writes partial cache, deletes some files.
* Per-player XDG cache dirs (testingmulti) — running client adds files via mirror-sync.
* Cross-compile harnesses (testingcompilation{windows,mac}) — symlinks across emulation flaky.

These read from `<build>/datapack` (now a symlink into tmpfs) so even copy is faster.

### Module map

* `test/datapack_stage.py` — `staged_local(src)`, `staged_remote(node, src)`, `remote_cache_for(node)`, `datapack_id(src)`, `stage_all(srcs, exec_nodes)`. Constants: `LOCAL_CACHE_ROOT`, `DEFAULT_REMOTE_CACHE`.
* `test/stage_datapacks.py` — one-shot driver for `all.sh`.
* `test/remote_nodes.json` — `execution_nodes` entry has `datapack_cache` field (default `/home/user/datapack-cache/`).
* `test/all.sh` — wipes tmpfs except persistent caches (`cc-datapack/`, `ccache-catchchallenger/`), then runs `stage_datapacks.py`.

## Diagnosing a hung process — gdb attach, then decide

When a process appears stuck, NOT `pkill -9` first — `gdb -p <pid>` to see what it's doing.

```bash
pid=$(pgrep -f "catchchallenger.*--autosolo" | head -1)
gdb -batch -p "$pid" \
    -ex "set pagination off" \
    -ex "thread apply all bt 60" \
    -ex "detach" -ex "quit" 2>&1 | tee /tmp/hung-bt.txt
```

| Top of stack | Diagnosis | Action |
|---|---|---|
| `epoll_wait`/`poll`/`select`/`recvmsg`/`accept`/`read` | Idle, blocked on I/O. **Not a hang.** | Detach. Test timeout too tight or peer never sent expected packet. |
| `pthread_cond_wait`/`futex_wait` | Blocked on event-loop/mutex/condvar | Inspect siblings — if other thread waits on related lock, real deadlock. |
| `sqlite3_step`/`pq_get_result`/`mysql_real_query` | Waiting on SQL daemon. | Detach. Check daemon. |
| `__libc_*`/`malloc_consolidate`/`arena_…` | Heap corruption | Capture bt, kill, look at recent allocator changes. |
| Tight loop in our code | Infinite loop | Real bug. Capture, kill, fix. |
| All `??` | Stripped/missing debug | Rebuild Debug, retry. |

Only "infinite loop" justifies kill.

Variants:
* **LXC**: `ssh root@<lxc-ip> gdb -batch -p <container-pid>` (preferred) or host gdb with cgroup.procs pid.
* **Under valgrind/sanitizer**: skip — wrappers produce better trace on hang.
* **Solo/autosolo client (in-process server)**: Qt event loop shows `QEventLoop::exec`/`epoll_wait` (idle, not hang). Look at every thread; interesting is the game-logic thread.

Don't replace with `strace -p` — strace tells you syscall, not stack.

## Image-comparison tolerance (testingmap2png.py / testingmap4client.py)

Same two rules — keep in sync:

* **Per-pixel tolerance — ±10% per channel.** Pixel "different" only when at least one R/G/B/A differs by `> 0.10 * ref`. Reference 0 = new must be exactly 0.
* **Per-image budget — 0 pixels.** ANY failing pixel fails the whole test. Fix the source (pin rand, freeze animation, lock GL state) — don't widen budget.

FAIL line: `<failed>/<total> pixels diff > 10% (<pct>%) [hint]; first fail @ (x,y) channel <C>: new=<n> ref=<r>; diff mask saved to <path>`. Mask: `/mnt/data/perso/tmpfs/fail-map4client.png` or `/mnt/data/perso/tmpfs/fail.png` — black on white.

**Diff-percentage triage** (printed inside the `[hint]`):
* **`<10%`** → real renderer / save-state / RNG-seed bug. Same datapack, same maincode, only a handful of pixels drifted; investigate the source code path that produced them.
* **`10-40%`** → ambiguous. Camera scrolled by one tile, different sprite frame, partial UI clip. Read the diff mask AND check savegame state.
* **`>40%`** → almost always **wrong datapack or wrong `--main-datapack-code`**. The whole scene differs; verify the launch flags match the ones used to capture the reference PNG before chasing source-code regressions.

## Bisecting a failing test (testingbisect.py)

`paths.git_repo` (default `/home/user/Desktop/CatchChallenger/git`) is the source repo; `paths.bisect_work` (default `<tmpfs_root>/bisec`) is per-commit scratch. `testingbisect.py` walks the last `--max-commits` (default 20) newest→oldest, materialises each commit with `git archive`, overlays the *current* test/ scripts on top (so harness fixes don't pollute the bisect target), and runs `--script <name>` until the named `--test` either fails (≤ `--runs-per-commit` retries) or passes them all. Stops at the first GOOD after a BAD; prints last-BAD / first-GOOD for the operator to chase. Doesn't drive `git bisect` directly — flat-list walk is friendlier with flaky failures.

## Port probing — local only when server is local

* `cmd_helpers.assert_port_or_fail_with_remotes(...)` ssh's into exec_node and runs `bash /dev/tcp/<test-box>/<port>` — ALWAYS fails when server is on test box (new connection in blocked direction). Use remote probe ONLY when target is the exec_node (testingremote.py exec phase).
* Tests with server on test box (testingserver, testingbots, testingmulti, testinghttp): only LOCAL probe (`127.0.0.1:port`) is meaningful.
* testingremote.py exec phase: server on exec_node, probe direction test-box → exec_node:port allowed; local probe alone is correct.
* Never expect remote/exec node to dial out. Bidirectional traffic? Drive both halves from test box (ssh tunnels, or client on test box vs server on exec node).
