TEMPLATE = lib
DEFINES += CATCHCHALLENGER_SOLO CATCHCHALLENGERLIB TINYXML2_EXPORT
DEFINES += LIBEXPORT
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
linux:QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
linux:QMAKE_CFLAGS+="-Wno-missing-braces -Wall -Wextra"

#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-serverheader.pri)
}
else
{
}
