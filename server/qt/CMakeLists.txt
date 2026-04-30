# server/qt — Qt-based in-process server for the single-player path.
# Used by client targets (qtopengl, qtcpu800x600) when single-player
# is enabled (CATCHCHALLENGER_SOLO defined). Otherwise the exec stays
# multi-only and doesn't link this lib.

find_package(Qt6 COMPONENTS Core Network Sql REQUIRED)

add_library(catchchallenger_server_qt INTERFACE)

target_sources(catchchallenger_server_qt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/EventThreader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QFakeServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QFakeSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QSslServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtClientList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtClientWithMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/QtPlayerUpdater.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/QtTimeRangeEventScanBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/QtTimerEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InternalServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NormalServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db/QtDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db/QtDatabaseMySQL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db/QtDatabasePostgreSQL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db/QtDatabaseSQLite.cpp
)

target_include_directories(catchchallenger_server_qt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# CATCHCHALLENGER_CLASS_QT replaces the epoll-based class macros
# (ALLINONESERVER / ONLYGAMESERVER) — server/base sources branch on
# this exact define for the in-process Qt path.
target_compile_definitions(catchchallenger_server_qt INTERFACE
    CATCHCHALLENGER_CLASS_QT
    # The server/qt sources guard their bodies with
    # `#if defined(CATCHCHALLENGER_SOLO)`. Linking this lib means
    # single-player is on; propagate the flag so the bodies compile.
    CATCHCHALLENGER_SOLO
)
