include(lib.pri)
include(../../general/general.pri)

DEFINES += CATCHCHALLENGERLIB

TARGET = catchchallengerclient
TEMPLATE = lib

HEADERS += \
    DatapackChecksum.h \
    DatapackClientLoader.h \
    DisplayStructures.h \
    ClientVariableAudio.h \
    ClientVariable.h \
    ClientStructures.h

SOURCES += \
    DatapackClientLoader.cpp
