# server/game-server-alone/CMakeLists.txt — catchchallenger-game-server-alone binary.
# Self-contained, one binary. Same big source set as the cli-epoll exec
# under server/epoll/, but compiled with CATCHCHALLENGER_CLASS_ONLYGAMESERVER
# + DATAPACK_ONLYBYMIRROR (instead of _ALLINONESERVER) AND adds
# LinkToMaster + PlayerUpdaterToMaster + TimerPurgeTokenAuthList sources.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_game_server_alone LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

find_package(Threads REQUIRED)
find_library(LIBPQ_LIBRARY NAMES pq REQUIRED)

# Inlined source list — server/epoll/ also references these, but each
# binary is its own CMake project, so duplicate the list rather than
# share a PARENT_SCOPE variable.
set(_epoll_sources
    ${CC_REPO_ROOT}/server/epoll/EpollSocket.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollClient.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollClientList.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollServer.cpp
    ${CC_REPO_ROOT}/server/epoll/Epoll.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollTimer.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollGenericServer.cpp
    ${CC_REPO_ROOT}/server/epoll/BaseServerEpoll.cpp
    ${CC_REPO_ROOT}/server/epoll/ServerPrivateVariablesEpoll.cpp
    ${CC_REPO_ROOT}/server/epoll/ClientWithMapEpoll.cpp
    ${CC_REPO_ROOT}/server/epoll/main-epoll.cpp
    ${CC_REPO_ROOT}/server/epoll/main-epoll2.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimerCityCapture.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimerSendInsertMoveRemove.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimerPositionSync.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimerDdos.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimerEvents.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimeRangeEvent.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/PlayerUpdaterEpoll.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimeRangeEventScan.cpp
)

set(_extra_sources
    LinkToMaster.cpp
    LinkToMasterProtocolParsing.cpp
    LinkToMasterStaticVar.cpp
    timer/PlayerUpdaterToMaster.cpp
    ${CC_REPO_ROOT}/server/epoll/timer/TimerPurgeTokenAuthList.cpp
    # game-server-alone always uses PostgreSQL.
    ${CC_REPO_ROOT}/server/epoll/db/EpollPostgresql.cpp
)

add_executable(catchchallenger-game-server-alone
    ${_epoll_sources}
    ${_extra_sources}
)

target_include_directories(catchchallenger-game-server-alone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}/server/epoll
)

target_link_libraries(catchchallenger-game-server-alone PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    catchchallenger_server_base
    catchchallenger_server_sql
    Threads::Threads
    ZLIB::ZLIB
    ${LIBPQ_LIBRARY}
)

target_compile_definitions(catchchallenger-game-server-alone PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    CATCHCHALLENGER_DB_POSTGRESQL
    CATCHCHALLENGER_DB_PREPAREDSTATEMENT
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger-game-server-alone PRIVATE CATCHCHALLENGER_CACHE_HPS)
endif()

if(CATCHCHALLENGER_POLL)
    target_compile_definitions(catchchallenger-game-server-alone PRIVATE CATCHCHALLENGER_POLL)
endif()
if(CATCHCHALLENGER_IO_URING)
    target_compile_definitions(catchchallenger-game-server-alone PRIVATE CATCHCHALLENGER_IO_URING)
    target_include_directories(catchchallenger-game-server-alone PRIVATE ${LIBURING_INCLUDE_DIR})
    target_link_libraries(catchchallenger-game-server-alone PRIVATE ${LIBURING_LIBRARY})
endif()

target_link_libraries(catchchallenger-game-server-alone PRIVATE ${ZSTD_LIBRARY})

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-game-server-alone PRIVATE -fuse-ld=mold)
endif()
