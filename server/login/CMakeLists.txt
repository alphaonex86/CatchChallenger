# server/login/CMakeLists.txt — catchchallenger-server-login binary.
# Self-contained, one binary. CATCHCHALLENGER_CLASS_LOGIN +
# CATCHCHALLENGER_DB_POSTGRESQL + DB_PREPAREDSTATEMENT.
# Doesn't share catchchallenger_server_base (no game-logic sources);
# pulls only a small subset of server/base + a few server/cli files.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_server_login LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

find_package(Threads REQUIRED)
# DB backend: PostgreSQL (default) or MySQL/MariaDB. See
# server/master/CMakeLists.txt for the same selector logic — login
# uses the switchable `EventLoopDb` alias from EventLoopDatabase.hpp.
if(CATCHCHALLENGER_DB_MYSQL AND CATCHCHALLENGER_DB_POSTGRESQL)
    message(FATAL_ERROR
        "catchchallenger-server-login: both CATCHCHALLENGER_DB_MYSQL and CATCHCHALLENGER_DB_POSTGRESQL are ON; pick one.")
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

set(_login_sources
    main-unix-login-slave.cpp
    EventLoopClientLoginSlave.cpp
    EventLoopServerLoginSlave.cpp
    EventLoopClientLoginSlaveStaticVar.cpp
    EventLoopClientLoginSlaveHeavyLoad.cpp
    LinkToMaster.cpp
    LinkToMasterStaticVar.cpp
    LinkToMasterProtocolParsing.cpp
    LinkToMasterProtocolParsingMessage.cpp
    LinkToMasterProtocolParsingReply.cpp
    EventLoopClientLoginSlaveProtocolParsing.cpp
    EventLoopClientLoginSlaveWrite.cpp
    CharactersGroupForLogin.cpp
    CharactersGroupClient.cpp
    LinkToGameServerStaticVar.cpp
    LinkToGameServerProtocolParsing.cpp
    LinkToGameServer.cpp
    TimerDdos.cpp
    TimerDetectTimeout.cpp
    # unix/ files reused by all servers
    ../cli/EventLoop.cpp
    ../cli/EventLoopGenericServer.cpp
    ../cli/EventLoopClient.cpp
    ../cli/SocketUtil.cpp
    ${_db_source}
    ../cli/EventLoopClientToServer.cpp
    ../cli/EventLoopTimer.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../unix
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
)

target_link_libraries(catchchallenger-server-login PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
    catchchallenger_server_sql
    Threads::Threads
    ZLIB::ZLIB
    ${_db_lib}
)

target_compile_definitions(catchchallenger-server-login PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_LOGIN
    ${_db_define}
    CATCHCHALLENGER_DB_PREPAREDSTATEMENT
    CATCHCHALLENGER_DYNAMIC_MAP_LIST
)

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-server-login PRIVATE ${ZSTD_LIBRARY})
endif()

# io_uring is MANDATORY for this binary (per server/CLAUDE.md).
if(NOT LIBURING_LIBRARY)
    find_library(LIBURING_LIBRARY NAMES uring)
    find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
endif()
if(NOT LIBURING_LIBRARY)
    message(FATAL_ERROR
        "catchchallenger-server-login requires liburing (mandatory io_uring backend).")
endif()
target_compile_definitions(catchchallenger-server-login PRIVATE CATCHCHALLENGER_IO_URING)
target_include_directories(catchchallenger-server-login PRIVATE ${LIBURING_INCLUDE_DIR})
target_link_libraries(catchchallenger-server-login PRIVATE ${LIBURING_LIBRARY})

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(catchchallenger-server-login PRIVATE -fuse-ld=${CC_LINKER})
endif()

# Test-only kernel/user-space wakeup-loop detector. When the harness
# expects no traffic burst (no DDOS fuzz, no large-connexion stress,
# no event flooding) it sets this and any process delivering >1000
# events/s through EventLoop::wait() aborts so the wall-watchdog
# captures a backtrace. See test/CLAUDE.md "Per-script wall limit".
option(CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
       "Abort if EventLoop::wait() delivers >1000 events/s (testing only)"
       OFF)
if(CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE)
    target_compile_definitions(catchchallenger-server-login PRIVATE
        CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE)
endif()
