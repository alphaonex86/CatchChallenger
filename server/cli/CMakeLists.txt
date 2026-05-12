# server/cli/CMakeLists.txt — catchchallenger-server-cli binary.
#
# Self-contained: opening this file in Qt Creator (or
# `cmake -S server/cli -B build/server-epoll`) configures and builds
# exactly the epoll game server. Pulls in catchchallenger_general +
# catchchallenger_server_base via add_subdirectory of the lib subdirs.
#
# server/cli/filedb-converter/ has its own CMakeLists.txt that
# produces the filedb-converter binary; this CMakeLists.txt does NOT
# add_subdirectory(filedb-converter) — open that one directly to build it.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_server_cli_unix LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Threads REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)

set(_filedb_sources
    SocketUtil.cpp
    EventLoopClient.cpp
    EventLoopClientList.cpp
    EventLoopServer.cpp
    EventLoop.cpp
    EventLoopTimer.cpp
    EventLoopGenericServer.cpp
    BaseServerEventLoop.cpp
    ServerPrivateVariablesEventLoop.cpp
    ClientWithMapEventLoop.cpp
    main-unix.cpp
    main-unix2.cpp
    timer/TimerCityCapture.cpp
    timer/TimerSendInsertMoveRemove.cpp
    timer/TimerPositionSync.cpp
    timer/TimerDdos.cpp
    timer/TimerEvents.cpp
    timer/TimeRangeEvent.cpp
    timer/PlayerUpdaterEventLoop.cpp
    timer/TimeRangeEventScan.cpp
)

add_executable(catchchallenger-server-cli ${_filedb_sources})

target_include_directories(catchchallenger-server-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(catchchallenger-server-cli PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    catchchallenger_server_base
    Threads::Threads
    ZLIB::ZLIB
)

# IO-backend + class-mode + DB-mode are per-executable.
target_compile_definitions(catchchallenger-server-cli PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_ALLINONESERVER
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_CACHE_HPS)
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
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_DB_FILE)
endif()
if(CATCHCHALLENGER_DB_POSTGRESQL)
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_DB_POSTGRESQL)
    find_library(LIBPQ_LIBRARY NAMES pq REQUIRED)
    target_sources(catchchallenger-server-cli PRIVATE db/EventLoopPostgresql.cpp)
    target_link_libraries(catchchallenger-server-cli PRIVATE ${LIBPQ_LIBRARY})
endif()
if(CATCHCHALLENGER_DB_MYSQL)
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_DB_MYSQL)
    # Prefer MariaDB client (libmariadb) — drop-in replacement for
    # libmysqlclient, ABI-compatible with the <mysql/mysql.h> headers
    # EventLoopMySQL.cpp uses. Fall back to libmysqlclient if mariadb
    # isn't installed.
    find_library(LIBMARIADB_LIBRARY NAMES mariadb mysqlclient REQUIRED)
    target_sources(catchchallenger-server-cli PRIVATE db/EventLoopMySQL.cpp)
    target_link_libraries(catchchallenger-server-cli PRIVATE ${LIBMARIADB_LIBRARY})
endif()
if(CATCHCHALLENGER_DB_SQLITE)
    message(FATAL_ERROR
        "catchchallenger-server-cli has no SQLite driver — only DB_FILE, DB_POSTGRESQL, DB_MYSQL are implemented for this binary.")
endif()

# Pull in PreparedDBQuery* / SqlFunction / DatabaseBase plumbing when
# any SQL backend is selected. Without this the MySQL/PG build links
# cleanly except for these helpers (every PreparedStatement call
# resolves as an undefined symbol — see
# server/catchchallenger-server.cmake `catchchallenger_server_sql`).
# DB_FILE alone does not need the SQL path; that's why it's gated.
if(CATCHCHALLENGER_DB_POSTGRESQL OR CATCHCHALLENGER_DB_MYSQL)
    target_link_libraries(catchchallenger-server-cli PRIVATE catchchallenger_server_sql)
endif()

# On Windows the only supported IO backend is select(2). Force-pin and
# fatal-error if the user toggled poll/io_uring (the latter is Linux-only,
# the former needs POSIX poll() semantics not present in MinGW).
if(WIN32)
    if(CATCHCHALLENGER_POLL OR CATCHCHALLENGER_IO_URING)
        message(FATAL_ERROR "Windows build supports only CATCHCHALLENGER_SELECT (no poll/io_uring).")
    endif()
    set(CATCHCHALLENGER_SELECT ON)
endif()

# Backend selection — these affect EventLoop.cpp only. The three options
# are mutually exclusive (EventLoop.hpp #error's if more than one is set).
if(CATCHCHALLENGER_SELECT)
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_SELECT)
endif()
if(CATCHCHALLENGER_POLL)
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_POLL)
endif()
if(CATCHCHALLENGER_IO_URING)
    target_compile_definitions(catchchallenger-server-cli PRIVATE CATCHCHALLENGER_IO_URING)
    target_include_directories(catchchallenger-server-cli PRIVATE ${LIBURING_INCLUDE_DIR})
    target_link_libraries(catchchallenger-server-cli PRIVATE ${LIBURING_LIBRARY})
endif()

if(WIN32)
    target_link_libraries(catchchallenger-server-cli PRIVATE ws2_32 mswsock)
    target_compile_definitions(catchchallenger-server-cli PRIVATE
        _WIN32_WINNT=0x0A00
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    # MinGW PE/COFF object format caps at 65535 sections; large
    # template-instantiation TUs (BaseServer3.cpp etc.) overflow it.
    # -Wa,-mbig-obj switches to the bigobj COFF variant.
    target_compile_options(catchchallenger-server-cli PRIVATE -Wa,-mbig-obj)
endif()

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-server-cli PRIVATE ${ZSTD_LIBRARY})
else()
    target_link_libraries(catchchallenger-server-cli PRIVATE ${ZSTD_LIBRARY})
endif()

# Use mold if available — same default as the qmake-driven build.
find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-server-cli PRIVATE -fuse-ld=mold)
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
    target_compile_definitions(catchchallenger-server-cli PRIVATE
        CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE)
endif()
