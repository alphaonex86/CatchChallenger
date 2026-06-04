# gba2catchchallenger — convert a Gen3 GBA ROM into a CatchChallenger datapack
# region (maps + rendered tilesets).  Self-contained, one binary.  Pulls
# catchchallenger_general (datapack base, zstd, tinyxml2) and compiles the
# client datapack loader so the base datapack is read exactly like the client.

cmake_minimum_required(VERSION 3.20)
project(gba2catchchallenger LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)

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
    Qt6::Core
    Qt6::Gui
    ${ZSTD_LIBRARY}
)

# Linker driver: use the test-link-verified accelerator that CCCommon.cmake
# picked for THIS target.  Never hard-code a linker — CCCommon is the source
# of truth.
if(CC_LINKER)
    target_link_options(gba2catchchallenger PRIVATE -fuse-ld=${CC_LINKER})
endif()
