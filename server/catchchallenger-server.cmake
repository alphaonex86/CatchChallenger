# server/catchchallenger-server.cmake — mirrors catchchallenger-server.pri
#                                       + catchchallenger-serverheader.pri.
#
# INTERFACE libraries for the shared server game-logic sources.
# Consumed by server/, server/epoll/, server/game-server-alone/,
# server/login/, server/master/, server/gateway/, AND single-player
# Qt clients (qtopengl + qtcpu800x600 with SOLO).
#
# Three layered INTERFACE libs:
#   catchchallenger_server_base — the all-in-one game logic (BaseServer,
#       ClientEvents/, Crafting/, Fight/, MapManagement/, Dictionaries,
#       NormalServerGlobal). Used by all "game" servers.
#   catchchallenger_server_db_minimal — DatabaseBase + DatabaseFunction +
#       SqlFunction. Used by master server (which does its own bespoke
#       queries, not via the PreparedDBQuery facade).
#   catchchallenger_server_sql — adds PreparedDBQueryLogin/Common/Server +
#       PreparedStatementUnit on top of db_minimal.
#
# Each consuming executable picks the CLASS macro (CATCHCHALLENGER_CLASS_QT
# vs ALLINONESERVER vs ONLYGAMESERVER vs LOGIN vs MASTER vs GATEWAY) and
# DB macro (DB_FILE / DB_SQLITE / DB_MYSQL / …) on itself; INTERFACE libs
# let the same source files compile differently for each consumer.

if(NOT TARGET catchchallenger_server_base)
    add_library(catchchallenger_server_base INTERFACE)

    target_sources(catchchallenger_server_base INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/base/GlobalServerData.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/PlayerUpdaterBase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServer2.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServer3.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerSettings.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoad.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadProfile.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadDatapack.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadOther.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadMapAfterDB.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadSQL.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLoadSQLNotEpoll.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerUnload.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerMasterLoadDictionary.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerMasterSendDatapack.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BaseServer/BaseServerLogin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerMove.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerReputation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerFightManage.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerObject.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerWarehouse.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerCommand.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerDatabaseSync.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerShop.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerClan.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerCity.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerWithoutSender.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerQuest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientEvents/LocalClientHandlerTrade.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLocalBroadcast.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/Client.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientList.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientStaticVar.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoad.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadLogin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadLogin2.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadMirror.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadSelectChar.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadSelectCharCommon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadSelectCharServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientLoad/ClientHeavyLoadSelectCharFinal.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientNetworkWrite.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientNetworkRead.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientNetworkReadMessage.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientNetworkReadQuery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientNetworkReadWithoutSender.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ClientBroadCast.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MapServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/BroadCastWithoutSender.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MapManagement/ClientWithMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MapManagement/ClientMapManagement.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MapManagement/MapVisibilityAlgorithm_WithoutSender.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MapManagement/MapVisibilityAlgorithm.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MapManagement/MapBasicMove.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/DictionaryLogin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/DictionaryServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/TimeRangeEventScanBase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/StringWithReplacement.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/NormalServerGlobal.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crafting/BaseServerCrafting.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crafting/ClientLocalBroadcastCrafting.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crafting/LocalClientHandlerCrafting.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crafting/ClientHeavyLoadCrafting.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/LocalClientHandlerFight.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/LocalClientHandlerFightSkill.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/LocalClientHandlerFightBuff.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/LocalClientHandlerFightWild.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/LocalClientHandlerFightDatabase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/LocalClientHandlerFightBattle.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/BaseServerFight.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/ClientHeavyLoadFight.cpp
    )

    if(NOT CATCHCHALLENGER_NOXML)
        target_sources(catchchallenger_server_base INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/base/TinyXMLSettings.cpp
        )
    endif()

    target_include_directories(catchchallenger_server_base INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/base
    )

    target_compile_definitions(catchchallenger_server_base INTERFACE
        CATCHCHALLENGER_DYNAMIC_MAP_LIST
    )
    if(CATCHCHALLENGER_HARDENED)
        target_compile_definitions(catchchallenger_server_base INTERFACE CATCHCHALLENGER_HARDENED)
    endif()
    if(CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK)
        target_compile_definitions(catchchallenger_server_base INTERFACE CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK)
    endif()
    if(CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR)
        target_compile_definitions(catchchallenger_server_base INTERFACE CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR)
    endif()
endif()

if(NOT TARGET catchchallenger_server_db_minimal)
    add_library(catchchallenger_server_db_minimal INTERFACE)
    target_sources(catchchallenger_server_db_minimal INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/base/SqlFunction.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/DatabaseBase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/DatabaseFunction.cpp
    )
    target_include_directories(catchchallenger_server_db_minimal INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/base
    )
endif()

if(NOT TARGET catchchallenger_server_sql)
    add_library(catchchallenger_server_sql INTERFACE)
    target_link_libraries(catchchallenger_server_sql INTERFACE catchchallenger_server_db_minimal)
    target_sources(catchchallenger_server_sql INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/base/PreparedDBQueryLogin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/PreparedDBQueryCommon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/PreparedDBQueryServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/PreparedStatementUnit.cpp
    )
endif()
