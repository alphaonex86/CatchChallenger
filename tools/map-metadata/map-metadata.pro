include(../../general/general.pri)

QT       += core gui xml sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = map-metadata
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    ../../server/base/MapServer.cpp \
    MainWindowLoadMap.cpp
HEADERS  += MainWindow.h \
    ../../server/base/ServerStructures.h \
    ../../server/base/MapServer.h
FORMS    += MainWindow.ui

RESOURCES += \
    resources.qrc

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
