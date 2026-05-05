# client/CMakeLists.txt — catchchallenger (qtopengl) binary.
#
# Self-contained: opening this file in Qt Creator (or
# `cmake -S client -B build/client`) configures and builds the qtopengl
# client. Sources live in client/qtopengl/, but the CMakeLists.txt that
# produces the binary is HERE at /client/ — that matches the
# one-binary-per-CMakeLists rule (the qtcpu800x600 binary lives at
# /client/qtcpu800x600/CMakeLists.txt).
#
# Pulls in:
#   general/                              → catchchallenger_general
#   client/libcatchchallenger/            → catchchallenger_client_lib
#   client/libqtcatchchallenger/          → catchchallenger_qt_lib
#                                           (libtiled / libogg / libopus
#                                           / libopusfile cascade from
#                                           there as needed)
# Single-player (CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON) also
# embeds the in-process Qt server:
#   server/base/                          → catchchallenger_server_base /
#                                           catchchallenger_server_sql
#   server/qt/                            → catchchallenger_server_qt /
#                                           catchchallenger_qfakesocket

cmake_minimum_required(VERSION 3.20)
project(catchchallenger LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui Network Widgets OpenGL OpenGLWidgets Xml REQUIRED)

option(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS
       "Link Qt6::WebSockets into qtopengl (matches default qmake build)"
       ON)
option(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER
       "Embed the in-process Qt server (CATCHCHALLENGER_CLASS_QT + CATCHCHALLENGER_DB_SQLITE)"
       ON)
option(CATCHCHALLENGER_NOAUDIO
       "Disable QtMultimedia audio (Audio.cpp / Audio.hpp dropped)"
       OFF)

# Pull SOLO into the libqtcatchchallenger lib build before we add it as a
# subdirectory — libqtcatchchallenger.cmakefile reads CATCHCHALLENGER_SOLO
# at configure time and includes SoloDatabaseInit.cpp + Audio.cpp behaviour
# accordingly. Allow either spelling — `-DCATCHCHALLENGER_SOLO=ON` and
# `-DCATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON` are equivalent.
if(CATCHCHALLENGER_SOLO)
    set(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER ON CACHE BOOL "" FORCE)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    set(CATCHCHALLENGER_SOLO ON CACHE BOOL "" FORCE)
endif()

# When the Qt6::WebSockets module isn't installed (Android Qt-for-Android
# default), libqtcatchchallenger forces CATCHCHALLENGER_NO_WEBSOCKET ON;
# turn the qtopengl-side toggle OFF in that case so we don't try to
# find_package(WebSockets REQUIRED) — find_package(... QUIET) is enough.
find_package(Qt6 COMPONENTS WebSockets QUIET)
if(NOT Qt6WebSockets_FOUND)
    set(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS OFF CACHE BOOL "" FORCE)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    find_package(Qt6 COMPONENTS WebSockets REQUIRED)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    find_package(Qt6 COMPONENTS Sql REQUIRED)
endif()

# Library .cmake fragments — mirrors the qmake-era include() of .pri
# files. Each fragment defines its INTERFACE / STATIC libs idempotently
# (guarded by `if(NOT TARGET …)`).
include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libqt.cmake)
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    # Single-player needs the in-process Qt server. server-qt.cmake must
    # be include()'d BEFORE libqt.cmake's SOLO branch links qfakesocket
    # to qt_lib — but libqt.cmake guards that link with TARGET checks, so
    # the order is OK either way.
    include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)
    include(${CC_REPO_ROOT}/server/qt/catchchallenger-server-qt.cmake)
endif()

