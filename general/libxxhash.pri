# object_parallel_to_source: avoid xxhash.o basename collision with libzstd's bundled xxhash.c
CONFIG += object_parallel_to_source

INCLUDEPATH += $$PWD/libxxhash/

SOURCES += $$PWD/libxxhash/xxhash.c

HEADERS += $$PWD/libxxhash/xxhash.h

DEFINES += XXH_INLINE_ALL
