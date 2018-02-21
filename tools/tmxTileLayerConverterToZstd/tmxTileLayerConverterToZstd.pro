QT       += core xml
QT       -= gui

TARGET = tmxTileLayerConverterToZstd
CONFIG   += c++11
CONFIG   += console
CONFIG   -= app_bundle

LIBS += -lz -lzstd

TEMPLATE = app


SOURCES += main.cpp
