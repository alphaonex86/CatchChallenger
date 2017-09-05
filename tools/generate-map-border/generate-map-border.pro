QT       += core gui widgets xml

include(../../client/base/client.pri)
include(../../general/general.pri)

TARGET = generate-map-border
CONFIG   += console
DEFINES += GENERATEMAPBORDER CATCHCHALLENGER_VERSION_SOLO

TEMPLATE = app

LIBS += -lz

SOURCES += main.cpp \
    ../map-procedural-generation/PartialMap.cpp

HEADERS += \
    ../map-procedural-generation/PartialMap.h
