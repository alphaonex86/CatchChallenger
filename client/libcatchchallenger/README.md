# libcatchchallenger — the minimal C++ client library

Plain C++ (C++11..C++23) client core: protocol, datapack parsing, datapack
checksum. **No Qt, no Qt resources (`.qrc`), no audio, no images, no rendering,
no network transport.** It only depends on `general/`.

Verified: `tools/bot-bench` (this lib + a POSIX socket) links only
libblake3, libxxhash, libtinyxml2, libzstd, libz and the C++ runtime.

## What is (not) inside

| In this lib | NOT in this lib (where it lives) |
|---|---|
| Client protocol state machine (`Api_protocol*`) | TCP / WebSocket / SSL socket (`libqtcatchchallenger/Api_protocol_Qt`, or `tools/libbot/cli/CliSocket` for Qt-free) |
| Datapack text parsing into C++ structs (`DatapackClientLoader`) | Tileset / image loading (`libqtcatchchallenger/QtDatapackClientLoader`) |
| Datapack checksum, file mtime/partial-hash cache (`DatapackChecksum`) | HTTP mirror download (`libqtcatchchallenger/Api_client_real*`) |
| `tar` + `zstd` in-memory decoders (`TarDecode`, `ZstdDecode`) | Audio playback, opus/ogg (`libqtcatchchallenger` + `audio.cmake`) |
| Map movement / collision on the parsed maps (`canGoTo()`, `move()`) | Map rendering, pathfinding (`libqtcatchchallenger/maprender/`) |

Images and audio are only **relative path strings** read from the datapack XML
(`ItemExtra::imagePath`, `MonsterExtra::frontPath/backPath`,
`ZoneExtra::audioAmbiance`, `get_audioAmbiance()`). The lib never opens them;
a headless consumer just ignores them.

## Files

