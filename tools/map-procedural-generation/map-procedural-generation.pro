include(../map-procedural-generation-terrain/map-procedural-generation-terrain.pri)

TARGET = map-procedural-generation

CONFIG += debug

# libtiled already included via map-procedural-generation-terrain.pri

DEFINES += MAPPROCEDURALGENFULL

SOURCES += \
    $$PWD/main.cpp \
    SettingsAll.cpp \
    LoadMapAll.cpp \
    LoadMapCity.cpp \
    PartialMap.cpp \
    MiniMapAll.cpp \
    LoadMapRoad.cpp

HEADERS += \
    SettingsAll.h \
    LoadMapAll.h \
    PartialMap.h \
    MiniMapAll.h
