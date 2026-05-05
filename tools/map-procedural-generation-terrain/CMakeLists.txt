# map-procedural-generation-terrain — terrain-only procedural map gen.
# Self-contained, one binary (`map-procedural-generation-terrain`).
# Bundles vendored znoise (Perlin/Simplex/Worley/...) sources.
#
# Also exports `map_procgen_terrain_lib` (INTERFACE) so the sibling
# tools/map-procedural-generation/ can reuse the same source list when
# its CMakeLists.txt opts into building both.

cmake_minimum_required(VERSION 3.20)
project(map_procedural_generation_terrain LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Xml Widgets Gui REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)
# Reusable INTERFACE lib (mirrors map-procedural-generation-terrain.pri).
include(${CMAKE_CURRENT_LIST_DIR}/map-procedural-generation-terrain.cmake)

# When this CMakeLists.txt is the standalone-opened project, also produce
# the binary. When add_subdirectory'd from the sibling map-procedural-
# generation/ project, EXCLUDE_FROM_ALL means this binary doesn't end up
# in the parent's default-build set.
add_executable(map-procedural-generation-terrain
    main.cpp
    resources.qrc
)

target_link_libraries(map-procedural-generation-terrain PRIVATE
    map_procgen_terrain_lib
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Xml
    Qt6::Widgets
    Qt6::Gui
)

set_target_properties(map-procedural-generation-terrain PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(map-procedural-generation-terrain PRIVATE -fuse-ld=mold)
endif()
