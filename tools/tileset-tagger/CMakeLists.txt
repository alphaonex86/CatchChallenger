# tileset-tagger — tag a Tiled .tsx with semantic categories for the
# learn-from-examples map generator. One self-contained binary. Qt only
# (QDomDocument for surgical .tsx edits, QImage for the untagged-pixel guard,
# Widgets for the rectangle-select GUI).

cmake_minimum_required(VERSION 3.20)
project(tileset_tagger LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Xml Gui Widgets REQUIRED)

add_executable(tileset-tagger
    main.cpp
    TagModel.cpp
    TilesetView.cpp
    MainWindow.cpp
)

target_link_libraries(tileset-tagger PRIVATE
    Qt6::Core
    Qt6::Xml
    Qt6::Gui
    Qt6::Widgets
)

set_target_properties(tileset-tagger PROPERTIES
    AUTOMOC ON
)

# Linker driver: use the test-link-verified accelerator CCCommon.cmake picked for
# this target (mold/lld/bfd). Never hard-code a linker — CCCommon is the source.
if(CC_LINKER)
    target_link_options(tileset-tagger PRIVATE -fuse-ld=${CC_LINKER})
endif()
