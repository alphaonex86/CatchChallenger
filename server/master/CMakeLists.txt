# server/master/CMakeLists.txt — catchchallenger-server-master binary.
# Self-contained, one binary. CATCHCHALLENGER_CLASS_MASTER +
# CATCHCHALLENGER_DB_POSTGRESQL + DB_PREPAREDSTATEMENT +
# CATCHCHALLENGER_SERVER_NO_COMPRESSION + NOGAMESERVER.
# Master has its own self-contained sources; doesn't share
# catchchallenger_server_base.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_server_master LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

find_package(Threads REQUIRED)
# DB backend: PostgreSQL (default) or MySQL/MariaDB. Cluster source
# uses the switchable `EventLoopDb` alias from EventLoopDatabase.hpp,
# so the only configure-time choice is which backend .cpp + client
# library to wire in. Exactly one of CATCHCHALLENGER_DB_POSTGRESQL /
# CATCHCHALLENGER_DB_MYSQL must be ON (PG is the default).
if(CATCHCHALLENGER_DB_MYSQL AND CATCHCHALLENGER_DB_POSTGRESQL)
    message(FATAL_ERROR
        "catchchallenger-server-master: both CATCHCHALLENGER_DB_MYSQL and CATCHCHALLENGER_DB_POSTGRESQL are ON; pick one.")
endif()
if(NOT CATCHCHALLENGER_DB_MYSQL)
    set(CATCHCHALLENGER_DB_POSTGRESQL ON)
endif()
if(CATCHCHALLENGER_DB_MYSQL)
    find_library(_db_lib NAMES mariadb mysqlclient REQUIRED)
    set(_db_source ../cli/db/EventLoopMySQL.cpp)
    set(_db_define CATCHCHALLENGER_DB_MYSQL)
else()
    find_library(_db_lib NAMES pq REQUIRED)
    set(_db_source ../cli/db/EventLoopPostgresql.cpp)
    set(_db_define CATCHCHALLENGER_DB_POSTGRESQL)
endif()

set(_master_sources
    main-unix-login-master.cpp
    EventLoopClientLoginMaster.cpp
    EventLoopServerLoginMaster.cpp
    EventLoopClientLoginMasterStaticVar.cpp
    EventLoopClientLoginMasterProtocolParsing.cpp
    CharactersGroup.cpp
    PlayerUpdaterToLogin.cpp
    PurgeTheLockedAccount.cpp
    CheckTimeoutGameServer.cpp
    AutomaticPingSend.cpp
    TimeRangeEvent.cpp
    # unix/ files
    ../cli/EventLoopTimer.cpp
    ../cli/EventLoop.cpp
    ../cli/EventLoopGenericServer.cpp
    ../cli/EventLoopClient.cpp
    ../cli/SocketUtil.cpp
    ${_db_source}
    # server/base subset (SQL plumbing comes via catchchallenger_server_sql).
    ../base/BaseServer/BaseServerMasterLoadDictionary.cpp
    ../base/BaseServer/BaseServerMasterSendDatapack.cpp
    ../base/TinyXMLSettings.cpp
    # general/ datapack + fight loader subset.
    # master.pro lists CommonDatapack.cpp explicitly AND
    # DatapackGeneralLoaderMap.cpp; qmake silently drops the former
    # because the latter #include's it at file scope. We do the same.
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoader.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderCrafting.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderMap.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderMonsterDrop.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderPlant.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp
    ../../general/base/DatapackGeneralLoader/DatapackGeneralLoaderReputation.cpp
    ../../general/fight/FightLoader.cpp
    ../../general/fight/FightLoaderBuff.cpp
    ../../general/fight/FightLoaderMonster.cpp
    ../../general/fight/FightLoaderSkill.cpp
    ../../general/fight/CommonFightEngineBase.cpp
)

add_executable(catchchallenger-server-master ${_master_sources})

target_include_directories(catchchallenger-server-master PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../unix
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
)

target_link_libraries(catchchallenger-server-master PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
    catchchallenger_server_db_minimal
    Threads::Threads
    ZLIB::ZLIB
    ${_db_lib}
)

target_compile_definitions(catchchallenger-server-master PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_SERVER_NO_COMPRESSION
    # No CATCHCHALLENGER_SOLO: master coordinates the cluster but does
    # not run game logic itself.
    CATCHCHALLENGER_CLASS_MASTER
    ${_db_define}
    CATCHCHALLENGER_DB_PREPAREDSTATEMENT
    CATCHCHALLENGER_DYNAMIC_MAP_LIST
)

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-server-master PRIVATE ${ZSTD_LIBRARY})
endif()

# io_uring is MANDATORY for this binary (per server/CLAUDE.md).
if(NOT LIBURING_LIBRARY)
    find_library(LIBURING_LIBRARY NAMES uring)
    find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
endif()
if(NOT LIBURING_LIBRARY)
    message(FATAL_ERROR
        "catchchallenger-server-master requires liburing (mandatory io_uring backend).")
endif()
target_compile_definitions(catchchallenger-server-master PRIVATE CATCHCHALLENGER_IO_URING)
target_include_directories(catchchallenger-server-master PRIVATE ${LIBURING_INCLUDE_DIR})
target_link_libraries(catchchallenger-server-master PRIVATE ${LIBURING_LIBRARY})

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-server-master PRIVATE -fuse-ld=mold)
endif()
