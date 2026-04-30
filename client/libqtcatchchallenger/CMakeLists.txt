# libqtcatchchallenger — Qt-based client middleware (Api_client_real,
# settings, datapack-on-Qt loader, …) plus the maprender/ subdir.
# Mirrors libqt.pri + libqtheader.pri + maprender/render.pri.
#
# Phase 3 scope: Linux-native, NOAUDIO (skips ogg/opus/opusfile),
# MULTI-only (so SoloDatabaseInit.cpp / Audio.cpp drop out at the
# .pri-equivalent level).

find_package(Qt6 COMPONENTS Core Gui Network Widgets OpenGL OpenGLWidgets Xml REQUIRED)

option(CATCHCHALLENGER_NOAUDIO
       "Disable QtMultimedia audio (Audio.cpp / Audio.hpp dropped)"
       OFF)
option(CATCHCHALLENGER_SOLO
       "Embed an in-process single-player server (SoloDatabaseInit + server/qt/)"
       OFF)

# Embedded libtiled — built as a separate static lib so it can be
# reused by tools/ later.
add_subdirectory(libtiled)

set(_libqtcc_sources
    Api_client_real_base.cpp
    Api_client_real.cpp
    Api_client_real_main.cpp
    Api_client_real_sub.cpp
    Api_client_virtual.cpp
    Api_protocol_Qt.cpp
    QtDatapackClientLoader.cpp
    QtDatapackClientLoaderThread.cpp
    QtDatapackChecksum.cpp
    QZstdDecodeThread.cpp
    Language.cpp
    Settings.cpp
    CliClientOptions.cpp
    FeedNews.cpp
    InternetUpdater.cpp
    Ultimate.cpp
    LanBroadcastWatcher.cpp
    LocalListener.cpp
    ExtraSocket.cpp
    ConnectedSocket.cpp
    QInfiniteBuffer.cpp
)

# Maprender sources sit in a sibling dir but share the same target —
# matches the qmake .pri layout (render.pri included from the .pro).
list(APPEND _libqtcc_sources
    maprender/MapController.cpp
    maprender/MapControllerMPAPI.cpp
    maprender/MapControllerMP.cpp
    maprender/MapControllerMPMove.cpp
    maprender/MapDoor.cpp
    maprender/MapItem.cpp
    maprender/MapMark.cpp
    maprender/MapObjectItem.cpp
    maprender/MapVisualiser.cpp
    maprender/MapVisualiser-map.cpp
    maprender/MapVisualiserOrder.cpp
    maprender/MapVisualiserPlayer.cpp
    maprender/MapVisualiserThread.cpp
    maprender/ObjectGroupItem.cpp
    maprender/PathFinding.cpp
    maprender/PreparedLayer.cpp
    maprender/TemporaryTile.cpp
    maprender/TileLayerItem.cpp
    maprender/TriggerAnimation.cpp
    maprender/QMap_client.cpp
    maprender/MapVisualiserPlayerWithFight.cpp
    maprender/ClientPlantWithTimer.cpp
    maprender/MapControllerCrafting.cpp
)

if(NOT CATCHCHALLENGER_NOAUDIO)
    list(APPEND _libqtcc_sources Audio.cpp)
endif()
if(CATCHCHALLENGER_SOLO)
    list(APPEND _libqtcc_sources SoloDatabaseInit.cpp)
endif()

add_library(catchchallenger_qt_lib STATIC ${_libqtcc_sources})

target_include_directories(catchchallenger_qt_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/maprender
)

target_compile_definitions(catchchallenger_qt_lib PUBLIC
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGERLIB
)
if(CATCHCHALLENGER_NOAUDIO)
    target_compile_definitions(catchchallenger_qt_lib PUBLIC CATCHCHALLENGER_NOAUDIO)
endif()
if(CATCHCHALLENGER_SOLO)
    find_package(Qt6 COMPONENTS Sql REQUIRED)
    target_compile_definitions(catchchallenger_qt_lib PUBLIC CATCHCHALLENGER_SOLO)
    target_link_libraries(catchchallenger_qt_lib PUBLIC Qt6::Sql)
    # qt_lib's ConnectedSocket.cpp references QFakeSocket when SOLO is
    # set. Pull the in-process server INTERFACE lib here so consumers
    # that don't know about server/qt/ (e.g. tools/bot-actions,
    # datapack-downloader) get the QFakeSocket symbols transitively.
    target_link_libraries(catchchallenger_qt_lib PUBLIC catchchallenger_server_qt)
endif()

set_target_properties(catchchallenger_qt_lib PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# catchchallenger_general / _client_lib are INTERFACE libs whose
# .cpp files attach to whichever target consumes them. Linking them
# PUBLIC here would re-attach those sources to every consumer of
# catchchallenger_qt_lib (e.g. tools/bot-actions), producing duplicate
# symbols at link time because the .a already contains those object
# files. Use PRIVATE so they live inside catchchallenger_qt_lib only.
target_link_libraries(catchchallenger_qt_lib PRIVATE
    catchchallenger_general
    catchchallenger_client_lib
)
# Pass the INTERFACE include dirs + defines through to consumers
# without re-attaching the sources. Mirrors what PUBLIC would do for
# headers + defines but stops short of source propagation.
target_include_directories(catchchallenger_qt_lib PUBLIC
    $<TARGET_PROPERTY:catchchallenger_general,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:catchchallenger_client_lib,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(catchchallenger_qt_lib PUBLIC
    $<TARGET_PROPERTY:catchchallenger_general,INTERFACE_COMPILE_DEFINITIONS>
    $<TARGET_PROPERTY:catchchallenger_client_lib,INTERFACE_COMPILE_DEFINITIONS>
)
target_link_libraries(catchchallenger_qt_lib PUBLIC
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Xml
)

if(NOT CATCHCHALLENGER_NOAUDIO)
    find_package(Qt6 COMPONENTS Multimedia REQUIRED)
    # Vendored libogg + libopus + libopusfile — used by Audio.cpp.
    include(${CMAKE_CURRENT_SOURCE_DIR}/audio.cmake)
    target_link_libraries(catchchallenger_qt_lib PUBLIC
        Qt6::Multimedia
        catchchallenger_libopusfile
    )
endif()
