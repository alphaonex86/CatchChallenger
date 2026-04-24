# libopusfile - Opus file decoding library
# Embedded build for CatchChallenger (no HTTP streaming)

DEFINES += OP_DISABLE_HTTP

INCLUDEPATH += \
    $$PWD/libopusfile/include/ \
    $$PWD/libopusfile/src/

HEADERS += \
    $$PWD/libopusfile/include/opusfile.h

SOURCES += \
    $$PWD/libopusfile/src/info.c \
    $$PWD/libopusfile/src/internal.c \
    $$PWD/libopusfile/src/opusfile.c \
    $$PWD/libopusfile/src/stream.c
