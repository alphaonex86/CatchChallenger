# map-procedural-generation — full procedural map generator (city + road
# + plants on top of terrain). Self-contained, one binary. Reuses the
# znoise + procgen sources from the sibling map-procedural-generation-
# terrain/ directory by directly compiling them in (the sibling tool's
# CMakeLists.txt also exports them as map_procgen_terrain_lib INTERFACE
# for the case where someone configures both — but here we are a
# standalone project, so we list the sources inline).

cmake_minimum_required(VERSION 3.20)
project(map_procedural_generation LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Xml Widgets Gui REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)
# Pull in the terrain lib's INTERFACE source list (matches the qmake-era
# include of map-procedural-generation-terrain.pri).
include(${CC_REPO_ROOT}/tools/map-procedural-generation-terrain/map-procedural-generation-terrain.cmake)

add_executable(map-procedural-generation
    main.cpp
    SettingsAll.cpp
    LoadMapAll.cpp
    LoadMapCity.cpp
    PartialMap.cpp
    MiniMapAll.cpp
    LoadMapRoad.cpp
)

target_compile_definitions(map-procedural-generation PRIVATE
    MAPPROCEDURALGENFULL
)

target_link_libraries(map-procedural-generation PRIVATE
    map_procgen_terrain_lib
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Xml
    Qt6::Widgets
    Qt6::Gui
)

set_target_properties(map-procedural-generation PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(map-procedural-generation PRIVATE -fuse-ld=mold)
endif()
