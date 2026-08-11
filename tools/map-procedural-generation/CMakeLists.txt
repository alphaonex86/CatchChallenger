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
    LoadMapBuilding.cpp
    LoadMapCity.cpp
    TerrainFlattener.cpp
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

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(map-procedural-generation PRIVATE -fuse-ld=${CC_LINKER})
endif()

# Runtime data next to the binary: the generator resolves template/, skin/,
# tileset/ and its txt/xml inputs relative to the EXECUTABLE (LoadMap::readMap),
# so a fresh build directory is usable without any manual copy.
# This is a target of its OWN, always built: a POST_BUILD command only runs when
# the binary is relinked, so editing a template would never reach the build dir.
set(CC_RUNTIME_STAGE_COMMANDS)
# LINKED, not copied: the build directory then uses the very folders of the
# sources, so a template edited on either side is the same file and a folder
# deleted from the sources disappears here too. They were rm -rf'd and copied
# again on every build, so nothing local can be lost by replacing that copy.
foreach(runtimeDir template skin tileset)
    list(APPEND CC_RUNTIME_STAGE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_CURRENT_LIST_DIR}/${runtimeDir}
                -DDESTINATION=$<TARGET_FILE_DIR:map-procedural-generation>/${runtimeDir}
                -DREPLACE=1 -P ${CC_REPO_ROOT}/general/link-runtime.cmake)
endforeach()
# npc-slots.json is the REVIEWED npc line bank (git tracked): it is copied on
# every build, so what the generator writes is what was validated in the repo.
foreach(runtimeFile cities.txt cities-types.ini dialog.txt npc-slots.json informations.xml start.xml
                    minimap-1way-water.png minimap-2way1-water.png
                    minimap-2way2-water.png minimap-3way-water.png
                    minimap-4way-water.png
                    minimap-1way.png minimap-2way1.png minimap-2way2.png
                    minimap-3way.png minimap-4way.png minimap-citybig.png
                    minimap-citymedium.png minimap-citysmall.png
                    miniMapColor.png)
    list(APPEND CC_RUNTIME_STAGE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_LIST_DIR}/${runtimeFile}
                $<TARGET_FILE_DIR:map-procedural-generation>/${runtimeFile})
endforeach()
# The config files are LINKED too, so there is ONE of each: editing the
# checked-in settings.ini is editing what every build directory runs. Both hold
# maincode already, so the write-back of the generator changes nothing in them.
# settings.ini keeps an existing build-local file (it may hold local edits, the
# cmake message says how to opt in); settings-reference.ini was overwritten on
# every build anyway, so it is replaced.
list(APPEND CC_RUNTIME_STAGE_COMMANDS
    COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_CURRENT_LIST_DIR}/settings.ini
            -DDESTINATION=$<TARGET_FILE_DIR:map-procedural-generation>/settings.ini
            -P ${CC_REPO_ROOT}/general/link-runtime.cmake
    COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_CURRENT_LIST_DIR}/settings-reference.ini
            -DDESTINATION=$<TARGET_FILE_DIR:map-procedural-generation>/settings-reference.ini
            -DREPLACE=1 -P ${CC_REPO_ROOT}/general/link-runtime.cmake)
add_custom_target(map-procedural-generation-runtime ALL
    ${CC_RUNTIME_STAGE_COMMANDS}
    COMMENT "Staging template/ skin/ tileset/ and the inputs next to the binary")
add_dependencies(map-procedural-generation-runtime map-procedural-generation)
