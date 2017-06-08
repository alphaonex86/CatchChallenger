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
    main.cpp \
    znoise/cpp/FBM.cpp \
    znoise/cpp/HybridMultiFractal.cpp \
    znoise/cpp/MixerBase.cpp \
    znoise/cpp/NoiseBase.cpp \
    znoise/cpp/NoiseTools.cpp \
    znoise/cpp/Perlin.cpp \
    znoise/cpp/Simplex.cpp \
    znoise/cpp/Worley.cpp \
    VoronioForTiledMapTmx.cpp \
    LoadMap.cpp \
    TransitionTerrain.cpp

RESOURCES += \
    resources.qrc

HEADERS += \
    PoissonGenerator.h \
    znoise/headers/Enums.hpp \
    znoise/headers/FBM.hpp \
    znoise/headers/HybridMultiFractal.hpp \
    znoise/headers/MixerBase.hpp \
    znoise/headers/NoiseBase.hpp \
    znoise/headers/NoiseTools.hpp \
    znoise/headers/Perlin.hpp \
    znoise/headers/Simplex.hpp \
    znoise/headers/Worley.hpp \
    VoronioForTiledMapTmx.h \
    LoadMap.h \
    TransitionTerrain.h

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
