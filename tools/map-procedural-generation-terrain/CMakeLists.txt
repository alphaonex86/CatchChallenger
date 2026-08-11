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

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(map-procedural-generation-terrain PRIVATE -fuse-ld=${CC_LINKER})
endif()

# The settings.xml of the build directory is a LINK to the checked-in one, so
# there is ONE config: editing it here is editing what the build runs. It holds
# maincode already, so the write-back of the generator changes nothing in it.
# Its own target, always built, so a settings edit reaches the build directory
# even when the binary is not relinked. An existing build-local settings.xml is
# kept (see link-runtime.cmake).
add_custom_target(map-procedural-generation-terrain-runtime ALL
    COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_CURRENT_LIST_DIR}/settings.xml
            -DDESTINATION=$<TARGET_FILE_DIR:map-procedural-generation-terrain>/settings.xml
            -P ${CC_REPO_ROOT}/general/link-runtime.cmake
    COMMENT "Linking settings.xml next to the binary")
add_dependencies(map-procedural-generation-terrain-runtime map-procedural-generation-terrain)
