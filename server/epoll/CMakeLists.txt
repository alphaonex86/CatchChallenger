# server/epoll — catchchallenger-server-cli-epoll target (Phase 1) +
# the source-list variable that game-server-alone (Phase 2) reuses.

# Sources from catchchallenger-server-epoll.pri minus the bits that
# exclude_list in make-catchchallenger-server-cli-epoll.py drops.
# Exposed in PARENT_SCOPE so server/game-server-alone/CMakeLists.txt
# can pick it up (game-server-alone compiles them with a different
# CLASS macro than this exec, so they have to be re-listed there).
set(CATCHCHALLENGER_EPOLL_FILEDB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollClientList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollSslClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollSslServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Epoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollTimer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollGenericServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollGenericSslServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BaseServerEpoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerPrivateVariablesEpoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientWithMapEpoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main-epoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main-epoll2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimerCityCapture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimerSendInsertMoveRemove.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimerPositionSync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimerDdos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimerEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimeRangeEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/PlayerUpdaterEpoll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/TimeRangeEventScan.cpp
    PARENT_SCOPE
)

set(_filedb_sources
    EpollSocket.cpp
    EpollClient.cpp
    EpollClientList.cpp
    EpollServer.cpp
    EpollSslClient.cpp
    EpollSslServer.cpp
    Epoll.cpp
    EpollTimer.cpp
    EpollGenericServer.cpp
    EpollGenericSslServer.cpp
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

find_package(Threads REQUIRED)

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
if(CATCHCHALLENGER_DB_FILE)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_DB_FILE)
endif()

# Backend selection — these affect Epoll.cpp only.
if(CATCHCHALLENGER_POLL)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_POLL)
endif()
if(CATCHCHALLENGER_IO_URING)
    target_compile_definitions(catchchallenger-server-cli-epoll PRIVATE CATCHCHALLENGER_IO_URING)
    target_include_directories(catchchallenger-server-cli-epoll PRIVATE ${LIBURING_INCLUDE_DIR})
    target_link_libraries(catchchallenger-server-cli-epoll PRIVATE ${LIBURING_LIBRARY})
endif()

# zstd: external lib in the filedb build.
if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-server-cli-epoll PRIVATE ${ZSTD_LIBRARY})
endif()

# Use mold if available — same default as the qmake-driven build.
find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-server-cli-epoll PRIVATE -fuse-ld=mold)
endif()
