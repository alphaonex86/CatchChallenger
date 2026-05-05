# server/login/CMakeLists.txt — catchchallenger-server-login binary.
# Self-contained, one binary. CATCHCHALLENGER_CLASS_LOGIN +
# CATCHCHALLENGER_DB_POSTGRESQL + DB_PREPAREDSTATEMENT.
# Doesn't share catchchallenger_server_base (no game-logic sources);
# pulls only a small subset of server/base + a few server/epoll files.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_server_login LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

find_package(Threads REQUIRED)
find_library(LIBPQ_LIBRARY NAMES pq REQUIRED)

set(_login_sources
    main-epoll-login-slave.cpp
    EpollClientLoginSlave.cpp
    EpollServerLoginSlave.cpp
    EpollClientLoginSlaveStaticVar.cpp
    EpollClientLoginSlaveHeavyLoad.cpp
    LinkToMaster.cpp
    LinkToMasterStaticVar.cpp
    LinkToMasterProtocolParsing.cpp
    LinkToMasterProtocolParsingMessage.cpp
    LinkToMasterProtocolParsingReply.cpp
    EpollClientLoginSlaveProtocolParsing.cpp
    EpollClientLoginSlaveWrite.cpp
    CharactersGroupForLogin.cpp
    CharactersGroupClient.cpp
    LinkToGameServerStaticVar.cpp
    LinkToGameServerProtocolParsing.cpp
    LinkToGameServer.cpp
    TimerDdos.cpp
    TimerDetectTimeout.cpp
    # epoll/ files reused by all servers
    ../epoll/Epoll.cpp
    ../epoll/EpollGenericServer.cpp
    ../epoll/EpollClient.cpp
    ../epoll/EpollSocket.cpp
    ../epoll/db/EpollPostgresql.cpp
    ../epoll/EpollClientToServer.cpp
    ../epoll/EpollTimer.cpp
    # Small subset of server/base/ specific to login. The SQL plumbing
    # (DatabaseBase, PreparedDBQuery*, etc.) is pulled via the
    # catchchallenger_server_sql INTERFACE lib in target_link_libraries.
    ../base/GlobalServerData.cpp
    ../base/BaseServer/BaseServerLogin.cpp
    ../base/DictionaryLogin.cpp
    ../base/TinyXMLSettings.cpp
    ../base/StringWithReplacement.cpp
)

add_executable(catchchallenger-server-login ${_login_sources})

target_include_directories(catchchallenger-server-login PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../epoll
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
)

target_link_libraries(catchchallenger-server-login PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
    catchchallenger_server_sql
    Threads::Threads
    ZLIB::ZLIB
    ${LIBPQ_LIBRARY}
)

target_compile_definitions(catchchallenger-server-login PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_LOGIN
    CATCHCHALLENGER_DB_POSTGRESQL
    CATCHCHALLENGER_DB_PREPAREDSTATEMENT
    CATCHCHALLENGER_DYNAMIC_MAP_LIST
)

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-server-login PRIVATE ${ZSTD_LIBRARY})
endif()

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-server-login PRIVATE -fuse-ld=mold)
endif()
