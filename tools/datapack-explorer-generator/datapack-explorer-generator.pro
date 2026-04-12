TEMPLATE = app
TARGET = datapack-explorer-generator

CONFIG += console c++17
CONFIG -= app_bundle

QT += core xml gui
QT -= widgets

DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2
DEFINES += NOWEBSOCKET ONLYMAPRENDER
DEFINES += NOTHREADS
DEFINES += CATCHCHALLENGER_CLIENT
DEFINES += CATCHCHALLENGER_NOAUDIO

QMAKE_CXXFLAGS += -fstack-protector-all -g
DEFINES += SRC_DIR=\\\"$$PWD\\\"

LIBS += -ltiled
INCLUDEPATH += /usr/include/tiled/

include(../../general/general.pri)

SOURCES += \
    main.cpp \
    Helper.cpp \
    MapStore.cpp \
    Generator.cpp \
    GeneratorItems.cpp \
    GeneratorMonsters.cpp \
    GeneratorPlants.cpp \
    GeneratorSkills.cpp \
    GeneratorTypes.cpp \
    GeneratorBuffs.cpp \
    GeneratorCrafting.cpp \
    GeneratorIndustries.cpp \
    GeneratorMaps.cpp \
    GeneratorQuests.cpp \
    GeneratorIndex.cpp \
    GeneratorStart.cpp \
    GeneratorTree.cpp \
    ../../client/libcatchchallenger/DatapackChecksum.cpp \
    ../../client/libcatchchallenger/DatapackClientLoader.cpp \
    ../../client/libqtcatchchallenger/Language.cpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoader.cpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoaderThread.cpp \
    ../../client/libqtcatchchallenger/Settings.cpp

HEADERS += \
    Helper.hpp \
    MapStore.hpp \
    Generator.hpp \
    GeneratorItems.hpp \
    GeneratorMonsters.hpp \
    GeneratorPlants.hpp \
    GeneratorSkills.hpp \
    GeneratorTypes.hpp \
    GeneratorBuffs.hpp \
    GeneratorCrafting.hpp \
    GeneratorIndustries.hpp \
    GeneratorMaps.hpp \
    GeneratorQuests.hpp \
    GeneratorIndex.hpp \
    GeneratorStart.hpp \
    GeneratorTree.hpp \
    ../../client/libcatchchallenger/DatapackChecksum.hpp \
    ../../client/libcatchchallenger/DatapackClientLoader.hpp \
    ../../client/libqtcatchchallenger/Language.hpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoaderThread.hpp \
    ../../client/libqtcatchchallenger/Settings.hpp

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2c.cpp
