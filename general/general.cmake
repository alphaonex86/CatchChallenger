# general/general.cmake — mirrors the qmake-era general/general.pri.
#
# A binary's CMakeLists.txt include()s this file to pull in the shared
# general/ sources as INTERFACE libs:
#   include(${CMAKE_CURRENT_LIST_DIR}/<rel>/general/general.cmake)
# then links catchchallenger_general or catchchallenger_general_minimal.
#
# Two layered INTERFACE libs:
#   catchchallenger_general_minimal — class-agnostic networking + hashing +
#       settings + tinyXML2. Safe for LOGIN / MASTER / GATEWAY.
#   catchchallenger_general — adds map / datapack / fight loaders. Used by
#       game servers (cli-epoll-filedb, game-server-alone) and clients.
#
# Re-include guard: include() doesn't have a built-in one, and the libs
# below cannot be added twice.
if(NOT TARGET catchchallenger_general_minimal)
    add_library(catchchallenger_general_minimal INTERFACE)

    target_sources(catchchallenger_general_minimal INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/base/ProtocolParsingGeneral.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ProtocolParsingInput.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ProtocolParsingOutput.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ProtocolParsingCheck.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CompressionProtocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/FacilityLibGeneral.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonSettingsCommon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonSettingsServer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/cpp11addition.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/cpp11additionstringtointcpp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/cpp11additionstringtointc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/Version.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CatchChallenger_Hash.cpp
    )
    if(CC_USE_SYSTEM_BLAKE3)
        target_link_libraries(catchchallenger_general_minimal INTERFACE ${BLAKE3_LIBRARY})
        target_include_directories(catchchallenger_general_minimal INTERFACE ${BLAKE3_INCLUDE_DIR})
    else()
        target_sources(catchchallenger_general_minimal INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/blake3/blake3.c
            ${CMAKE_CURRENT_LIST_DIR}/blake3/blake3_dispatch.c
            ${CMAKE_CURRENT_LIST_DIR}/blake3/blake3_portable.c
        )
        # Add general/blake3 to the include path so call sites can use
        # `<blake3.h>` regardless of whether the lib is system or vendored.
        target_include_directories(catchchallenger_general_minimal INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/blake3)
    endif()
    if(CC_USE_SYSTEM_XXHASH)
        target_link_libraries(catchchallenger_general_minimal INTERFACE ${XXHASH_LIBRARY})
        target_include_directories(catchchallenger_general_minimal INTERFACE ${XXHASH_INCLUDE_DIR})
    else()
        target_sources(catchchallenger_general_minimal INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/libxxhash/xxhash.c
        )
        target_include_directories(catchchallenger_general_minimal INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/libxxhash)
        # XXH_INLINE_ALL only makes sense with the vendored header; the
        # system header expects to resolve the symbols against -lxxhash.
        target_compile_definitions(catchchallenger_general_minimal INTERFACE
            XXH_INLINE_ALL)
    endif()
    if(NOT CATCHCHALLENGER_NOXML)
        if(CC_USE_SYSTEM_TINYXML2)
            target_link_libraries(catchchallenger_general_minimal INTERFACE ${TINYXML2_LIBRARY})
            target_include_directories(catchchallenger_general_minimal INTERFACE
                ${TINYXML2_INCLUDE_DIR}
                ${CC_TINYXML2_SHIM_DIR})
        else()
            target_sources(catchchallenger_general_minimal INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/tinyXML2/tinyxml2.cpp
                ${CMAKE_CURRENT_LIST_DIR}/tinyXML2/tinyxml2b.cpp
                ${CMAKE_CURRENT_LIST_DIR}/tinyXML2/tinyxml2c.cpp
            )
        endif()
    endif()
    target_include_directories(catchchallenger_general_minimal INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
    )
    if(EXTERNALLIBZSTD)
        target_compile_definitions(catchchallenger_general_minimal INTERFACE EXTERNALLIBZSTD)
    endif()
    if(CATCHCHALLENGER_HARDENED)
        # Propagated as an INTERFACE define so every binary that links
        # catchchallenger_general_minimal (server-cli, gateway, login,
        # master, clients) picks up the abort-on-invariant and
        # abort-on-parse-fail guards in ProtocolParsingInput.cpp and
        # the server/general code. Without this, the option was only
        # propagated by catchchallenger-server.cmake (server-only),
        # so client/library callers of the parse* helpers silently
        # skipped the guard — defeating the point of HARDENED for
        # the parse layer specifically.
        target_compile_definitions(catchchallenger_general_minimal INTERFACE
            CATCHCHALLENGER_HARDENED)
    endif()
    # Propagate zstd + zlib link deps to all consumers regardless of source —
    # the general sources call ZSTD_*/inflate/deflate from CompressionProtocol.cpp
    # and the tiled-style map decoder. ${ZSTD_LIBRARY} is set by
    # general/CCCommon.cmake (system libzstd or vendored libzstd_static).
    target_link_libraries(catchchallenger_general_minimal INTERFACE
        ${ZSTD_LIBRARY}
        ZLIB::ZLIB
    )
    if(CATCHCHALLENGER_CACHE_HPS)
        target_include_directories(catchchallenger_general_minimal INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/hps)
    endif()
    if(CATCHCHALLENGER_NOXML)
        target_compile_definitions(catchchallenger_general_minimal INTERFACE
            CATCHCHALLENGER_NOXML)
    endif()
endif()

if(NOT TARGET catchchallenger_general)
    add_library(catchchallenger_general INTERFACE)
    target_link_libraries(catchchallenger_general INTERFACE catchchallenger_general_minimal)

    target_sources(catchchallenger_general INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/base/GeneralStructures.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/MoveOnTheMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/FacilityLib.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/FightLoader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonMap/BaseMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonMap/CommonMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonMap/ItemOnMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonMap/Map_Border.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonMap/Teleporter.cpp
        # CommonDatapack.cpp is intentionally NOT listed: it's #include'd
        # at file scope by DatapackGeneralLoaderMap.cpp (see the
        # CATCHCHALLENGER_NOXML branch below for the alternative).
        ${CMAKE_CURRENT_LIST_DIR}/base/CommonDatapackServerSpec.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngineEnd.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngineTurn.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngineBuff.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngineSkill.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngineWild.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fight/CommonFightEngineBase.cpp
    )

    if(CATCHCHALLENGER_NOXML)
        target_sources(catchchallenger_general INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/base/CommonDatapack.cpp
        )
    else()
        target_sources(catchchallenger_general INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/base/Map_loader.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/Map_loaderMain.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoader.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderCrafting.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderMap.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderMonsterDrop.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderPlant.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp
            ${CMAKE_CURRENT_LIST_DIR}/base/DatapackGeneralLoader/DatapackGeneralLoaderReputation.cpp
            ${CMAKE_CURRENT_LIST_DIR}/fight/FightLoaderBuff.cpp
            ${CMAKE_CURRENT_LIST_DIR}/fight/FightLoaderMonster.cpp
            ${CMAKE_CURRENT_LIST_DIR}/fight/FightLoaderSkill.cpp
        )
        # tinyxml2 sources only when system lib not used (already added
        # to catchchallenger_general_minimal otherwise; the duplicate
        # listing here mirrored the legacy non-NOXML path).
        if(NOT CC_USE_SYSTEM_TINYXML2)
            target_sources(catchchallenger_general INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/tinyXML2/tinyxml2.cpp
                ${CMAKE_CURRENT_LIST_DIR}/tinyXML2/tinyxml2b.cpp
                ${CMAKE_CURRENT_LIST_DIR}/tinyXML2/tinyxml2c.cpp
            )
        endif()
    endif()
endif()
