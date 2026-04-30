# Sources here reference CATCHCHALLENGER_CLASS_* macros (e.g. inside
# CommonDatapack.cpp, FightLoader.cpp, ProtocolParsingInput.cpp), so a
# single OBJECT-lib compilation can't be reused by servers that pick
# different class modes (LOGIN / MASTER / GATEWAY / ONLYGAMESERVER /
# ALLINONESERVER). qmake side-steps this by compiling each .pro target
# independently. We mirror that by exposing the file list as an
# INTERFACE target: target_sources(... INTERFACE) attaches the .cpp
# files to whichever executable consumes the lib, and each executable
# gets its own object files compiled with its own class-mode defines.
#
# Split into two:
#   catchchallenger_general_minimal — class-agnostic networking +
#     hashing + settings + tinyXML2. Safe for LOGIN / MASTER / GATEWAY.
#   catchchallenger_general — pulls in the map/datapack/fight loader
#     sources too (these contain `#error` triggers for the non-game
#     class modes). Used by game servers (cli-epoll-filedb,
#     game-server-alone).

add_library(catchchallenger_general_minimal INTERFACE)

target_sources(catchchallenger_general_minimal INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ProtocolParsingGeneral.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ProtocolParsingInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ProtocolParsingOutput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/ProtocolParsingCheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CompressionProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/FacilityLibGeneral.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonSettingsCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonSettingsServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/cpp11addition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/cpp11additionstringtointcpp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/cpp11additionstringtointc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/Version.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CatchChallenger_Hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/blake3/blake3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/blake3/blake3_dispatch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/blake3/blake3_portable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libxxhash/xxhash.c
)
if(NOT CATCHCHALLENGER_NOXML)
    target_sources(catchchallenger_general_minimal INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyXML2/tinyxml2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyXML2/tinyxml2b.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyXML2/tinyxml2c.cpp
    )
endif()
target_include_directories(catchchallenger_general_minimal INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libxxhash
)
target_compile_definitions(catchchallenger_general_minimal INTERFACE
    XXH_INLINE_ALL
)
if(EXTERNALLIBZSTD)
    target_compile_definitions(catchchallenger_general_minimal INTERFACE EXTERNALLIBZSTD)
    # Propagate zstd + zlib link deps to all consumers — the general
    # sources call ZSTD_*/inflate/deflate from CompressionProtocol.cpp
    # and the tiled-style map decoder. Saves each consumer from
    # repeating target_link_libraries(... ${ZSTD_LIBRARY} ZLIB::ZLIB).
    target_link_libraries(catchchallenger_general_minimal INTERFACE
        ${ZSTD_LIBRARY}
        ZLIB::ZLIB
    )
endif()
# CATCHCHALLENGER_CACHE_HPS is set on the FULL general lib only — the
# minimal subset is consumed by login/master/gateway, none of which
# ship a cache file (login.pro / master.pro / gateway.pro never define
# CATCHCHALLENGER_CACHE_HPS).
if(CATCHCHALLENGER_CACHE_HPS)
    target_include_directories(catchchallenger_general_minimal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/hps)
endif()
if(CATCHCHALLENGER_NOXML)
    target_compile_definitions(catchchallenger_general_minimal INTERFACE CATCHCHALLENGER_NOXML)
endif()
# tinyxml2 is the only XML parser; no compile-time toggle needed.

# Full general lib for game servers — adds GeneralStructures, MoveOnTheMap,
# CommonMap/, fight engine, datapack loaders, map loader.
add_library(catchchallenger_general INTERFACE)
target_link_libraries(catchchallenger_general INTERFACE catchchallenger_general_minimal)

target_sources(catchchallenger_general INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/base/GeneralStructures.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/MoveOnTheMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/FacilityLib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/FightLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonMap/BaseMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonMap/CommonMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonMap/ItemOnMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonMap/Map_Border.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonMap/Teleporter.cpp
    # CommonDatapack.cpp is intentionally NOT listed: it's #include'd
    # at file scope by DatapackGeneralLoaderMap.cpp (see the
    # CATCHCHALLENGER_NOXML branch below for the alternative).
    ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonDatapackServerSpec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngineEnd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngineTurn.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngineBuff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngineSkill.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngineWild.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fight/CommonFightEngineBase.cpp
)

if(CATCHCHALLENGER_NOXML)
    # NOXML drops DatapackGeneralLoaderMap.cpp (which #include'd
    # CommonDatapack.cpp), so CommonDatapack.cpp must be added back
    # as its own TU.
    target_sources(catchchallenger_general INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/base/CommonDatapack.cpp
    )
else()
    target_sources(catchchallenger_general INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/base/Map_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/Map_loaderMain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderCrafting.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderMap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderMonsterDrop.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderPlant.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderReputation.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fight/FightLoaderBuff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fight/FightLoaderMonster.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fight/FightLoaderSkill.cpp
        # tinyXML2: three .cpp files split the upstream impl, all
        # three need to compile (no include-cpp pattern here).
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyXML2/tinyxml2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyXML2/tinyxml2b.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyXML2/tinyxml2c.cpp
    )
endif()

# Includes / defines for the full lib are inherited from _minimal via
# target_link_libraries(... INTERFACE catchchallenger_general_minimal).
# CACHE_HPS is per-consumer (per-target) — map2png and similar render
# tools include the same general/ sources but build without it.
