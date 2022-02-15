DEFINES += CATCHCHALLENGER_SOLO TINYXML2_EXPORT
TEMPLATE = lib
CONFIG   += precompile_header
precompile_header:!isEmpty(PRECOMPILED_HEADER) {
DEFINES += USING_PCH
}
DEFINES += LIBEXPORT
include(libqt.pri)
include(libqtheader.pri)
include(../libcatchchallenger/lib.pri)
include(../libcatchchallenger/libheader.pri)
include(../tiled/tiled.pri)
include(../tiled/tiledheader.pri)
include(../qtmaprender/render.pri)
include(../qtmaprender/renderheader.pri)
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-server.pri)
include(../../server/catchchallenger-serverheader.pri)
include(../../server/qt/catchchallenger-server-qt.pri)
include(../../server/qt/catchchallenger-server-qtheader.pri)
}
include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)
QT       += network widgets opengl
TARGET = qtcatchchallengerclient

linux:QMAKE_LFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CXXFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
linux:QMAKE_CFLAGS+="-Wno-missing-braces -Wall -Wextra"
