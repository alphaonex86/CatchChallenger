# catchchallenger_server_base — the "all-in-one game logic" sources
# under server/base, server/crafting, server/fight. Consumers are the
# game servers that hold actual map state: cli-epoll-filedb (Phase 1)
# and game-server-alone (Phase 2). They compile this set with
# CLASS_ALLINONESERVER and CLASS_ONLYGAMESERVER respectively, so the
# lib stays INTERFACE — sources attach to whichever exec consumes it
# and get compiled with that exec's PRIVATE defines.
#
# login / master / gateway do NOT link this lib; they have small
# self-contained source lists and live under their own subdirs.

add_library(catchchallenger_server_base INTERFACE)

target_sources(catchchallenger_server_base INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/base/GlobalServerData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/PlayerUpdaterBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServer2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServer3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoad.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadProfile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadDatapack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadOther.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadMapAfterDB.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadSQL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLoadSQLNotEpoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerUnload.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerMasterLoadDictionary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerMasterSendDatapack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BaseServer/BaseServerLogin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerMove.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerReputation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerFightManage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerWarehouse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerDatabaseSync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerShop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerClan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerCity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerWithoutSender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerQuest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientEvents/LocalClientHandlerTrade.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLocalBroadcast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/Client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientStaticVar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoad.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadLogin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadLogin2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadMirror.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadSelectChar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadSelectCharCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadSelectCharServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientLoad/ClientHeavyLoadSelectCharFinal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientNetworkWrite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientNetworkRead.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientNetworkReadMessage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientNetworkReadQuery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientNetworkReadWithoutSender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ClientBroadCast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MapServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/BroadCastWithoutSender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MapManagement/ClientWithMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MapManagement/ClientMapManagement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MapManagement/MapVisibilityAlgorithm_WithoutSender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MapManagement/MapBasicMove.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/DictionaryLogin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/DictionaryServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/TimeRangeEventScanBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/StringWithReplacement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/NormalServerGlobal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crafting/BaseServerCrafting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crafting/ClientLocalBroadcastCrafting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crafting/LocalClientHandlerCrafting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crafting/ClientHeavyLoadCrafting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/LocalClientHandlerFight.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/LocalClientHandlerFightSkill.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/LocalClientHandlerFightBuff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/LocalClientHandlerFightWild.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/LocalClientHandlerFightDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/LocalClientHandlerFightBattle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/BaseServerFight.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/ClientHeavyLoadFight.cpp
)

# DB plumbing has two layers:
#   _db_minimal — DatabaseBase + DatabaseFunction + SqlFunction.
#     Used by master (which does its own bespoke queries, not via the
#     PreparedDBQuery facade).
#   _sql — adds PreparedDBQueryLogin/Common/Server + PreparedStatementUnit.
#     The PreparedDBQuery* files reference CLASS_LOGIN/CLIENT/
#     ALLINONESERVER/ONLYGAMESERVER/QT and don't compile against
#     CLASS_MASTER, so master must not pull this layer.
add_library(catchchallenger_server_db_minimal INTERFACE)
target_sources(catchchallenger_server_db_minimal INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/base/SqlFunction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/DatabaseBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/DatabaseFunction.cpp
)
target_include_directories(catchchallenger_server_db_minimal INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
)

add_library(catchchallenger_server_sql INTERFACE)
target_link_libraries(catchchallenger_server_sql INTERFACE catchchallenger_server_db_minimal)
target_sources(catchchallenger_server_sql INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/base/PreparedDBQueryLogin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/PreparedDBQueryCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/PreparedDBQueryServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/PreparedStatementUnit.cpp
)

if(NOT CATCHCHALLENGER_NOXML)
    target_sources(catchchallenger_server_base INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/base/TinyXMLSettings.cpp
    )
endif()

target_include_directories(catchchallenger_server_base INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
)

# Defines from catchchallenger-serverheader.pri.
# CATCHCHALLENGER_SERVER and CATCHCHALLENGER_CLASS_* live on the
# consuming executable, not this lib — single-player Qt clients use
# this game-logic with CATCHCHALLENGER_CLASS_QT (and Qt-based IO via
# server/qt/), not epoll.
target_compile_definitions(catchchallenger_server_base INTERFACE
    CATCHCHALLENGER_DYNAMIC_MAP_LIST
)
# QT_NO_EMIT is on per-server execs only (epoll-based servers don't
# use Qt signals). Single-player Qt clients DO use `emit` so they
# must not inherit it.
# DB choice (CATCHCHALLENGER_DB_FILE / DB_POSTGRESQL / DB_MYSQL /
# DB_BLACKHOLE) is per-executable, not propagated through this lib —
# game-server-alone uses a different DB mode than cli-epoll-filedb.
if(CATCHCHALLENGER_HARDENED)
    target_compile_definitions(catchchallenger_server_base INTERFACE CATCHCHALLENGER_HARDENED)
endif()
if(CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK)
    target_compile_definitions(catchchallenger_server_base INTERFACE CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK)
endif()
if(CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR)
    target_compile_definitions(catchchallenger_server_base INTERFACE CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR)
endif()

# Phase 1 + Phase 2 server executables
if(CATCHCHALLENGER_BUILD_EPOLL_FILEDB)
    add_subdirectory(epoll)
endif()

# Phase 2 servers — toggleable individually.
option(CATCHCHALLENGER_BUILD_LOGIN  "Build catchchallenger-server-login (Phase 2)"  OFF)
option(CATCHCHALLENGER_BUILD_MASTER "Build catchchallenger-server-master (Phase 2)" OFF)
option(CATCHCHALLENGER_BUILD_GATEWAY "Build catchchallenger-gateway (Phase 2)"      OFF)
option(CATCHCHALLENGER_BUILD_GAME_SERVER_ALONE
       "Build catchchallenger-game-server-alone (Phase 2)" OFF)

if(CATCHCHALLENGER_BUILD_LOGIN)
    add_subdirectory(login)
endif()
if(CATCHCHALLENGER_BUILD_MASTER)
    add_subdirectory(master)
endif()
if(CATCHCHALLENGER_BUILD_GATEWAY)
    add_subdirectory(gateway)
endif()
if(CATCHCHALLENGER_BUILD_GAME_SERVER_ALONE)
    add_subdirectory(game-server-alone)
endif()

# filedb-converter under server/epoll/ — Phase 4 tool, lives there
# in the source tree so leave the CMakeLists.txt next to its sources.
if(CATCHCHALLENGER_BUILD_TOOL_FILEDB_CONVERTER)
    add_subdirectory(epoll/filedb-converter)
endif()

# Qt in-process server lib for single-player mode (clients pull this
# in when single-player is on). Always available — clients opt in.
add_subdirectory(qt)
