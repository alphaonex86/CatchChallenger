QT       += core gui widgets xml

include(../../client/tiled/tiled.pri)

TARGET = generate-map-border
CONFIG   += console
DEFINES += GENERATEMAPBORDER

TEMPLATE = app

LIBS += -lz

SOURCES += main.cpp
