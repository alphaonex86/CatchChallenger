# map2png — render TMX maps to PNG. Self-contained, one binary.
# Pulls catchchallenger_general + catchchallenger_tiled (libtiled) from
# their library subdirs.

cmake_minimum_required(VERSION 3.20)
project(map2png LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui Xml Network OpenGL Widgets REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)

add_executable(map2png
    main.cpp
    map2png.cpp
    MapDoor.cpp
    Map_client.cpp
    TriggerAnimation.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/DatapackChecksum.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/DatapackClientLoader.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Language.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/QtDatapackClientLoader.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/QtDatapackClientLoaderThread.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Settings.cpp
    resources.qrc
)

target_compile_definitions(map2png PRIVATE
    CATCHCHALLENGER_ONLYMAPRENDER
    CATCHCHALLENGER_NO_WEBSOCKET
)

target_include_directories(map2png PRIVATE
    ${CC_REPO_ROOT}/client/libcatchchallenger
    ${CC_REPO_ROOT}/client/libqtcatchchallenger
)

set_target_properties(map2png PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_link_libraries(map2png PRIVATE
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::Network
    Qt6::OpenGL
    Qt6::Widgets
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(map2png PRIVATE -fuse-ld=mold)
endif()