set(_qtopengl_dir ${CMAKE_CURRENT_SOURCE_DIR}/qtopengl)
set(_qtopengl_sources
    # client.pri SOURCES (UI + game flow)
    ${_qtopengl_dir}/CCGraphicsTextItem.cpp
    ${_qtopengl_dir}/CCSliderH.cpp
    ${_qtopengl_dir}/MultiItem.cpp
    ${_qtopengl_dir}/ConnexionInfo.cpp
    ${_qtopengl_dir}/CCScrollZone.cpp
    ${_qtopengl_dir}/LineEdit.cpp
    ${_qtopengl_dir}/ComboBox.cpp
    ${_qtopengl_dir}/SpinBox.cpp
    ${_qtopengl_dir}/above/AddOrEditServer.cpp
    ${_qtopengl_dir}/above/OptionsDialog.cpp
    ${_qtopengl_dir}/above/Login.cpp
    ${_qtopengl_dir}/CheckBox.cpp
    ${_qtopengl_dir}/foreground/Battle.cpp
    ${_qtopengl_dir}/foreground/MainScreen.cpp
    ${_qtopengl_dir}/foreground/Multi.cpp
    ${_qtopengl_dir}/foreground/CharacterList.cpp
    ${_qtopengl_dir}/foreground/SubServer.cpp
    ${_qtopengl_dir}/foreground/QGraphicsTextItemOutline.cpp
    ${_qtopengl_dir}/above/AddCharacter.cpp
    ${_qtopengl_dir}/above/NewGame.cpp
    ${_qtopengl_dir}/FacilityLibClient.cpp
    ${_qtopengl_dir}/background/CCMap.cpp
    ${_qtopengl_dir}/foreground/OverMap.cpp
    ${_qtopengl_dir}/CustomText.cpp
    ${_qtopengl_dir}/foreground/widgets/MapMonsterPreview.cpp
    ${_qtopengl_dir}/ImagesStrechMiddle.cpp
    ${_qtopengl_dir}/MyApplication.cpp
    ${_qtopengl_dir}/above/DebugDialog.cpp
    ${_qtopengl_dir}/foreground/OverMapLogic.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicTrade.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicShop.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicBot.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicFactory.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicClan.cpp
    ${_qtopengl_dir}/above/inventory/Inventory.cpp
    ${_qtopengl_dir}/above/inventory/Plant.cpp
    ${_qtopengl_dir}/above/inventory/Crafting.cpp
    ${_qtopengl_dir}/above/Animation.cpp
    ${_qtopengl_dir}/above/Encyclopedia.cpp
    ${_qtopengl_dir}/above/Shop.cpp
    ${_qtopengl_dir}/above/Factory.cpp
    ${_qtopengl_dir}/above/Trade.cpp
    ${_qtopengl_dir}/above/Warehouse.cpp
    ${_qtopengl_dir}/above/player/Player.cpp
    ${_qtopengl_dir}/above/player/Reputations.cpp
    ${_qtopengl_dir}/above/player/Quests.cpp
    ${_qtopengl_dir}/above/player/FinishedQuests.cpp
    ${_qtopengl_dir}/QGraphicsPixmapItemClick.cpp
    ${_qtopengl_dir}/above/MonsterDetails.cpp
    ${_qtopengl_dir}/GameLoader.cpp
    ${_qtopengl_dir}/ChatParsing.cpp
    ${_qtopengl_dir}/GameLoaderThread.cpp
    ${_qtopengl_dir}/ProgressBarPixel.cpp
    ${_qtopengl_dir}/main.cpp
    ${_qtopengl_dir}/CCTitle.cpp
    ${_qtopengl_dir}/ConnexionManager.cpp
    ${_qtopengl_dir}/foreground/LoadingScreen.cpp
    ${_qtopengl_dir}/CustomButton.cpp
    ${_qtopengl_dir}/ScreenTransition.cpp
    ${_qtopengl_dir}/background/CCBackground.cpp
    ${_qtopengl_dir}/CCprogressbar.cpp
    ${_qtopengl_dir}/ScreenInput.cpp
    ${_qtopengl_dir}/AudioGL.cpp
)

set(_qtopengl_resources
    ${_qtopengl_dir}/resources/client-resources.qrc
)
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    # SoloDatabaseInit reads `:/catchchallenger-sqlite.sql` to bootstrap
    # the savegame DB. The qrc lives next to the canonical schema in
    # server/databases/ so server + client embed the same file.
    list(APPEND _qtopengl_resources
        ${CC_REPO_ROOT}/server/databases/resources-db-sqlite.qrc
    )
endif()

# Use Qt's executable wrapper when available — on Android it produces a
# shared library + a -deployment-settings.json suitable for androiddeployqt.
# On host platforms it behaves the same as add_executable.
if(COMMAND qt_add_executable)
    qt_add_executable(catchchallenger ${_qtopengl_sources} ${_qtopengl_resources})
else()
    add_executable(catchchallenger ${_qtopengl_sources} ${_qtopengl_resources})
endif()

target_compile_definitions(catchchallenger PRIVATE
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_CLASS_QT
    CATCHCHALLENGER_CLASS_CLIENT
    CATCHCHALLENGER_MULTI
    CATCHCHALLENGER_DB_SQLITE
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_CACHE_HPS)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_SOLO)
endif()
if(CATCHCHALLENGER_NOAUDIO)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_NOAUDIO)
endif()
if(NOT CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_NO_WEBSOCKET)
endif()

set_target_properties(catchchallenger PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_include_directories(catchchallenger PRIVATE
    ${_qtopengl_dir}
)

# NOTE: catchchallenger_common_flags is NOT linked here. That target
# carries `-fno-exceptions` (suitable for the epoll server / general lib
# consumers), but Qt UI code relies on exceptions (MyApplication.cpp's
# try/catch around bad_alloc, qFatal-with-exception flow, etc.). Use the
# Qt6 default flags instead.
target_link_libraries(catchchallenger PRIVATE
    catchchallenger_qt_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Xml
)
if(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    target_link_libraries(catchchallenger PRIVATE Qt6::WebSockets)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    target_link_libraries(catchchallenger PRIVATE
        catchchallenger_general
        catchchallenger_server_base
        catchchallenger_server_sql
        catchchallenger_server_qt
        catchchallenger_qfakesocket
        Qt6::Sql
    )
endif()

find_program(MOLD_EXE mold)
# mold doesn't have a PE/COFF backend yet, so MinGW cross-compiles must
# fall back to lld (or the default linker). Only opt into mold when the
# target system is Linux; otherwise the MinGW driver rejects -fuse-ld=mold
# at link time even though the host has mold installed.
if(MOLD_EXE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(catchchallenger PRIVATE -fuse-ld=mold)
endif()

# Android-only: install the qt_add_executable-produced shared library to
# the layout androiddeployqt expects.
if(ANDROID)
    install(TARGETS catchchallenger
        LIBRARY DESTINATION "libs/${CMAKE_ANDROID_ARCH_ABI}"
        RUNTIME DESTINATION "libs/${CMAKE_ANDROID_ARCH_ABI}"
    )
endif()
