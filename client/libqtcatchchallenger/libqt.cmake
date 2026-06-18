# client/libqtcatchchallenger/libqt.cmake — mirrors libqt.pri + libqtheader.pri
#                                            + maprender/render.pri + renderheader.pri.
#
# Qt-based client middleware (Api_client_real, settings, datapack-on-Qt
# loader) plus the maprender/ subdir.
#
# Phase 3 scope: Linux-native, NOAUDIO opt-in (skips ogg/opus/opusfile),
# MULTI by default (CATCHCHALLENGER_SOLO opt-in adds SoloDatabaseInit.cpp +
# Audio.cpp + the in-process server libs from server/qt/).
#
# include() this fragment from a binary's CMakeLists.txt to get the
# `catchchallenger_qt_lib` static library. Pre-requisites the binary
# must already include() before this file:
#   * general/CCCommon.cmake (find_package ZLIB, ZSTD_LIBRARY discovery)
#   * general/general.cmake (catchchallenger_general)
#   * client/libcatchchallenger/lib.cmake (catchchallenger_client_lib)
# This fragment also pulls in libtiled.cmake (a sibling of this file).

if(NOT TARGET catchchallenger_qt_lib)
    find_package(Qt6 COMPONENTS Core Gui Network Widgets OpenGL OpenGLWidgets Xml REQUIRED)

    if(NOT DEFINED CATCHCHALLENGER_NOAUDIO)
        option(CATCHCHALLENGER_NOAUDIO
               "Disable QtMultimedia audio (Audio.cpp / Audio.hpp dropped)"
               OFF)
    endif()
    if(NOT DEFINED CATCHCHALLENGER_SOLO)
        option(CATCHCHALLENGER_SOLO
               "Embed an in-process single-player server (SoloDatabaseInit + server/qt/)"
               OFF)
    endif()

    # Probe Qt6::WebSockets — Android Qt-for-Android ships without it,
    # so silently force CATCHCHALLENGER_NO_WEBSOCKET=ON when missing.
    find_package(Qt6 COMPONENTS WebSockets QUIET)
    if(NOT Qt6WebSockets_FOUND)
        set(CATCHCHALLENGER_NO_WEBSOCKET ON CACHE BOOL "" FORCE)
    endif()

    # Embedded libtiled — sibling .cmake fragment (same dir as this file).
    include(${CMAKE_CURRENT_LIST_DIR}/libtiled.cmake)

    set(_libqtcc_dir ${CMAKE_CURRENT_LIST_DIR})
    set(_libqtcc_sources
        ${_libqtcc_dir}/Api_client_real_base.cpp
        ${_libqtcc_dir}/Api_client_real.cpp
        ${_libqtcc_dir}/Api_client_real_main.cpp
        ${_libqtcc_dir}/Api_client_real_sub.cpp
        ${_libqtcc_dir}/Api_client_virtual.cpp
        ${_libqtcc_dir}/Api_protocol_Qt.cpp
        ${_libqtcc_dir}/QtDatapackClientLoader.cpp
        ${_libqtcc_dir}/QtDatapackClientLoaderThread.cpp
        ${_libqtcc_dir}/QtDatapackChecksum.cpp
        ${_libqtcc_dir}/QZstdDecodeThread.cpp
        ${_libqtcc_dir}/Language.cpp
        ${_libqtcc_dir}/Settings.cpp
        ${_libqtcc_dir}/CliClientOptions.cpp
        ${_libqtcc_dir}/FeedNews.cpp
        ${_libqtcc_dir}/InternetUpdater.cpp
        ${_libqtcc_dir}/Ultimate.cpp
        ${_libqtcc_dir}/LanBroadcastWatcher.cpp
        ${_libqtcc_dir}/LocalListener.cpp
        ${_libqtcc_dir}/ExtraSocket.cpp
        ${_libqtcc_dir}/ConnectedSocket.cpp
        ${_libqtcc_dir}/QInfiniteBuffer.cpp
        # maprender/render.pri — same target.
        ${_libqtcc_dir}/maprender/MapController.cpp
        ${_libqtcc_dir}/maprender/MapControllerMPAPI.cpp
        ${_libqtcc_dir}/maprender/MapControllerMP.cpp
        ${_libqtcc_dir}/maprender/MapControllerMPMove.cpp
        ${_libqtcc_dir}/maprender/MapDoor.cpp
        ${_libqtcc_dir}/maprender/MapItem.cpp
        ${_libqtcc_dir}/maprender/MapMark.cpp
        ${_libqtcc_dir}/maprender/MapObjectItem.cpp
        ${_libqtcc_dir}/maprender/MapVisualiser.cpp
        ${_libqtcc_dir}/maprender/MapVisualiser-map.cpp
        ${_libqtcc_dir}/maprender/MapVisualiserOrder.cpp
        ${_libqtcc_dir}/maprender/MapVisualiserPlayer.cpp
        ${_libqtcc_dir}/maprender/MapVisualiserThread.cpp
        ${_libqtcc_dir}/maprender/ObjectGroupItem.cpp
        ${_libqtcc_dir}/maprender/PathFinding.cpp
        ${_libqtcc_dir}/maprender/PreparedLayer.cpp
        ${_libqtcc_dir}/maprender/TemporaryTile.cpp
        ${_libqtcc_dir}/maprender/TileLayerItem.cpp
        ${_libqtcc_dir}/maprender/TriggerAnimation.cpp
        ${_libqtcc_dir}/maprender/QMap_client.cpp
        ${_libqtcc_dir}/maprender/MapVisualiserPlayerWithFight.cpp
        ${_libqtcc_dir}/maprender/ClientPlantWithTimer.cpp
        ${_libqtcc_dir}/maprender/MapControllerCrafting.cpp
    )

    if(NOT CATCHCHALLENGER_NOAUDIO)
        list(APPEND _libqtcc_sources ${_libqtcc_dir}/Audio.cpp)
    endif()
    if(CATCHCHALLENGER_SOLO)
        list(APPEND _libqtcc_sources ${_libqtcc_dir}/SoloDatabaseInit.cpp)
    endif()

    add_library(catchchallenger_qt_lib STATIC ${_libqtcc_sources})

    target_include_directories(catchchallenger_qt_lib PUBLIC
        ${_libqtcc_dir}
        ${_libqtcc_dir}/maprender
    )

    target_compile_definitions(catchchallenger_qt_lib PUBLIC
        CATCHCHALLENGER_CLIENT
        CATCHCHALLENGERLIB
    )
    if(CATCHCHALLENGER_NOAUDIO)
        target_compile_definitions(catchchallenger_qt_lib PUBLIC CATCHCHALLENGER_NOAUDIO)
    endif()
    if(CATCHCHALLENGER_NO_WEBSOCKET)
        target_compile_definitions(catchchallenger_qt_lib PUBLIC CATCHCHALLENGER_NO_WEBSOCKET)
    endif()
    if(CATCHCHALLENGER_SOLO)
        find_package(Qt6 COMPONENTS Sql REQUIRED)
        target_compile_definitions(catchchallenger_qt_lib PUBLIC CATCHCHALLENGER_SOLO)
        target_link_libraries(catchchallenger_qt_lib PUBLIC Qt6::Sql)
        # qt_lib's ConnectedSocket.cpp references QFakeSocket when SOLO is
        # set. The QFakeSocket symbols live in catchchallenger_qfakesocket
        # (defined in server/qt/catchchallenger-server-qt.cmake), so consumers
        # of qt_lib get the symbols transitively.
        if(TARGET catchchallenger_qfakesocket)
            target_link_libraries(catchchallenger_qt_lib PUBLIC catchchallenger_qfakesocket)
        endif()
    endif()

    set_target_properties(catchchallenger_qt_lib PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )

    # catchchallenger_general / _client_lib are INTERFACE libs; link them
    # PRIVATE so their .cpp files attach to qt_lib only (consumers like
    # tools/bot-actions don't re-attach via PUBLIC).
    target_link_libraries(catchchallenger_qt_lib PRIVATE
        catchchallenger_general
        catchchallenger_client_lib
    )
    # Pass INTERFACE include dirs + defines through to consumers.
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
        # The audio.cmake helper file (sibling) defines all three lib targets.
        if(EXISTS ${_libqtcc_dir}/audio.cmake)
            include(${_libqtcc_dir}/audio.cmake)
            target_link_libraries(catchchallenger_qt_lib PUBLIC
                Qt6::Multimedia
                catchchallenger_libopusfile
            )
        endif()
    endif()
endif()
