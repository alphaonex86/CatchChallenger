wasm: DEFINES += CATCHCHALLENGER_NOAUDIO NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
DEFINES += CATCHCHALLENGER_NOAUDIO
DEFINES += CATCHCHALLENGER_SOLO
DEFINES += LIBIMPORT
TEMPLATE = app
QT += core gui widgets network sql
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
include(base/solo.pri)
QT += multimedia
}
precompile_header:!isEmpty(PRECOMPILED_HEADER) {
DEFINES += USING_PCH
}

#DEFINES+=PROTOCOLPARSINGDEBUG
DEFINES += CATCHCHALLENGER_CLASS_QT
include(ultimate/specific.pri)
include(base/client.pri)
include(base/multi.pri)

linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"
linux:LIBS += -fuse-ld=mold

LIBS += -L../build-qtcatchchallengerclient-Desktop-Debug/ -lqtcatchchallengerclient

# CATCHCHALLENGER_CACHE_HPS -> include hps.pri
TARGET = catchchallenger
