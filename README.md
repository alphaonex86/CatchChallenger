# Intro

https://catchchallenger.herman-brule.com/

![structure](structure.png)

This game is a MMORPG, Lan game and a single player game. It's mix of pokemon for the RPG part, crafting/clan/TvT, industry. Modular datapck.

## License
client/tiled/ is extracted version of http://www.mapeditor.org/, https://github.com/bjorn/tiled
libogg and fileopus is extracted from other project
Interface UI is from bought template (then under copyright)

# Target
- minimal dependency (searcg dependency hell, bug/security problem in dependency)
- no bloatware (no stupid features, no features used for only 1 person if imply lot of code or dangerous code, no unrelated features)
- async, support high latency, very low bandwidth (tipical of TOR/I2P)
- send datapack by internal protocol (overall compression to group similar part into multiple file) and http mirror, and when datapack is downloaded by client can be used to mount new server with same datapack
- most processing is cliend side

## Compiling

Use **C++11** to **C++23**, C++23 is preferred to use std::flat_map on read only data of datapack where apply (lower RAM and improve speed)

Dependency:
* zlib (can be disabled but it's for tiled map editor). zstd
* blake3 to have maximal performance and be future proof to replace sha224
* xxh to have light check for what file have changed, eg truncated to 32Bits datapack file list
* Client
  * Qt openssl enabled to have QSslSocket or QtWebSocket, QThread
* Server
  * Qt if generic server + Qt SQL drivers (mysql, sqlite for game solo, postgresql)
  * libpq (postgresql in async) or raw binary file
* Gateway
  * curl to download datapack via http

The project now uses **CMake** (qmake `.pro` files are gone). There is no
root `CMakeLists.txt`: each binary has its own self-contained
`CMakeLists.txt`, so you configure the subdir of the binary you want.
Builds with vanilla `cmake` out of the box (ninja/mold/ccache are picked
up automatically when present, plain make otherwise).

### Client

```sh
cmake -S client -B build/client
cmake --build build/client
mkdir build/client/datapack
git clone --depth=1 https://github.com/alphaonex86/CatchChallenger-datapack build/client/datapack/internal
cd build/client
./catchchallenger
```

### Server

```sh
cmake -S server -B build/server      # catchchallenger-server-gui (Qt admin)
cmake --build build/server
```

Other server variants live in their own sibling subdirs, built the same
way: `server/cli` (epoll game server), `server/login`, `server/master`,
`server/gateway`, `server/game-server-alone`.

# Sources
* The sources of the client/server: https://github.com/alphaonex86/CatchChallenger
* The sources of the datapack: https://github.com/alphaonex86/CatchChallenger-datapack