| File | Role |
|---|---|
| `Api_protocol.hpp/.cpp` | Abstract client: login, character select, game-server reconnect, send commands (move, chat, shop, fight, trade, clan, quest...) |
| `Api_protocol_message.cpp` | Server → client messages (no reply) |
| `Api_protocol_query.cpp` | Server → client queries (client must reply) |
| `Api_protocol_reply.cpp` | Replies to the client's own queries |
| `Api_protocol_loadchar.cpp` | Character block (inventory, monsters, quests, rescue point...) |
| `DatapackClientLoader.hpp/.cpp` | Abstract datapack loader: base then main/sub, fills `CommonDatapack` singletons + client-only "extra" texts |
| `DatapackChecksum.hpp/.cpp` | Base/main/sub datapack hash (must match the server's) |
| `TarDecode`, `ZstdDecode` | Decode a `.tar.zst` datapack archive in memory |
| `ClientStructures.hpp` | Server list, logical groups, `Bot` |
| `Map_client.hpp/.cpp` | `CommonMap` + client border info |
| `ClientVariable.hpp` | Client constants (server list URL, timings) |
| `BlacklistPassword` | Weak-password list for account creation |
| `lib.cmake` | CMake INTERFACE target `catchchallenger_client_lib` |

## Dependencies

* `general/` (`general/general.cmake` → `catchchallenger_general`): protocol
  parsing, `CommonDatapack`, map loader, hashes.
* Through it: blake3, xxhash, tinyxml2, zstd, zlib — system lib first, vendored
  fallback (`EXTERNALLIB*` options in `general/CCCommon.cmake`).
* POSIX headers (`sys/stat.h`, `utime.h`, `unistd.h`); `__WIN32__` paths in
  `DatapackChecksum.cpp`.

## Build (CMake)

Minimal Qt-free binary (same as `tools/bot-bench/CMakeLists.txt`):

```cmake
cmake_minimum_required(VERSION 3.20)
project(myclient LANGUAGES C CXX)
include(${CMAKE_CURRENT_LIST_DIR}/<rel>/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)
add_executable(myclient main.cpp MyApiClient.cpp MyDatapackLoader.cpp)
target_link_libraries(myclient PRIVATE
    catchchallenger_common_flags catchchallenger_general catchchallenger_client_lib)
```

`lib.cmake` adds `CATCHCHALLENGER_CLIENT` and `CATCHCHALLENGERLIB`. Defines a
consumer may add:

| Define | Effect in this lib |
|---|---|
| `CATCHCHALLENGER_CACHE_HPS` (CMake option, ON) | HPS binary datapack cache serializers |
| `CATCHCHALLENGER_NOXML` (CMake option) | No tinyxml2: datapack only from the binary cache |
| `CATCHCHALLENGER_HARDENED` (CMake option) | Extra invariant checks (tests) |
| `CATCHCHALLENGER_BOT_TESTCONNECT` | `parseDatapack()` skips the base datapack parsing |
| `CATCHCHALLENGER_ABORTIFERROR` | Abort on protocol error (debug bots) |
| `BENCHMARKMUTIPLECLIENT` | Precomputed packets for multi-client benchmark |
| `CATCHCHALLENGER_ONLYMAPRENDER` | Removes `Api_protocol` entirely (map viewer tools) |

## Using it: two classes to subclass

### 1. `Api_protocol` — transport + notifications

`Api_protocol` derives from `ProtocolParsingInputOutput` (`general/base/ProtocolParsing.hpp`)
and `MoveOnTheMap`. It owns no socket; you give it one through the transport seam:

| Override | Contract |
|---|---|
| `readFromSocket(data,size)` | `>0` bytes read, `0` nothing pending, `-1` closed/error |
| `writeToSocket(data,size)` | Must accept **all** bytes: a short write disconnects. Buffer the rest yourself and flush when writable (see `CliApiClient::flushOutput()`) |
| `closeSocket()`, `tryDisconnect()` | Close the fd |
| `readForFirstHeader()` | Once the TCP connect is done: set `haveFirstHeader`, move `Stage2`→`Stage3`, call `connectTheExternalSocketInternal()` (sends the protocol header) |
| `defineMaxPlayers()` | Usually `ProtocolParsing::setMaxPlayers()` |
| `getLanguage()`, `haveBeatBot()` | Language code; local beaten-bot lookup |

Then every other pure-virtual is a **notification** (replaces the Qt signals):
`logged()`, `haveCharacter()`, `insert_player()`, `new_chat_text()`,
`newFileBase()`, `tradeRequested()`, `battleAcceptedByOther()`... ~80 in total.
A headless client implements the ones it needs and leaves the rest empty
(`tools/libbot/cli/CliApiClientNotifications.cpp` is a ready empty set).

Feed incoming bytes by calling `parseIncommingData()` when the fd is readable.

### 2. `DatapackClientLoader` — 5 hooks

| Override | Headless implementation |
|---|---|
| `emitdatapackParsed()`, `emitdatapackParsedMainSub()` | Empty (or set a flag) |
| `emitdatapackChecksumError()` | Set an error flag: local hash ≠ server hash |
| `parseTopLib()` | Empty (the GUI builds its tilesets here) |
| `getLanguage()` | `"en"` |

Call `datapackProblem(path)` first (returns the reason a local datapack can't
load), then `parseDatapack(path)` and, once the server sent the main/sub codes,
`parseDatapackMainSub(main,sub)`. Results go to the `CommonDatapack` /
`CommonDatapackServerSpec` singletons (one datapack per process) plus the
`get_*Extra()` getters.

## Connection flow

```
tryLogin(login,pass)           only stores credentials before the header
connect login server
readForFirstHeader()           -> protocol header
  protocol_is_good()           -> login sent automatically
  logged(characters)           -> selectCharacter() or addCharacter()
  connectingOnGameServer()     login socket closed by the lib
socketDisconnectedForReconnect() -> "host:port" of the game server
connect game server, readForFirstHeader() again -> token sent
  haveDatapackMainSubCode()    -> load the datapack (base + main/sub)
setMapNumber(mapCount); have_main_and_sub_datapack_loaded()
  haveCharacter(map,x,y,dir)   player on the map
```

**Never reconnect nor load the datapack inside a notification**: you are inside
the parser. Flag it and do it from your event loop after `parseIncommingData()`
returns (`CliApiClient::pendingWork`). Call `setMapNumber()` **before**
`have_main_and_sub_datapack_loaded()`, else the queued character block is never
replayed.

## Datapack sync

The datapack is checksummed (see `client/CLAUDE.md`). Either it is already on
disk (bots, same machine), or the server streams it: `datapackSize*()`, then
`newFile*()` (raw bytes to write under base/main/sub), `newHttpFile*()` (fetch
from the mirror), `removeFile*()`. A mirror `.tar.zst` is decoded with
`ZstdDecode` then `TarDecode`. Keep the hash coherent with
`DatapackChecksum::doChecksum*()`, otherwise every connect re-downloads.

## Reference implementations

* **Qt-free:** `tools/libbot/cli/` (`CliSocket` non-blocking POSIX socket,
  `CliApiClient`, `CliDatapack`, `CliEventLoop` select() loop over N clients),
  used by `tools/bot-bench`.
* **Qt:** `client/libqtcatchchallenger/Api_protocol_Qt.*`,
  `QtDatapackClientLoader.*`, `Api_client_real*.cpp` (download + mirror).
