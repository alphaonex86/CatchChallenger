linux:QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"

linux:QMAKE_LFLAGS += -fuse-ld=mold
linux:LIBS += -fuse-ld=mold

DEFINES += OPENGL CATCHCHALLENGER_CACHE_HPS
wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
}
DEFINES += CATCHCHALLENGER_DB_SQLITE
#DEFINES+=PROTOCOLPARSINGDEBUG
DEFINES += CATCHCHALLENGER_CLASS_QT
!contains(DEFINES, NOSINGLEPLAYER) {
#include(base/solo.pri)
    INCLUDEPATH += -I/usr/include/
    include(../../server/catchchallenger-server.pri)
    include(../../server/catchchallenger-serverheader.pri)
    include(../../server/qt/catchchallenger-server-qt.pri)
    include(../../server/qt/catchchallenger-server-qtheader.pri)
}
TARGET = catchchallenger
include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)
include(../libcatchchallenger/lib.pri)
include(../libcatchchallenger/libheader.pri)
include(../libqtcatchchallenger/libqt.pri)
include(../libqtcatchchallenger/libqtheader.pri)
include(../libqtcatchchallenger/maprender/render.pri)
include(../libqtcatchchallenger/maprender/renderheader.pri)
include(client.pri)
