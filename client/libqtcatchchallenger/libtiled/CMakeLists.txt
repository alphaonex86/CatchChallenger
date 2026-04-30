# Embedded libtiled (Tiled map editor library).
# Mirrors client/libqtcatchchallenger/libtiled.pri — vendored sources
# only (do NOT modify upstream layout per CLAUDE.md).

find_package(Qt6 COMPONENTS Core Gui REQUIRED)
find_package(ZLIB REQUIRED)

set(_tiled_sources
    compression.cpp
    fileformat.cpp
    filesystemwatcher.cpp
    gidmapper.cpp
    grouplayer.cpp
    hex.cpp
    hexagonalrenderer.cpp
    imagecache.cpp
    imagelayer.cpp
    imagereference.cpp
    isometricrenderer.cpp
    layer.cpp
    logginginterface.cpp
    map.cpp
    mapformat.cpp
    mapobject.cpp
    mapreader.cpp
    maprenderer.cpp
    maptovariantconverter.cpp
    mapwriter.cpp
    minimaprenderer.cpp
    object.cpp
    objectgroup.cpp
    objecttemplate.cpp
    objecttemplateformat.cpp
    objecttypes.cpp
    obliquerenderer.cpp
    orthogonalrenderer.cpp
    plugin.cpp
    pluginmanager.cpp
    properties.cpp
    propertytype.cpp
    savefile.cpp
    staggeredrenderer.cpp
    templatemanager.cpp
    tile.cpp
    tileanimationdriver.cpp
    tiled.cpp
    tilelayer.cpp
    tileset.cpp
    tilesetformat.cpp
    tilesetmanager.cpp
    tmxmapformat.cpp
    varianttomapconverter.cpp
    wangset.cpp
    world.cpp
)

add_library(catchchallenger_tiled STATIC ${_tiled_sources})

target_include_directories(catchchallenger_tiled PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# TILED_ZSTD_SUPPORT and TILED_ZLIB are baked-in (always on).
# TILED_LIB_DIR is referenced by pluginmanager.cpp for plugin search;
# we never use plugins so the value is cosmetic — keep it.
# TIXML_USE_STL: vendored libtiled's tinyxml uses std::string when set
# (rather than the vendored CString). Required here because the
# upstream tinyxml interfaces compile under that mode in our tree.
target_compile_definitions(catchchallenger_tiled PUBLIC
    TILED_ZSTD_SUPPORT
    TILED_LIB_DIR="lib"
    TIXML_USE_STL
)

target_link_libraries(catchchallenger_tiled PUBLIC
    Qt6::Core
    Qt6::Gui
    ZLIB::ZLIB
)
if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger_tiled PUBLIC ${ZSTD_LIBRARY})
endif()

# tiled uses Qt's automoc.
set_target_properties(catchchallenger_tiled PROPERTIES
    AUTOMOC ON
)
