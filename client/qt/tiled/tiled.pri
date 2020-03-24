DEFINES += TILED_ZLIB
!wasm: {
LIBS += -lz
}

HEADERS += $$PWD/tiled_compression.hpp \
$$PWD/tiled_gidmapper.hpp \
$$PWD/tiled_imagelayer.hpp \
$$PWD/tiled_isometricrenderer.hpp \
$$PWD/tiled_layer.hpp \
$$PWD/tiled_logginginterface.hpp \
$$PWD/tiled_map.hpp \
$$PWD/tiled_mapobject.hpp \
$$PWD/tiled_mapreader.hpp \
$$PWD/tiled_mapreaderinterface.hpp \
$$PWD/tiled_maprenderer.hpp \
$$PWD/tiled_mapwriter.hpp \
$$PWD/tiled_mapwriterinterface.hpp \
$$PWD/tiled_object.hpp \
$$PWD/tiled_objectgroup.hpp \
$$PWD/tiled_orthogonalrenderer.hpp \
$$PWD/tiled_properties.hpp \
$$PWD/tiled_staggeredrenderer.hpp \
$$PWD/tiled_terrain.hpp \
$$PWD/tiled_tile.hpp \
$$PWD/tiled_tiled.hpp \
$$PWD/tiled_tilelayer.hpp \
$$PWD/tiled_tileset.hpp

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

