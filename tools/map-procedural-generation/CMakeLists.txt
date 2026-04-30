# map-procedural-generation — full procedural map generator (city + road
# + plants on top of terrain). Reuses map_procgen_terrain_lib INTERFACE
# from the sibling tool, just like the .pro `include`s its .pri.

find_package(Qt6 COMPONENTS Core Xml Widgets Gui REQUIRED)

# map_procgen_terrain_lib lives in the sibling subdir. CMake doesn't
# automatically traverse it unless the user enabled that tool too,
# so include it here if needed.
if(NOT TARGET map_procgen_terrain_lib)
    add_subdirectory(
        ${CMAKE_SOURCE_DIR}/tools/map-procedural-generation-terrain
        ${CMAKE_BINARY_DIR}/tools/map-procedural-generation-terrain-aux
        EXCLUDE_FROM_ALL
    )
endif()

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
