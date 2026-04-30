# Phase 4 — tools.
# Each tool is opt-in. Most reuse the libs created in Phases 1-3
# (catchchallenger_general / catchchallenger_general_minimal /
# catchchallenger_client_lib / catchchallenger_qt_lib /
# catchchallenger_tiled). A few have small private bits or vendored
# helpers (znoise) — those stay in their own subdir.

option(CATCHCHALLENGER_BUILD_TOOL_BOT_TEST_CONNECT
       "Build bot-test-connect-to-gameserver (load test client)" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_BOT_ACTIONS
       "Build bot-actions (Qt6 GUI bot debugger)" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_DATAPACK_DOWNLOADER
       "Build datapack-downloader" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_DATAPACK_EXPLORER
       "Build datapack-explorer-generator" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_MAP2PNG
       "Build map2png" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_MAP_PROCGEN
       "Build map-procedural-generation" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_MAP_PROCGEN_TERRAIN
       "Build map-procedural-generation-terrain" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_STATS
       "Build stats" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_TMX_LAYER_CONVERTER
       "Build tmxTileLayerConverterToZstd" OFF)
option(CATCHCHALLENGER_BUILD_TOOL_FILEDB_CONVERTER
       "Build filedb-converter (lives under server/epoll/)" OFF)

# Most tools need the client libs. Auto-promote build flags so each
# tool's enable also pulls its dependencies.
if(CATCHCHALLENGER_BUILD_TOOL_BOT_TEST_CONNECT
   OR CATCHCHALLENGER_BUILD_TOOL_BOT_ACTIONS
   OR CATCHCHALLENGER_BUILD_TOOL_DATAPACK_DOWNLOADER
   OR CATCHCHALLENGER_BUILD_TOOL_DATAPACK_EXPLORER
   OR CATCHCHALLENGER_BUILD_TOOL_MAP2PNG)
    set(CATCHCHALLENGER_BUILD_CLIENT_QT_LIB ON CACHE BOOL "" FORCE)
endif()

if(CATCHCHALLENGER_BUILD_TOOL_BOT_TEST_CONNECT)
    add_subdirectory(bot-test-connect-to-gameserver-cli)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_BOT_ACTIONS)
    add_subdirectory(bot-actions)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_DATAPACK_DOWNLOADER)
    add_subdirectory(datapack-downloader-cli)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_DATAPACK_EXPLORER)
    add_subdirectory(datapack-explorer-generator-cli)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_MAP2PNG)
    add_subdirectory(map2png)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_MAP_PROCGEN_TERRAIN)
    add_subdirectory(map-procedural-generation-terrain)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_MAP_PROCGEN)
    add_subdirectory(map-procedural-generation)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_STATS)
    add_subdirectory(stats)
endif()
if(CATCHCHALLENGER_BUILD_TOOL_TMX_LAYER_CONVERTER)
    add_subdirectory(tmxTileLayerConverterToZstd)
endif()
