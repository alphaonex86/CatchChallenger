include(../../client/tiled/tiled.pri)

TEMPLATE = app
TARGET = map-procedural-generation-terrain

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"

QT += xml

DEFINES += ONLYMAPRENDER

include(../../general/general.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += \
    src/Diagram.cpp \
    src/Edge.cpp \
    src/Point2.cpp \
    src/Vector2.cpp \
    src/VoronoiDiagramGenerator.cpp \
    src/BeachLine.cpp \
    src/Cell.cpp \
    src/CircleEventQueue.cpp \
    main.cpp

RESOURCES += \
    resources.qrc

HEADERS += \
    include/Cell.h \
    include/Diagram.h \
    include/Edge.h \
    include/VoronoiDiagramGenerator.h \
    include/Point2.h \
    include/Vector2.h \
    src/CircleEventQueue.h \
    src/Epsilon.h \
    src/RBTree.h \
    src/BeachLine.h \
    src/MemoryPool/StackAlloc.h \
    src/MemoryPool/C-11/MemoryPool.h \
    PoissonGenerator.h

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
