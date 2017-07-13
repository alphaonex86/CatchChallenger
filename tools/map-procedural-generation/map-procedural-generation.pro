include(../map-procedural-generation-terrain/map-procedural-generation-terrain.pri)

TARGET = map-procedural-generation

DEFINES += MAPPROCEDURALGENFULL

SOURCES += \
    $$PWD/main.cpp \
    SettingsAll.cpp \
    LoadMapAll.cpp \
    LoadMapCity.cpp

HEADERS += \
    SettingsAll.h \
    LoadMapAll.h
