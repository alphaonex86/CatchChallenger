You are expert into C, C++ (preferred) and Qt. You like KISS ("Keep it simple, stupid").
We are a small company and can't maintain a big project's code, so keep it simple and maintainable. Not overengineered.
Explain code where is not clear. Try compile with make -j32 after changes to see if remain compiling else fix util it compile. Be compatible from C++11 to C++23.
Before starting a task, always check for a CLAUDE.md file in the current working directory. If it exists, combine its specific instructions with the global rules found in the root CLAUDE.md

# Subdirectory CLAUDE.md (auto-loaded when Claude reads/edits that subdir)

* `test/CLAUDE.md` — testing harness: all.sh, testing*.py, remote/exec nodes, datapack staging, sanitizers/valgrind, image-comparison tolerance, hung-process diagnosis, wall-time optimisation.
* `server/CLAUDE.md` — per-server-type `server-properties.xml` keys, BaseServer init phases.
* `client/CLAUDE.md` — client server-selection CLI flags (`--server` / `--host`+`--port` / `--url`).
* `security/CLAUDE.md` — IA security tools (`server.py` auditor, `secret.py` leak scanner, `common.py` shared Ollama/Claude transport); default model `gemma4:26b`; `OLLAMA_HOST` selects local/remote/PHP-router; HARD RULE: backend URL/IP + secret-scan config live OUT of the repo (`~/.config/CatchChallenger/`), never commit infra IP.
* `tools/codecheck/CLAUDE.md` — GENERAL C/C++ code-quality auditor (`codecheck.py`; bugs/naming/clarity/perf, NOT security). Reuses the security/ IA core; engine shared with `server.py codecheck`. Mandatory explicit `--llm`; clang-tidy + triviality skip + compute-only verdict cache + adversarial verify. NOT `test/codecheck.py` (that is the harness self-test).
* `tools/gba2catchchallenger/CLAUDE.md` — Gen3 ROM→datapack converter: sub-datapack overlays (families/diff-only) and the tileset model (32-wide dominant blocks, near-dup fold, gap-fill, region subfolders, composite vs decompose, Wang sets).
* `tools/map-procedural-generation-terrain/CLAUDE.md` — THE standalone base terrain generator (Voronoi biomes + grid-aligned vegetation → one big CSV `all.tmx`); RIGID rectangular features; run-staging.
* `tools/map-procedural-generation/CLAUDE.md` — content/assembly stage: chunks the terrain `all.tmx` into per-map zlib tmx + adds cities/roads/bots/encounters/interiors; staging to `dest/`, zlib-not-zstd render-verify, name/dialog content rules.
* `tools/tileset-tagger/CLAUDE.md` — learn-from-tags pipeline: tag tilesets with a visual vocabulary (logical layer/walkable auto-derived from the maps via the Collisions-cancels-Walkable precedence; visual category human-tagged but pre-filled), report mis-detections, then learn the model structure and transfer to a partial datapack (request missing items proportionally; reproducibility + human-rating gate).

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
* never use exceptions or RTTI (dynamic_cast/typeid), not supported on all platforms
* prefer while over for
* never use in-class member variable initialization in .hpp; always init in constructor initializer list in .cpp
* never use Qt5 or lower, only Qt6+; if you need qmake use /usr/bin/qmake6
* never try install/update packages on any system (request it from user)
* never use symlinks in the repository
* **separate compilation artifacts.** CMake build dirs go OUT of source tree. Generated files (`CMakeCache.txt`, `*.o`, `moc_*`, `qrc_*`, `ui_*.h`, `Makefile`, `__pycache__/`) must NEVER appear next to checked-in `.cpp/.hpp/.pro/CMakeLists.txt`. Fix the tool, don't `.gitignore` artefacts.
* **HARD RULE: never alter the original datapack `/home/user/Desktop/CatchChallenger/CatchChallenger-datapack/`.** Read-only source of truth; tests stage copies elsewhere. Fix the staging/copy, never write to this tree. (Owner-authorized sole exception: the learn-from-tags generator's OUTPUT folder `map/main/generated/` is generator-owned and writable; everything else — existing `map/main/official|test`, `map/tileset/`, etc. — stays read-only. Generated maps reference the target tileset `map/tileset/` by relative path. Dev/iteration may still stage to tmpfs first.)
* **Datapack-derived work/temporal data (tags, learned rules, stats, tool state) is stored OUT of the datapack, under the tool's OWN app subdir `~/.local/share/CatchChallenger/<tool>/datapack-<sha256(abs datapack root path)>/` (XDG `GenericDataLocation`; Qt org/app convention).** NEVER write derived/work data into the datapack tree (not even into its `.tsx`/`.tmx`) — the datapack stays the read-only source of truth, so tagging works on `CatchChallenger-datapack/` too. Use a DEDICATED per-tool subdir (e.g. `tileset-tagger/`), NEVER the bare org dir or a near-duplicate name — the client keeps its data in sibling subdirs `~/.local/share/CatchChallenger/client*/`, so a stray rm/collision there would destroy game data. Sidecar dir keyed by the SHA256 of the datapack's absolute path → one per datapack.
* **`datapack-pkmn/` is a LOCAL-only external reference dataset, NOT part of this project.** Never commit, redistribute, or ship it or its contents. It is a structural learning MODEL only — derive abstract layout/size/distribution rules from it; never copy its assets. All generated content and the target `CatchChallenger-datapack/` must be ORIGINAL and independently created.
* **`.tmx` maps reference `.tsx` tilesets by RELATIVE path only (never absolute).** When writing a map, compute the `<tileset source=...>` relative to the map's directory.
* **HARD RULE — a ROM converter writes ONLY inside its per-ROM output folder `map/main/<label>/`** (e.g. `datapack-pkmn-convert/map/main/firered/`). NEVER create or modify any file outside it. In particular NEVER write the shared `map/invisible.png` / `map/invisible.tsx`: that is the engine's marker tileset for things that are invisible in the Tiled editor because CatchChallenger adds them at runtime (teleporters, bots, …); it is referenced **read-only**. Every map TILE a converter places must be a REAL tile EXTRACTED FROM THE ROM, in a tileset LOCAL under `map/main/<label>/tileset/`. **Do NOT generate/synthesise tiles (no coloured "marker" tiles) — that is a trap.** (Bot SKINS are a separate, deliberate mechanism: extracted sprites matched/reused against the shared `skin/bot/` library — that is by design, not a tile asset.)
* **Do NOT trust the tile-LAYER ORDER or grouping in a `.tmx`** — CatchChallenger finds layers by NAME and rebuilds the layer list at load, INCONSISTENTLY (sometimes re-orders, sometimes not; sometimes inserts/groups layers, sometimes not). Per `client/libqtcatchchallenger/maprender/` (`MapVisualiserOrder::layerChangeLevelAndTagsChange` + `MapItem::addMap`): it deletes hidden-tag/unknown layers, inserts the player object-group **before `WalkBehind`** (else after `Collisions`, else appended), moves the `Moving` group, and inserts NEW object-group layers next to any tile layer holding animated/random/trigger tiles. So the on-disk order/grouping is non-authoritative — a map MUST be correct regardless of layer order or grouping; never rely on "layer X draws above/below Y", nor on layers staying separate or together. (Hence feature layers are real-tile DISJOINT: order-independent by construction.)
* **Semantic/feature tile layers use REAL ROM tiles, DISJOINT (no generated markers).** Each cell's real below-player tile goes to EXACTLY ONE layer chosen by its behaviour — `Water`→Water, `Ledge*`→that ledge layer, `Grass`→Grass, else collidable→`Collisions`, else `Walkable`. Because the ground layers never overlap, hiding any single layer in Tiled removes its cells → a visible change (the requirement; see `map/main/test/city.tmx`). `Walkable` is EMPTY at water/grass/ledge/collision cells: the engine still makes them passable — water/grass/lava via the layer's `walkOn` `monstersCollision` (`map/layers.xml`), ledges are one-way (250-253), collisions block (254). Three guards verify it: `layerVisibilityGuard`, the top-layer-cover guard, and `renderVisibilityGuard` (re-renders every `.tmx` with libtiled on all CPU cores; hiding each layer must change the pixels).
* **The above-player over-tile (`WalkBehind`) is ONLY for player-reachable cells.** `WalkBehind` (drawn above the player) must hold the over-tile of a cell ONLY when the player can stand there — a tree/roof top above a walkable path. For a COLLIDABLE cell the player is never there, so its over stays BELOW the player as a SECOND tile **superposed on a 2nd layer named `Collisions`** — the engine OR-merges every `Collisions` layer for blocking (`Map_loaderMain.cpp` `Collisions[i] |= …`), so additional same-named layers are FREE visual superposition (stack the under + over tile of a wall/building), NOT extra tiles above the player. (Putting collidable bodies on `WalkBehind` wrongly hid the player behind whole buildings — `viridian-city` WalkBehind 352→71 vs the right gen2 reference's 76.) Multiple identically-named layers = "logical OR-merge, visually stacked". Moving/Object markers use the shared `invisible.tsx` tiles 0-3.
* **Draw map features as RIGID, RECTANGULAR, grid-aligned BLOCKS — never scattered/organic per-cell.** Tall grass, tree clusters, water, ledges, paths = solid rigid grid-aligned rectangles (`doc/mapping/Goodmap.png` good vs `Badmap.png` bad). Reasons: (1) it IS the Pokémon GBA style; (2) it compresses far better (long runs of one gid shrink under base64+zstd; per-cell scatter bloats every layer). A map/terrain generator FILLS a feature zone solidly (the terrain generator shapes the zones) — never thin a zone with per-cell noise to look "natural". (`map-procedural-generation-terrain/` is the standalone base terrain generator; its grass is already correct — do not "organic-ify" it downstream.)
* never change `/home/user/Desktop/CatchChallenger/working/test/*.png` reference images. Fix the renderer/determinism, not the reference. Only project owner updates these PNGs.
* **Never compare reference images by checksum / md5sum / sha256sum.** PNG is lossless but compression is non-canonical: same pixels encode to different bytes (deflate strategy, filter, palette, metadata chunks, level) — even between two runs of the same Qt build. md5sum match = subset of "pixels match"; mismatch tells nothing. Always use the per-pixel-tolerance checker (`testingmap2png.py`/`testingmap4client.py` ±10% per-channel + diff-mask). Work the diff-mask hint, not bytes.
* forbid adding new Qt modules to .pro/.pri files; reuse only modules already listed
* forbid the Qt `quick` module (no QML/QtQuick); UI is widgets/OpenGL only
* embedded third-party libs — vendored as-is, do NOT modify sources/build flags:
  - general/blake3, general/hps, general/libxxhash, general/libzstd
  - client/libqtcatchchallenger/libogg, libopus, libopusfile, libtiled
* **System lib first, embedded only as fallback** (blake3 / xxhash / tinyxml2). `EXTERNALLIB*` default ON on EVERY build incl. cross/fleet nodes — each node's (cross) sysroot is a copy of the real target hardware, so its `.so` is what the device runs; prefer it. The vendored sources compile in ONLY when `find_library()` can't locate the lib in the sysroot at configure time. Keep the vendored copy at the latest upstream release (e.g. general/tinyXML2 = v11, single tinyxml2.cpp + tinyxml2.hpp wrapped in `#ifndef CATCHCHALLENGER_NOXML`; see version.txt). tinyXML2 is NOT in the do-not-modify list above — updating it from upstream is allowed.
* **Prefer fix out of vendor code.** Bug near a vendored lib → fix in OUR call site (guard inputs, hold stronger reference, fix object lifetime). Patching vendor forks from upstream and breaks re-imports. If truly only fixable in vendor, file upstream and route around on our side.
* **No SQL joins.** Fetch related rows with separate `mysqli_query()` calls in PHP loops.
* **Prefer nested `if`/`else` over `continue`.** Don't use guard `continue;`; wrap rest in `if(...) { ... }`. Yes, this nests deeply; that is the style.
* **Logging in .cpp files.** Never `fprintf`/`printf`. Use `qDebug()`/`qWarning()`/`qCritical()` when mostly Qt; `std::cout`/`std::cerr << ... << std::endl` when mostly plain C++ (general/, server/, libcatchchallenger/, ProtocolParsing*, FacilityLibGeneral, etc.).
* **Revert changes that fix nothing.** Only changes that demonstrably move [FAIL] to [PASS] stay. No "maybe-this-helps" patches.
* **When adding to CLAUDE.md, write minimal text — no examples, no rationale unless load-bearing. Keep file from growing.** Prefer the relevant subdir CLAUDE.md (test/, server/, client/) over root.
* **Add temporary debug prints when a bug is opaque — then remove them.** Better than guessing. Sprinkle `std::cerr << "[TRACE] ..." << std::endl` (or `qDebug()` in Qt code) at suspected branches, run, READ output, locate divergence, fix, then strip every `[TRACE]` in the same commit. Workflow: traces → run → root cause → minimal fix → re-run PASS → remove traces → re-run PASS still holds.
* **Don't add monitoring/instrumentation to a binary that's currently under test.** Adding `[TRACE]` prints, log lines, or any other observer code in the middle of a running testing*.py changes the binary's behaviour (timing, stdout volume, pipe back-pressure, valgrind error counts) and the test verdict no longer reflects the original bug. The result is false-positive PASS or false-positive FAIL that wastes a debugging cycle. Workflow: kill the running test → add traces → rebuild → re-run from scratch → read the trace → strip traces → rebuild → re-run to confirm the fix. Never edit the source between test cases that share a build.
* **Never comment-out or skip a `run_test ...` line in `test/all.sh`** (or any equivalent test invocation). The total test count must never decrease; an environment-broken test is still a signal worth surfacing as a FAIL/TIMEOUT in the run, not silently hidden. If a host can't run a test today, fix the host or the test — don't disable the test line.
* **Never search or operate from `/`** — the host's full filesystem is multi-TB and a single `find /` / `grep -r /` takes days. Always start searches from the working dir (`./`, `git/`, or a known subtree). Also exclude `/mnt` even when explicitly listing top-level dirs (it backs the build artefacts under `/mnt/data/perso/tmpfs/` plus other unrelated mounts).
* **MXE prefix:** `/mnt/data/perso/progs/catchchallenger-mxe/` (cross compilers under `bin/`, Qt6 under `x86_64-w64-mingw32.shared/qt6/`, runtime libs under `x86_64-w64-mingw32.shared/bin/`). Read-only; never re-install from testing*.py.
* **Don't ask "should I continue?" — just continue.** No "want me to keep going on X?" / "pause here?" prompts; the project is large, those checkpoints add up to hours of round-trips and we never finish. If you need an input (a missing file, a credential, a decision the user owns) request *that specific thing*, then when the answer arrives resume the work immediately. The default is forward motion.
* **Never reset git, never destroy git state.** Other humans/IAs work in this folder in parallel. Forbidden without explicit user request: `git reset --hard`, `git clean -fd`, `git checkout -- .`, `git restore .`, `git branch -D`, `git push --force`, deleting `.git/`, blowing away uncommitted edits, force-checkout overwriting local changes. If unsure, ask.
* **Never run `rm -rf`/`rm -Rf` directly — request permission first.** Too many mistakes wiping data with this command. Exception: executing a previously-created, verified script that contains the `rm` is allowed.
* **Canonical website is `https://catchchallenger.herman-brule.com/`.** `catchchallenger.first-world.info` is the dead old domain — never reintroduce it. (Deployment infra under `first-world.info` — VPS paths `/home/first-world.info/...`, hostnames `cc-server*.first-world.info`, `shop.`/`files.` subdomains — is separate and untouched.)
* **Prod hotfix:** when a bug is in production, `/home/user/Desktop/CatchChallenger/deploy/deploy.sh` redeploys prod — use it to push the fix quickly.

# Project Overview
CatchChallenger is a Pokémon-like MMORPG engine in C++/Qt. Designed for extremely constrained hardware (i486 66MHz, Geode LX800, MIPS2, RISC-V, RPI1). EventLoop server handles hundreds of players with 10-20MB RAM base + 1-4KB per player.

# Architecture
```
general/          - Shared (client & server)
  base/           - Core data, protocol, datapack, map loader
    fight/        - Battle engine (CommonFightEngine, turn-based)
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
    fight/        - Server-side battle
    crafting/     - Crafting
    SQL/          - Prepared statements (PreparedDBQuery*, SqlFunction)
  unix/          - Linux epoll single-threaded
  login/          - Login (auth, server list, DDoS proxy)
  master/         - Master (cluster, CharactersGroup)
  gateway/        - Gateway/proxy
  game-server-alone/ - Standalone (connects to master)
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
* Compression: Zstandard (level 1-9), optional — applied to a small set of packets only (the client character-load protocol), not the hot per-tick packets. Hard-disable via `#ifdef CATCHCHALLENGER_SERVER_NO_COMPRESSION`; runtime off via server-properties `compression=none`.
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
