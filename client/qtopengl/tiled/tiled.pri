DEFINES += TILED_ZLIB
!wasm: {
LIBS += -lz
}

HEADERS += $$PWD/tiled_compression.h \
$$PWD/tiled_gidmapper.h \
$$PWD/tiled_imagelayer.h \
$$PWD/tiled_isometricrenderer.h \
$$PWD/tiled_layer.h \
$$PWD/tiled_logginginterface.h \
$$PWD/tiled_map.h \
$$PWD/tiled_mapobject.h \
$$PWD/tiled_mapreader.h \
$$PWD/tiled_mapreaderinterface.h \
$$PWD/tiled_maprenderer.h \
$$PWD/tiled_mapwriter.h \
$$PWD/tiled_mapwriterinterface.h \
$$PWD/tiled_object.h \
$$PWD/tiled_objectgroup.h \
$$PWD/tiled_orthogonalrenderer.h \
$$PWD/tiled_properties.h \
$$PWD/tiled_staggeredrenderer.h \
$$PWD/tiled_terrain.h \
$$PWD/tiled_tile.h \
$$PWD/tiled_tiled.h \
$$PWD/tiled_tilelayer.h \
$$PWD/tiled_tileset.h

SOURCES += $$PWD/tiled_compression.cpp \
$$PWD/tiled_gidmapper.cpp \
$$PWD/tiled_imagelayer.cpp \
$$PWD/tiled_isometricrenderer.cpp \
$$PWD/tiled_layer.cpp \
$$PWD/tiled_map.cpp \
$$PWD/tiled_mapobject.cpp \
$$PWD/tiled_mapreader.cpp \
$$PWD/tiled_maprenderer.cpp \
$$PWD/tiled_mapwriter.cpp \
$$PWD/tiled_objectgroup.cpp \
$$PWD/tiled_orthogonalrenderer.cpp \
$$PWD/tiled_properties.cpp \
$$PWD/tiled_staggeredrenderer.cpp \
$$PWD/tiled_tile.cpp \
$$PWD/tiled_tilelayer.cpp \
$$PWD/tiled_tileset.cpp

