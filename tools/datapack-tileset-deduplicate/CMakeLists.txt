# datapack-tileset-deduplicate — find duplicate 16x16 tiles across a datapack's
# tilesets, merge them and rewrite every .tmx reference. Self-contained, one
# binary. Pulls in libtiled (catchchallenger_tiled) for TMX/TSX read+write.

cmake_minimum_required(VERSION 3.20)
project(datapack-tileset-deduplicate LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)

add_executable(datapack-tileset-deduplicate
    main.cpp
    TileDeduplicator.cpp
)

set_target_properties(datapack-tileset-deduplicate PROPERTIES
    AUTOMOC ON
)

target_link_libraries(datapack-tileset-deduplicate PRIVATE
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Linker driver: use the test-link-verified accelerator that CCCommon.cmake
# picked for THIS target (mold on native ELF, lld/bfd elsewhere). Never
# hard-code a linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(datapack-tileset-deduplicate PRIVATE -fuse-ld=${CC_LINKER})
endif()
