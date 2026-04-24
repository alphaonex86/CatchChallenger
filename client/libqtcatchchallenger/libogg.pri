# libogg - Ogg bitstream format library
# Embedded build for CatchChallenger

INCLUDEPATH += $$PWD/libogg/include/

HEADERS += \
    $$PWD/libogg/include/ogg/ogg.h \
    $$PWD/libogg/include/ogg/os_types.h

SOURCES += \
    $$PWD/libogg/src/bitwise.c \
    $$PWD/libogg/src/framing.c
