DEFINES += TILED_ZLIB
!wasm: {
LIBS += -lz
}

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

