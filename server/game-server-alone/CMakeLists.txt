# Phase 2 — server/game-server-alone/game-server-alone.pro -> CMake.
# catchchallenger-game-server-alone: same big source set as the
# all-in-one filedb cli-epoll exec, but compiled with
# CATCHCHALLENGER_CLASS_ONLYGAMESERVER + DATAPACK_ONLYBYMIRROR
# (instead of CATCHCHALLENGER_CLASS_ALLINONESERVER) AND adds
# the LinkToMaster + PlayerUpdaterToMaster + TimerPurgeTokenAuthList
# sources from this dir.

find_package(Threads REQUIRED)
find_library(LIBPQ_LIBRARY NAMES pq REQUIRED)

# Re-use the source list set in PARENT_SCOPE by server/epoll/CMakeLists.txt.
# game-server-alone defaults to PostgreSQL: same as the cli-epoll.pro
# default. DB_FILE-mode game-server-alone is not supported by upstream.
set(_extra_sources
    LinkToMaster.cpp
    LinkToMasterProtocolParsing.cpp
    LinkToMasterStaticVar.cpp
    timer/PlayerUpdaterToMaster.cpp
    ../epoll/timer/TimerPurgeTokenAuthList.cpp
    # TimerReconnectOnTheMaster.cpp exists in this dir but is NOT
    # listed in game-server-alone.pro — likely dead code.
    # DB-mode-specific source. cli-epoll.pro adds this conditionally;
    # game-server-alone always uses PostgreSQL.
    ../epoll/db/EpollPostgresql.cpp
)

add_executable(catchchallenger-game-server-alone
    ${CATCHCHALLENGER_EPOLL_FILEDB_SOURCES}
    ${_extra_sources}
)

target_include_directories(catchchallenger-game-server-alone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../epoll
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

# Class-mode + DB-mode overrides. ONLYGAMESERVER replaces
# ALLINONESERVER; DATAPACK_ONLYBYMIRROR is required (datapack content
# is fetched from the master mirror, never served by this process).
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

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-game-server-alone PRIVATE ${ZSTD_LIBRARY})
endif()

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-game-server-alone PRIVATE -fuse-ld=mold)
endif()
