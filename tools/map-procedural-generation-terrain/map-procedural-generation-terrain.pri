include(../../client/tiled/tiled.pri)

TEMPLATE = app
TARGET = map-procedural-generation-terrain

QMAKE_CXXFLAGS+="-std=c++0x -g"
QMAKE_CFLAGS += -fno-omit-frame-pointer -g
QMAKE_CXXFLAGS += -fno-omit-frame-pointer -g

QT += xml

DEFINES += ONLYMAPRENDER

include(../../general/general.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += \
    $$PWD/znoise/cpp/FBM.cpp \
    $$PWD/znoise/cpp/HybridMultiFractal.cpp \
    $$PWD/znoise/cpp/MixerBase.cpp \
    $$PWD/znoise/cpp/NoiseBase.cpp \
    $$PWD/znoise/cpp/NoiseTools.cpp \
    $$PWD/znoise/cpp/Perlin.cpp \
    $$PWD/znoise/cpp/Simplex.cpp \
    $$PWD/znoise/cpp/Worley.cpp \
    $$PWD/VoronioForTiledMapTmx.cpp \
    $$PWD/LoadMap.cpp \
    $$PWD/TransitionTerrain.cpp \
    $$PWD/Settings.cpp \
    $$PWD/MapPlants.cpp \
    $$PWD/MiniMap.cpp \
    $$PWD/MapBrush.cpp

RESOURCES += \
    $$PWD/resources.qrc

HEADERS += \
    $$PWD/PoissonGenerator.h \
    $$PWD/znoise/headers/Enums.hpp \
    $$PWD/znoise/headers/FBM.hpp \
    $$PWD/znoise/headers/HybridMultiFractal.hpp \
    $$PWD/znoise/headers/MixerBase.hpp \
    $$PWD/znoise/headers/NoiseBase.hpp \
    $$PWD/znoise/headers/NoiseTools.hpp \
    $$PWD/znoise/headers/Perlin.hpp \
    $$PWD/znoise/headers/Simplex.hpp \
    $$PWD/znoise/headers/Worley.hpp \
    $$PWD/VoronioForTiledMapTmx.h \
    $$PWD/LoadMap.h \
    $$PWD/TransitionTerrain.h \
    $$PWD/Settings.h \
    $$PWD/MapPlants.h \
    $$PWD/MiniMap.h \
    $$PWD/MapBrush.h

LIBS += -lboost_system

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML1
#DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2 not done into client

defined(CATCHCHALLENGER_XLMPARSER_TINYXML1)
{
    DEFINES += TIXML_USE_STL
    HEADERS += $$PWD/../../general/base/tinyXML/tinystr.h \
        $$PWD/../../general/base/tinyXML/tinyxml.h

    SOURCES += $$PWD/../../general/base/tinyXML/tinystr.cpp \
        $$PWD/../../general/base/tinyXML/tinyxml.cpp \
        $$PWD/../../general/base/tinyXML/tinyxmlerror.cpp \
        $$PWD/../../general/base/tinyXML/tinyxmlparser.cpp
}
defined(CATCHCHALLENGER_XLMPARSER_TINYXML2)
{
    HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
    SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp
}
