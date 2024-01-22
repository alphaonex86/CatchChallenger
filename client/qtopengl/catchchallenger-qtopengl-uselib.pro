wasm: DEFINES += CATCHCHALLENGER_NOAUDIO NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
DEFINES += CATCHCHALLENGER_SOLO CATCHCHALLENGER_DB_SQLITE
DEFINES += LIBIMPORT
TEMPLATE = app
QT += core gui widgets network sql quick websockets
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
}
linux:QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"

DEFINES += OPENGL CATCHCHALLENGER_CACHE_HPS
wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
}
precompile_header:!isEmpty(PRECOMPILED_HEADER) {
DEFINES += USING_PCH
}
#DEFINES+=PROTOCOLPARSINGDEBUG
DEFINES += CATCHCHALLENGER_CLASS_QT
TARGET = catchchallenger
include(client.pri)
include(../libqtcatchchallenger/libqtheader.pri)
linux:LIBS += -fuse-ld=mold

LIBS += -L../build-qtcatchchallengerclient-Desktop-Debug/ -lqtcatchchallengerclient
