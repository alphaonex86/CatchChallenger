# Embedded libtiled library (from Tiled map editor)
# Replaces: LIBS += -ltiled

QT += gui

# For #include "map.h" style (used by maprender/ and libtiled/ internally)
INCLUDEPATH += $$PWD/libtiled/
# For #include <libtiled/map.h> style (via $$PWD include path)
INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/libtiled/

# Embedded as static source, not a shared library: make TILEDSHARED_EXPORT empty
DEFINES += TILED_LIBRARY

# Enable zstd compression support in libtiled (compression.cpp uses <zstd.h>)
DEFINES += TILED_ZSTD_SUPPORT

# zlib is required by libtiled compression (gzip/deflate support)
LIBS += -lz
DEFINES += 'TILED_LIB_DIR=\\\"lib\\\"'

SOURCES += \
    $$PWD/libtiled/compression.cpp \
    $$PWD/libtiled/fileformat.cpp \
    $$PWD/libtiled/filesystemwatcher.cpp \
    $$PWD/libtiled/gidmapper.cpp \
    $$PWD/libtiled/grouplayer.cpp \
    $$PWD/libtiled/hex.cpp \
    $$PWD/libtiled/hexagonalrenderer.cpp \
    $$PWD/libtiled/imagecache.cpp \
    $$PWD/libtiled/imagelayer.cpp \
    $$PWD/libtiled/imagereference.cpp \
    $$PWD/libtiled/isometricrenderer.cpp \
    $$PWD/libtiled/layer.cpp \
    $$PWD/libtiled/logginginterface.cpp \
    $$PWD/libtiled/map.cpp \
    $$PWD/libtiled/mapformat.cpp \
    $$PWD/libtiled/mapobject.cpp \
    $$PWD/libtiled/mapreader.cpp \
    $$PWD/libtiled/maprenderer.cpp \
    $$PWD/libtiled/maptovariantconverter.cpp \
    $$PWD/libtiled/mapwriter.cpp \
    $$PWD/libtiled/minimaprenderer.cpp \
    $$PWD/libtiled/object.cpp \
    $$PWD/libtiled/objectgroup.cpp \
    $$PWD/libtiled/objecttemplate.cpp \
    $$PWD/libtiled/objecttemplateformat.cpp \
    $$PWD/libtiled/objecttypes.cpp \
    $$PWD/libtiled/obliquerenderer.cpp \
    $$PWD/libtiled/orthogonalrenderer.cpp \
    $$PWD/libtiled/plugin.cpp \
    $$PWD/libtiled/pluginmanager.cpp \
    $$PWD/libtiled/properties.cpp \
    $$PWD/libtiled/propertytype.cpp \
    $$PWD/libtiled/savefile.cpp \
    $$PWD/libtiled/staggeredrenderer.cpp \
    $$PWD/libtiled/templatemanager.cpp \
    $$PWD/libtiled/tile.cpp \
    $$PWD/libtiled/tileanimationdriver.cpp \
    $$PWD/libtiled/tiled.cpp \
    $$PWD/libtiled/tilelayer.cpp \
    $$PWD/libtiled/tileset.cpp \
    $$PWD/libtiled/tilesetformat.cpp \
    $$PWD/libtiled/tilesetmanager.cpp \
    $$PWD/libtiled/tmxmapformat.cpp \
    $$PWD/libtiled/varianttomapconverter.cpp \
    $$PWD/libtiled/wangset.cpp \
    $$PWD/libtiled/world.cpp

HEADERS += \
    $$PWD/libtiled/compression.h \
    $$PWD/libtiled/containerhelpers.h \
    $$PWD/libtiled/fileformat.h \
    $$PWD/libtiled/filesystemwatcher.h \
    $$PWD/libtiled/gidmapper.h \
    $$PWD/libtiled/grid.h \
    $$PWD/libtiled/grouplayer.h \
    $$PWD/libtiled/hex.h \
    $$PWD/libtiled/hexagonalrenderer.h \
    $$PWD/libtiled/imagecache.h \
    $$PWD/libtiled/imagelayer.h \
    $$PWD/libtiled/imagereference.h \
    $$PWD/libtiled/isometricrenderer.h \
    $$PWD/libtiled/layer.h \
    $$PWD/libtiled/logginginterface.h \
    $$PWD/libtiled/map.h \
    $$PWD/libtiled/mapformat.h \
    $$PWD/libtiled/mapobject.h \
    $$PWD/libtiled/mapreader.h \
    $$PWD/libtiled/maprenderer.h \
    $$PWD/libtiled/maptovariantconverter.h \
    $$PWD/libtiled/mapwriter.h \
    $$PWD/libtiled/minimaprenderer.h \
    $$PWD/libtiled/object.h \
    $$PWD/libtiled/objectgroup.h \
    $$PWD/libtiled/objecttemplate.h \
    $$PWD/libtiled/objecttemplateformat.h \
    $$PWD/libtiled/objecttypes.h \
    $$PWD/libtiled/obliquerenderer.h \
    $$PWD/libtiled/orthogonalrenderer.h \
    $$PWD/libtiled/plugin.h \
    $$PWD/libtiled/pluginmanager.h \
    $$PWD/libtiled/properties.h \
    $$PWD/libtiled/propertytype.h \
    $$PWD/libtiled/savefile.h \
    $$PWD/libtiled/staggeredrenderer.h \
    $$PWD/libtiled/templatemanager.h \
    $$PWD/libtiled/tile.h \
    $$PWD/libtiled/tileanimationdriver.h \
    $$PWD/libtiled/tiled.h \
    $$PWD/libtiled/tiled_global.h \
    $$PWD/libtiled/tilelayer.h \
    $$PWD/libtiled/tileset.h \
    $$PWD/libtiled/tilesetformat.h \
    $$PWD/libtiled/tilesetmanager.h \
    $$PWD/libtiled/tmxmapformat.h \
    $$PWD/libtiled/varianttomapconverter.h \
    $$PWD/libtiled/wangset.h \
    $$PWD/libtiled/world.h
