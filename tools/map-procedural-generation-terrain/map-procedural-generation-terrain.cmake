# tools/map-procedural-generation-terrain/map-procedural-generation-terrain.cmake
# Mirrors map-procedural-generation-terrain.pri.
#
# INTERFACE library sharing the znoise + procgen-terrain sources between
# this tool's binary and the sibling tools/map-procedural-generation/
# binary. include() this fragment from a binary's CMakeLists.txt; it
# produces `map_procgen_terrain_lib` that consumers link.

if(NOT TARGET map_procgen_terrain_lib)
    add_library(map_procgen_terrain_lib INTERFACE)

    target_sources(map_procgen_terrain_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/FBM.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/HybridMultiFractal.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/MixerBase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/NoiseBase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/NoiseTools.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/Perlin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/Simplex.cpp
        ${CMAKE_CURRENT_LIST_DIR}/znoise/cpp/Worley.cpp
        ${CMAKE_CURRENT_LIST_DIR}/VoronioForTiledMapTmx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/LoadMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/TransitionTerrain.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Settings.cpp
        ${CMAKE_CURRENT_LIST_DIR}/MapPlants.cpp
        ${CMAKE_CURRENT_LIST_DIR}/MiniMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/MapBrush.cpp
    )

    target_include_directories(map_procgen_terrain_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/znoise/headers
    )

    target_compile_definitions(map_procgen_terrain_lib INTERFACE
        CATCHCHALLENGER_ONLYMAPRENDER
    )
endif()
