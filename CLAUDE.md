You are expert into C, C++ (preferred) and Qt. You like KISS ("Keep it simple, stupid").
Explain code where is not clear. Try compile with make -j32 after changes to see if remain compiling else fix util it compile. Be compatible from C++11 to C++23.
Before starting a task, always check for a CLAUDE.md file in the current working directory. If it exists, combine its specific instructions with the global rules found in the root CLAUDE.md

# Subdirectory CLAUDE.md (auto-loaded when Claude reads/edits that subdir)

* `test/CLAUDE.md` — testing harness: all.sh, testing*.py, remote/exec nodes, datapack staging, sanitizers/valgrind, image-comparison tolerance, hung-process diagnosis, wall-time optimisation.
* `server/CLAUDE.md` — per-server-type `server-properties.xml` keys, BaseServer init phases.
* `client/CLAUDE.md` — client server-selection CLI flags (`--server` / `--host`+`--port` / `--url`).

* **CMake project layout — one binary per CMakeLists.txt.**
  There is **NO root `CMakeLists.txt`**. Each binary has its own self-contained `CMakeLists.txt` compiling **exactly one** executable. Library subdirs are INTERFACE libs pulled in via `add_subdirectory`.

  **Library subdirs** (INTERFACE libs only):
  - `general/` → `catchchallenger_general` / `catchchallenger_general_minimal`
  - `client/libcatchchallenger/` → `catchchallenger_client_lib`
  - `client/libqtcatchchallenger/` → `catchchallenger_qt_lib` (+ vendored `libogg`, `libopus`, `libopusfile` when audio)
  - `client/libqtcatchchallenger/libtiled/` → `catchchallenger_tiled`
  - `server/base/` → `catchchallenger_server_base` / `catchchallenger_server_db_minimal` / `catchchallenger_server_sql`
  - `server/qt/` → `catchchallenger_qfakesocket` + `catchchallenger_server_qt`

  **Binary subdirs** (one executable each):
  - `server/CMakeLists.txt` → `catchchallenger-server-gui` (Qt admin)
  - `server/cli/` → `catchchallenger-server-cli`
  - `server/cli/filedb-converter/` → `filedb-converter`
  - `server/login/` → `catchchallenger-server-login` (when ported)
  - `server/master/` → `catchchallenger-server-master`
  - `server/gateway/` → `catchchallenger-gateway`
  - `server/game-server-alone/` → `catchchallenger-game-server-alone`
  - `client/` → `catchchallenger` (qtopengl, embedded server for solo)
  - `client/qtcpu800x600/` → `catchchallenger800x600`
  - `tools/<x>/` → `<x>` (e.g. `tools/map2png/` → `map2png`)

  Common setup in `general/CCCommon.cmake`. Reusable lib fragments in `*.cmake` files (one per `*.pri`-style group). Skeleton:
  ```cmake
  cmake_minimum_required(VERSION 3.20)
  project(<name> LANGUAGES C CXX)
  include(${CMAKE_CURRENT_LIST_DIR}/<rel>/general/CCCommon.cmake)
  find_package(Qt6 …)
  include(${CC_REPO_ROOT}/general/general.cmake)
  include(${CC_REPO_ROOT}/<dep1>.cmake)
  add_executable(<name> …)
  target_link_libraries(<name> PRIVATE catchchallenger_general …)
  ```

  **`*.pri` ↔ `*.cmake` 1:1 mapping** (mirrors qmake):
  | .pri | .cmake |
  |---|---|
  | `general/general.pri` | `general/general.cmake` |
  | `client/libcatchchallenger/lib.pri` | `client/libcatchchallenger/lib.cmake` |
  | `client/libqtcatchchallenger/libqt.pri` | `client/libqtcatchchallenger/libqt.cmake` |
  | `client/libqtcatchchallenger/libtiled.pri` | `client/libqtcatchchallenger/libtiled.cmake` |
  | `server/catchchallenger-server.pri` | `server/catchchallenger-server.cmake` |
  | `server/qt/catchchallenger-server-qt.pri` | `server/qt/catchchallenger-server-qt.cmake` |
  | `tools/map-procedural-generation-terrain/*.pri` | same path with `.cmake` |
  | `client/libqtcatchchallenger/audio.cmake` (no .pri — replaces `libogg.pri`+`libopus.pri`+`libopusfile.pri`) | |

  Each `*.cmake` is an `include()` fragment (NOT a CMakeLists.txt), guarded by `if(NOT TARGET …)` so re-includes are idempotent.

  Why no root: each binary needs different PRIVATE defines; INTERFACE libs propagate sources without flags so the same lib produces correct objects per binary. A `--target all` would force a wrong lowest-common-denominator. Also lets external projects `add_subdirectory(general)` etc. without inheriting top-level toggles.

  **Test scripts pin to subdirs.** `testing*.py` invokes `cmake -S <subdir>` per binary. Don't add `cmake -S .`.

