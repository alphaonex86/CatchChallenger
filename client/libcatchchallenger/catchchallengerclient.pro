TEMPLATE = lib
DEFINES += CATCHCHALLENGER_SOLO CATCHCHALLENGERLIB
include(lib.pri)
include(libheader.pri)
include(../../general/general.pri)
QT       -= core gui
LIBS -= -lpthread
TARGET = catchchallengerclient
linux:QMAKE_LFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CXXFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"

DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2c.cpp
HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.h

#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-serverheader.pri)
}
else
{
}
