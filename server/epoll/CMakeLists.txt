# server/epoll/CMakeLists.txt — catchchallenger-server-cli-epoll binary.
#
# Self-contained: opening this file in Qt Creator (or
# `cmake -S server/epoll -B build/server-epoll`) configures and builds
# exactly the epoll game server. Pulls in catchchallenger_general +
# catchchallenger_server_base via add_subdirectory of the lib subdirs.
#
# server/epoll/filedb-converter/ has its own CMakeLists.txt that
# produces the filedb-converter binary; this CMakeLists.txt does NOT
# add_subdirectory(filedb-converter) — open that one directly to build it.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_server_cli_epoll LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Threads REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

set(_filedb_sources
    EpollSocket.cpp
    EpollClient.cpp
    EpollClientList.cpp
    EpollServer.cpp
    Epoll.cpp
    EpollTimer.cpp
    EpollGenericServer.cpp
    BaseServerEpoll.cpp
    ServerPrivateVariablesEpoll.cpp
    ClientWithMapEpoll.cpp
    main-epoll.cpp
    main-epoll2.cpp
    timer/TimerCityCapture.cpp
    timer/TimerSendInsertMoveRemove.cpp
    timer/TimerPositionSync.cpp
    timer/TimerDdos.cpp
    timer/TimerEvents.cpp
    timer/TimeRangeEvent.cpp
    timer/PlayerUpdaterEpoll.cpp
    timer/TimeRangeEventScan.cpp
)

add_executable(catchchallenger-server-cli-epoll ${_filedb_sources})

target_include_directories(catchchallenger-server-cli-epoll PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(catchchallenger-server-cli-epoll PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    catchchallenger_server_base
    Threads::Threads
    ZLIB::ZLIB
)

# IO-backend + class-mode + DB-mode are per-executable.
target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_ALLINONESERVER
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_CACHE_HPS)
endif()
# DB mode: default to file-based when no SQL backend was requested.
# server/base headers (PreparedDBQuery.hpp, ServerStructures.hpp) `#error`
# if no DB mode is picked. Standalone open in Qt Creator with no -D flags
# should still produce a compilable binary; pin DB_FILE as the default.
if(NOT CATCHCHALLENGER_DB_FILE
   AND NOT CATCHCHALLENGER_DB_POSTGRESQL
   AND NOT CATCHCHALLENGER_DB_MYSQL
   AND NOT CATCHCHALLENGER_DB_SQLITE
   AND NOT CATCHCHALLENGER_DB_BLACKHOLE)
    set(CATCHCHALLENGER_DB_FILE ON)
endif()
if(CATCHCHALLENGER_DB_FILE)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_DB_FILE)
endif()
if(CATCHCHALLENGER_DB_POSTGRESQL)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_DB_POSTGRESQL)
endif()
if(CATCHCHALLENGER_DB_MYSQL)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_DB_MYSQL)
endif()
if(CATCHCHALLENGER_DB_SQLITE)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_DB_SQLITE)
endif()

# Backend selection — these affect Epoll.cpp only. The three options
# are mutually exclusive (Epoll.hpp #error's if more than one is set).
if(CATCHCHALLENGER_SELECT)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_SELECT)
endif()
if(CATCHCHALLENGER_POLL)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_POLL)
endif()
if(CATCHCHALLENGER_IO_URING)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_IO_URING)
    target_include_directories(catchchallenger-server-cli-epoll PRIVATE ${LIBURING_INCLUDE_DIR})
    target_link_libraries(catchchallenger-server-cli-epoll PRIVATE ${LIBURING_LIBRARY})
endif()

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-server-cli-epoll PRIVATE ${ZSTD_LIBRARY})
else()
    target_link_libraries(catchchallenger-server-cli-epoll PRIVATE ${ZSTD_LIBRARY})
endif()

# Use mold if available — same default as the qmake-driven build.
find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-server-cli-epoll PRIVATE -fuse-ld=mold)
endif()
