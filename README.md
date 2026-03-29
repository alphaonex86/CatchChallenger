# Intro

http://catchchallenger.first-world.info/

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

Use **C++11**

Dependency:
* zlib (can be disabled but it's for tiled map editor)
* Client
  * Qt openssl enabled to have QSslSocket or QtWebSocket
* Server
  * Qt if generic server
  * db
  * db driver (mysql, sqlite for game solo, postgresql, depands of you choice)
* Gateway
  * curl to download datapack via http
  
### Linux distro
**Ubuntu**: apt-get install build-essential gcc automake qt5-qmake libzstd-dev zlib1g-dev libssl-dev libpq-dev libqt5sql5-psql libqt5websockets5-dev libqt5sql5-sqlite libqt5sql5-mysql qtdeclarative5-dev qtscript5-dev

**Debian stretch**: apt-get install build-essential gcc automake qt5-qmake libzstd-dev zlib1g-dev libssl-dev libpq-dev qttools5-dev libqt5sql5-psql libqt5websockets5-dev libqt5sql5-psql libqt5sql5-sqlite libqt5sql5-mysql qtdeclarative5-dev qtscript5-dev

### Client
**Debian stretch**: apt-get install build-essential gcc automake qt5-qmake libzstd-dev zlib1g-dev libssl-dev libpq-dev qttools5-dev qtdeclarative5-dev qtscript5-dev

```sh
cd client/
qmake *.pro
make
git clone --depth=1 https://github.com/alphaonex86/CatchChallenger-datapack datapack
chmod a+x catchchallenger
./catchchallenger
```

# Sources
* The sources of the client/server: https://github.com/alphaonex86/CatchChallenger
* The sources of the datapack: https://github.com/alphaonex86/CatchChallenger-datapack
