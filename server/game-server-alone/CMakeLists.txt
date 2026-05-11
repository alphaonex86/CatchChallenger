# server/game-server-alone/CMakeLists.txt — catchchallenger-game-server-alone binary.
# Self-contained, one binary. Same big source set as the cli-epoll exec
# under server/cli/, but compiled with CATCHCHALLENGER_CLASS_ONLYGAMESERVER
# + DATAPACK_ONLYBYMIRROR (instead of _ALLINONESERVER) AND adds
# LinkToMaster + PlayerUpdaterToMaster + TimerPurgeTokenAuthList sources.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_game_server_alone LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

find_package(Threads REQUIRED)
# DB backend: PostgreSQL (default) or MySQL/MariaDB. See
# server/master/CMakeLists.txt for the same selector logic.
if(CATCHCHALLENGER_DB_MYSQL AND CATCHCHALLENGER_DB_POSTGRESQL)
    message(FATAL_ERROR
        "catchchallenger-game-server-alone: both CATCHCHALLENGER_DB_MYSQL and CATCHCHALLENGER_DB_POSTGRESQL are ON; pick one.")
endif()
if(NOT CATCHCHALLENGER_DB_MYSQL)
    set(CATCHCHALLENGER_DB_POSTGRESQL ON)
endif()
if(CATCHCHALLENGER_DB_MYSQL)
    find_library(_db_lib NAMES mariadb mysqlclient REQUIRED)
    set(_db_source ${CC_REPO_ROOT}/server/cli/db/EventLoopMySQL.cpp)
    set(_db_define CATCHCHALLENGER_DB_MYSQL)
else()
    find_library(_db_lib NAMES pq REQUIRED)
    set(_db_source ${CC_REPO_ROOT}/server/cli/db/EventLoopPostgresql.cpp)
    set(_db_define CATCHCHALLENGER_DB_POSTGRESQL)
endif()

# Inlined source list — server/cli/ also references these, but each
# binary is its own CMake project, so duplicate the list rather than
# share a PARENT_SCOPE variable.
set(_unix_sources
    ${CC_REPO_ROOT}/server/cli/SocketUtil.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoopClient.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoopClientList.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoopServer.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoop.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoopTimer.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoopGenericServer.cpp
    ${CC_REPO_ROOT}/server/cli/BaseServerEventLoop.cpp
    ${CC_REPO_ROOT}/server/cli/ServerPrivateVariablesEventLoop.cpp
    ${CC_REPO_ROOT}/server/cli/ClientWithMapEventLoop.cpp
    ${CC_REPO_ROOT}/server/cli/main-unix.cpp
    ${CC_REPO_ROOT}/server/cli/main-unix2.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimerCityCapture.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimerSendInsertMoveRemove.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimerPositionSync.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimerDdos.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimerEvents.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimeRangeEvent.cpp
    ${CC_REPO_ROOT}/server/cli/timer/PlayerUpdaterEventLoop.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimeRangeEventScan.cpp
)

set(_extra_sources
    LinkToMaster.cpp
    LinkToMasterProtocolParsing.cpp
    LinkToMasterStaticVar.cpp
    timer/PlayerUpdaterToMaster.cpp
    ${CC_REPO_ROOT}/server/cli/timer/TimerPurgeTokenAuthList.cpp
    ${_db_source}
)

add_executable(catchchallenger-game-server-alone
    ${_unix_sources}
    ${_extra_sources}
)

target_include_directories(catchchallenger-game-server-alone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}/server/cli
)

target_link_libraries(catchchallenger-game-server-alone PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    catchchallenger_server_base
    catchchallenger_server_sql
    Threads::Threads
    ZLIB::ZLIB
    ${_db_lib}
)

target_compile_definitions(catchchallenger-game-server-alone PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    ${_db_define}
    CATCHCHALLENGER_DB_PREPAREDSTATEMENT
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger-game-server-alone PRIVATE CATCHCHALLENGER_CACHE_HPS)
endif()

# io_uring is MANDATORY for this binary (per server/CLAUDE.md).
if(NOT LIBURING_LIBRARY)
    find_library(LIBURING_LIBRARY NAMES uring)
    find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
endif()
if(NOT LIBURING_LIBRARY)
    message(FATAL_ERROR
        "catchchallenger-game-server-alone requires liburing (mandatory io_uring backend).")
endif()
target_compile_definitions(catchchallenger-game-server-alone PRIVATE CATCHCHALLENGER_IO_URING)
target_include_directories(catchchallenger-game-server-alone PRIVATE ${LIBURING_INCLUDE_DIR})
target_link_libraries(catchchallenger-game-server-alone PRIVATE ${LIBURING_LIBRARY})

target_link_libraries(catchchallenger-game-server-alone PRIVATE ${ZSTD_LIBRARY})

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-game-server-alone PRIVATE -fuse-ld=mold)
endif()
