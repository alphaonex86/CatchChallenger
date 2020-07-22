# Intro

The client and server is under GPL3. And part of the project:
http://catchchallenger.first-world.info/

## Description
This game is a MMORPG, Lan game and a single player game. It's mix of pokemon for the RPG part, lineage for the crafting/clan/MMORPG, X3 for the commerce/fabric/industry. With mod possibility, and datapack.

It's a pixel art game. The work is concentrated on the gameplay/performance/security/creativity/accessibility. The income is to paid the developping and the artwork. The game is fully open source (GPL3).

The gameplay have strong team part, but remain interresting in single player. Have clear advantage on 3G/wifi/TOR connexion or into the tiers world.

## License
client/tiled/ is extracted version of http://www.mapeditor.org/, https://github.com/bjorn/tiled
libogg and fileopus is extracted from other project
Interface UI is from buyed template (then under copyright)

## Programing

Asynchronous protocol with no influence of internet and server latency. Thread isolation for the important or heavy server task. It can be hosted on ADSL connection.

SGBD for Qt version: Mysql 5+, SQLite, PostgreSQL 9+. SGBD for epoll version (async to high performance): Mysql 5.5+, PostgreSQL 9+.

## Prerequisites

### Postgresql

You need to create this roles:
* catchchallenger-login
* catchchallenger-base

Then:
```sh
su -l postgres
cd server/databases/postgresql
echo 'CREATE DATABASE catchchallenger_login;' | psql postgres postgres
echo 'CREATE DATABASE catchchallenger_base;' | psql postgres postgres
echo 'CREATE DATABASE catchchallenger_common;' | psql postgres postgres
echo 'CREATE DATABASE catchchallenger_server;' | psql postgres postgres
psql catchchallenger_login postgres < catchchallenger-postgresql-login.sql
psql catchchallenger_base postgres < catchchallenger-postgresql-base.sql
psql catchchallenger_common postgres < catchchallenger-postgresql-common.sql
psql catchchallenger_server postgres < catchchallenger-postgresql-server.sql
echo 'GRANT ALL ON ALL TABLES IN SCHEMA public TO "catchchallenger-login";' | psql catchchallenger_login postgres
echo 'GRANT ALL ON ALL TABLES IN SCHEMA public TO "catchchallenger-base";' | psql catchchallenger_base postgres
echo 'GRANT ALL ON ALL TABLES IN SCHEMA public TO "catchchallenger-login";' | psql catchchallenger_common postgres
echo 'GRANT ALL ON ALL TABLES IN SCHEMA public TO "catchchallenger-login";' | psql catchchallenger_server postgres
```

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
**Ubuntu**: apt-get install build-essential gcc automake qt5-qmake qt5-default libzstd-dev zlib1g-dev libssl-dev libpq-dev libqt5sql5-psql libqt5websockets5-dev libqt5sql5-sqlite libqt5sql5-mysql qtdeclarative5-dev qtscript5-dev

**Debian stretch**: apt-get install build-essential gcc automake qt5-qmake libzstd-dev zlib1g-dev libssl-dev libpq-dev qttools5-dev qt5-default libqt5sql5-psql libqt5websockets5-dev libqt5sql5-psql libqt5sql5-sqlite libqt5sql5-mysql qtdeclarative5-dev qtscript5-dev

### Gui server
```sh
cd server/
qmake catchchallenger-server-gui.pro
make
git clone --depth=1 https://github.com/alphaonex86/CatchChallenger-datapack datapack
```

### CLI server
```sh
cd server/
qmake catchchallenger-server-cli.pro
make
git clone --depth=1 https://github.com/alphaonex86/CatchChallenger-datapack datapack
```

### Epoll server (linux only, high performance)
**Ubuntu**: apt-get install libzstd-dev zlib1g-dev libssl-dev libpq-dev

**Debian stretch**: apt-get install build-essential gcc automake qt5-qmake libzstd-dev zlib1g-dev libssl-dev libpq-dev qttools5-dev qt5-default

```sh
cd server/
qmake catchchallenger-server-cli-epoll.pro
make
git clone --depth=1 https://github.com/alphaonex86/CatchChallenger-datapack datapack
chmod a+x catchchallenger-server-cli-epoll
./catchchallenger-server-cli-epoll
```

### Client
**Debian stretch**: apt-get install build-essential gcc automake qt5-qmake libzstd-dev zlib1g-dev libssl-dev libpq-dev qttools5-dev qt5-default qtdeclarative5-dev qtscript5-dev

```sh
cd client/ultimate/
qmake *.pro
make
git clone --depth=1 https://github.com/alphaonex86/CatchChallenger-datapack datapack
chmod a+x catchchallenger
./catchchallenger
```

# Sources
* The sources of the client/server: https://github.com/alphaonex86/CatchChallenger
* The sources of the datapack: https://github.com/alphaonex86/CatchChallenger-datapack
* The sources of the site: https://github.com/alphaonex86/CatchChallenger-site

# Hardware
You need CPU with support of unaligned access on 8/16/32Bits for the server at least (plan to support all CPU)

The server is actually 10-20MB of memory (1MB measured by massif) and 2KB by player
