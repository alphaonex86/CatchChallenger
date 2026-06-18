# gba2catchchallenger — convert a Gen3 GBA ROM into a CatchChallenger datapack
# region (maps + rendered tilesets).  Self-contained, one binary.  Pulls
# catchchallenger_general (datapack base, zstd, tinyxml2) and compiles the
# client datapack loader so the base datapack is read exactly like the client.

cmake_minimum_required(VERSION 3.20)
project(gba2catchchallenger LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
# libtiled (catchchallenger_tiled): the render-based layer-visibility guard loads
# each written .tmx and renders it with libtiled — the same renderer map2png/the
# client use — to verify every layer is visible.  Self-contained include (mirrors
# tools/map2png); it find_package(ZLIB) itself and propagates its PUBLIC header dir.
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)

add_executable(gba2catchchallenger
    main.cpp
    Gba.cpp
    Decoder.cpp
    Gen3Text.cpp
    Gen3Script.cpp
    Naming.cpp
    Wild.cpp
    TilesetBuilder.cpp
    OverworldSprite.cpp
    SkinResolver.cpp
    CCWriter.cpp
    Gen3Data.cpp
    FullWriter.cpp
    SpriteRipper.cpp
    M4aRipper.cpp
    GsfWriter.cpp
    ${CC_REPO_ROOT}/general/mp2k/Mp2kPlayer.cpp
    ${CC_REPO_ROOT}/general/mp2k/Gsf.cpp
    DatapackBase.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/DatapackClientLoader.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/DatapackChecksum.cpp
)

target_include_directories(gba2catchchallenger PRIVATE
    ${CC_REPO_ROOT}/client/libcatchchallenger
    ${CC_REPO_ROOT}/general/base
)

set_target_properties(gba2catchchallenger PROPERTIES
    AUTOMOC ON
)

target_link_libraries(gba2catchchallenger PRIVATE
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    ${ZSTD_LIBRARY}
    ZLIB::ZLIB   # GSF container (general/mp2k/Gsf.cpp) — zlib deflate + crc32
)

# Linker driver: use the test-link-verified accelerator that CCCommon.cmake
# picked for THIS target.  Never hard-code a linker — CCCommon is the source
# of truth.
if(CC_LINKER)
    target_link_options(gba2catchchallenger PRIVATE -fuse-ld=${CC_LINKER})
endif()
