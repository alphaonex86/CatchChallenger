# map-procedural-generation-terrain — terrain-only procedural map gen.
# Bundles vendored znoise (Perlin/Simplex/Worley/...) sources.

find_package(Qt6 COMPONENTS Core Xml Widgets Gui REQUIRED)

# This same source list is reused by map-procedural-generation as a
# CMake INTERFACE lib so we don't repeat 30 .cpp lines.
add_library(map_procgen_terrain_lib INTERFACE)

target_sources(map_procgen_terrain_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/FBM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/HybridMultiFractal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/MixerBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/NoiseBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/NoiseTools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/Perlin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/Simplex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/cpp/Worley.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VoronioForTiledMapTmx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LoadMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransitionTerrain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MapPlants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MapBrush.cpp
)

target_include_directories(map_procgen_terrain_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/znoise/headers
)

target_compile_definitions(map_procgen_terrain_lib INTERFACE
    CATCHCHALLENGER_ONLYMAPRENDER
)

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
