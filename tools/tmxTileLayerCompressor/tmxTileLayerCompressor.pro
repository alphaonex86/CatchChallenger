QT       += core xml
QT       -= gui

TARGET = tmxTileLayerCompressor
CONFIG   += c++11
CONFIG   += console
CONFIG   -= app_bundle

LIBS += -lz

TEMPLATE = app


SOURCES += main.cpp \
    zopfli/src/zopfli/blocksplitter.c \
    zopfli/src/zopfli/cache.c \
    zopfli/src/zopfli/deflate.c \
    zopfli/src/zopfli/gzip_container.c \
    zopfli/src/zopfli/hash.c \
    zopfli/src/zopfli/katajainen.c \
    zopfli/src/zopfli/lz77.c \
    zopfli/src/zopfli/squeeze.c \
    zopfli/src/zopfli/tree.c \
    zopfli/src/zopfli/util.c \
    zopfli/src/zopfli/zlib_container.c \
    zopfli/src/zopfli/zopfli_lib.c

HEADERS += \
    zopfli/src/zopfli/blocksplitter.h \
    zopfli/src/zopfli/cache.h \
    zopfli/src/zopfli/deflate.h \
    zopfli/src/zopfli/gzip_container.h \
    zopfli/src/zopfli/hash.h \
    zopfli/src/zopfli/katajainen.h \
    zopfli/src/zopfli/lz77.h \
    zopfli/src/zopfli/squeeze.h \
    zopfli/src/zopfli/tree.h \
    zopfli/src/zopfli/util.h \
    zopfli/src/zopfli/zlib_container.h \
    zopfli/src/zopfli/zopfli.h
