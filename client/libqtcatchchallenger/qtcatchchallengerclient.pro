DEFINES += CATCHCHALLENGER_SOLO
TEMPLATE = lib
include(qtlib.pri)
include(qtlibheader.pri)
include(../libcatchchallenger/lib.pri)
include(../tiled/tiled.pri)
include(../tiled/tiledheader.pri)
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-server.pri)
include(../../server/qt/catchchallenger-server-qt.pri)
include(../../server/qt/catchchallenger-server-qtheader.pri)
}
QT       += network
TARGET = qtcatchchallengerclient
