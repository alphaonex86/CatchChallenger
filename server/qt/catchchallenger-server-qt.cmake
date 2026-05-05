# server/qt/catchchallenger-server-qt.cmake — mirrors catchchallenger-server-qt.pri
#                                              + catchchallenger-server-qtheader.pri.
#
# Qt-based in-process server libs:
#   catchchallenger_qfakesocket — QFakeSocket / QFakeServer (Qt-side
#       fake-pipe sockets used by the in-process single-player server
#       and by the GUI server's testing harness). STATIC lib so a single
#       AUTOMOC pass produces the moc files.
#   catchchallenger_server_qt — INTERFACE lib with the Qt event-loop
#       server scaffolding (NormalServer, QtClient*, QtServer, QSslServer,
#       QtDatabase*, QtTimer*, EventThreader).
#
# Consumers: server/CMakeLists.txt (catchchallenger-server-gui),
# client/CMakeLists.txt (qtopengl with embedded server),
# client/qtcpu800x600/CMakeLists.txt (qtcpu800x600 with embedded server).
#
# This file does NOT define any executable.

if(NOT TARGET catchchallenger_qfakesocket)
    find_package(Qt6 COMPONENTS Core Network Sql REQUIRED)

    add_library(catchchallenger_qfakesocket STATIC
        ${CMAKE_CURRENT_LIST_DIR}/QFakeSocket.cpp
        ${CMAKE_CURRENT_LIST_DIR}/QFakeServer.cpp
    )
    set_target_properties(catchchallenger_qfakesocket PROPERTIES
        AUTOMOC ON
    )
    target_include_directories(catchchallenger_qfakesocket PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    )
    target_compile_definitions(catchchallenger_qfakesocket PRIVATE
        CATCHCHALLENGER_SOLO
    )
    target_link_libraries(catchchallenger_qfakesocket PUBLIC
        Qt6::Core
        Qt6::Network
    )
endif()

if(NOT TARGET catchchallenger_server_qt)
    add_library(catchchallenger_server_qt INTERFACE)

    # QFakeSocket.cpp is intentionally NOT listed here — it lives in
    # catchchallenger_qfakesocket above. server_qt sources still #include
    # QFakeSocket.hpp; the consuming binary picks up the symbols via
    # transitive link to catchchallenger_qfakesocket.
    target_sources(catchchallenger_server_qt INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/EventThreader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/QtClient.cpp
        ${CMAKE_CURRENT_LIST_DIR}/QtClientList.cpp
        ${CMAKE_CURRENT_LIST_DIR}/QtClientWithMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/timer/QtPlayerUpdater.cpp
        ${CMAKE_CURRENT_LIST_DIR}/timer/QtTimeRangeEventScanBase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/timer/QtTimerEvents.cpp
        ${CMAKE_CURRENT_LIST_DIR}/QtServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/InternalServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/NormalServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/db/QtDatabase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/db/QtDatabaseMySQL.cpp
        ${CMAKE_CURRENT_LIST_DIR}/db/QtDatabasePostgreSQL.cpp
        ${CMAKE_CURRENT_LIST_DIR}/db/QtDatabaseSQLite.cpp
    )

    target_include_directories(catchchallenger_server_qt INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
    )

    # CATCHCHALLENGER_CLASS_QT replaces the epoll-based class macros
    # (ALLINONESERVER / ONLYGAMESERVER) — server/base sources branch on
    # this exact define for the in-process Qt path.
    # CATCHCHALLENGER_SOLO: server/qt sources guard their bodies with
    # `#if defined(CATCHCHALLENGER_SOLO)`. Linking this lib means
    # single-player is on; propagate the flag so the bodies compile.
    target_compile_definitions(catchchallenger_server_qt INTERFACE
        CATCHCHALLENGER_CLASS_QT
        CATCHCHALLENGER_SOLO
    )
endif()