* **Build out-of-the-box** on basic VPS CLI with vanilla `cmake` (`cmake -S server -B build/server && cmake --build build/server`, no extra packages) AND Qt Creator on Linux/Windows/macOS. Optional accelerators (ninja, mold, lld, ccache, LTO, sanitizers, custom DB backends) NEVER hard-coded — picked up when present, fallback otherwise. testing*.py toggles dynamically per host.
* prefer not use template, hard to debug
* prefer not use auto, poorly detected into qt creator
* prefer while over for
* never use in-class member variable initialization in .hpp; always init in constructor initializer list in .cpp
* never use Qt5 or lower, only Qt6+; if you need qmake use /usr/bin/qmake6
* never try install/update packages on any system (request it from user)
* never use symlinks in the repository
* **separate compilation artifacts.** CMake build dirs go OUT of source tree. Generated files (`CMakeCache.txt`, `*.o`, `moc_*`, `qrc_*`, `ui_*.h`, `Makefile`, `__pycache__/`) must NEVER appear next to checked-in `.cpp/.hpp/.pro/CMakeLists.txt`. Fix the tool, don't `.gitignore` artefacts.
* never change `/home/user/Desktop/CatchChallenger/working/test/*.png` reference images. Fix the renderer/determinism, not the reference. Only project owner updates these PNGs.
* **Never compare reference images by checksum / md5sum / sha256sum.** PNG is lossless but compression is non-canonical: same pixels encode to different bytes (deflate strategy, filter, palette, metadata chunks, level) — even between two runs of the same Qt build. md5sum match = subset of "pixels match"; mismatch tells nothing. Always use the per-pixel-tolerance checker (`testingmap2png.py`/`testingmap4client.py` ±10% per-channel + diff-mask). Work the diff-mask hint, not bytes.
* forbid adding new Qt modules to .pro/.pri files; reuse only modules already listed
* forbid the Qt `quick` module (no QML/QtQuick); UI is widgets/OpenGL only
* embedded third-party libs — vendored as-is, do NOT modify sources/build flags:
  - general/blake3, general/hps, general/libxxhash, general/libzstd
  - client/libqtcatchchallenger/libogg, libopus, libopusfile, libtiled
* **Prefer fix out of vendor code.** Bug near a vendored lib → fix in OUR call site (guard inputs, hold stronger reference, fix object lifetime). Patching vendor forks from upstream and breaks re-imports. If truly only fixable in vendor, file upstream and route around on our side.
* **No SQL joins.** Fetch related rows with separate `mysqli_query()` calls in PHP loops.
* **Prefer nested `if`/`else` over `continue`.** Don't use guard `continue;`; wrap rest in `if(...) { ... }`. Yes, this nests deeply; that is the style.
* **Logging in .cpp files.** Never `fprintf`/`printf`. Use `qDebug()`/`qWarning()`/`qCritical()` when mostly Qt; `std::cout`/`std::cerr << ... << std::endl` when mostly plain C++ (general/, server/, libcatchchallenger/, ProtocolParsing*, FacilityLibGeneral, etc.).
* **Revert changes that fix nothing.** Only changes that demonstrably move [FAIL] to [PASS] stay. No "maybe-this-helps" patches.
* **When adding to CLAUDE.md, write minimal text — no examples, no rationale unless load-bearing. Keep file from growing.** Prefer the relevant subdir CLAUDE.md (test/, server/, client/) over root.
* **Add temporary debug prints when a bug is opaque — then remove them.** Better than guessing. Sprinkle `std::cerr << "[TRACE] ..." << std::endl` (or `qDebug()` in Qt code) at suspected branches, run, READ output, locate divergence, fix, then strip every `[TRACE]` in the same commit. Workflow: traces → run → root cause → minimal fix → re-run PASS → remove traces → re-run PASS still holds.
* **Don't add monitoring/instrumentation to a binary that's currently under test.** Adding `[TRACE]` prints, log lines, or any other observer code in the middle of a running testing*.py changes the binary's behaviour (timing, stdout volume, pipe back-pressure, valgrind error counts) and the test verdict no longer reflects the original bug. The result is false-positive PASS or false-positive FAIL that wastes a debugging cycle. Workflow: kill the running test → add traces → rebuild → re-run from scratch → read the trace → strip traces → rebuild → re-run to confirm the fix. Never edit the source between test cases that share a build.

# Project Overview
CatchChallenger is a Pokémon-like MMORPG engine in C++/Qt. Designed for extremely constrained hardware (i486 66MHz, Geode LX800, MIPS2, RISC-V, RPI1). EventLoop server handles hundreds of players with 10-20MB RAM base + 1-4KB per player.

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
  unix/          - Linux epoll single-threaded
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
EventLoop: CATCHCHALLENGER_SERVER
Cache: CATCHCHALLENGER_CACHE_HPS, CATCHCHALLENGER_NOXML

# Source-control rhythm

`working/` is the active workspace; `git/` is the git checkout. `sync.sh` rsyncs working → git. **Commit and push frequently** — every meaningful change.

Workflow: edit under `working/` → `bash sync.sh` → `cd git && git add -A && git status` (verify only real changes) → `git commit -m "[Tag] summary"` (`[Fix]`/`[Add]`/`[Improvement]`/`[Clean]`) → `git push`.

If `git status` shows build dirs / `.qtcreator/` / `__pycache__/` / `.claude/` / `*.pro.user` / generated `Makefile`, extend `git/.gitignore` — never commit-include or `git rm --cached` ad-hoc. `.gitignore` is the single source of truth.

# Network Protocol
* Packet: header byte, with/without reply, both directions
* Compression: Zstandard (level 1-9)
* 4 separate DBs: login (accounts), common (shared, characters), server (server-specific)

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
