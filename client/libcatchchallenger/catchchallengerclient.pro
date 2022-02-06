TEMPLATE = lib
DEFINES += CATCHCHALLENGER_SOLO CATCHCHALLENGERLIB
include(lib.pri)
include(libheader.pri)
include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)
QT       -= core gui
LIBS -= -lpthread
TARGET = catchchallengerclient
linux:QMAKE_LFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CXXFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"

#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-serverheader.pri)
}
else
{
}
