wasm: DEFINES += CATCHCHALLENGER_NOAUDIO NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
DEFINES += CATCHCHALLENGER_SOLO
TEMPLATE = app
QT += core gui widgets network sql quick websockets
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
include(base/solo.pri)
QT += multimedia
}
precompile_header:!isEmpty(PRECOMPILED_HEADER) {
DEFINES += USING_PCH
}

#DEFINES+=PROTOCOLPARSINGDEBUG
DEFINES += CATCHCHALLENGER_CLASS_QT
TARGET = catchchallenger-uselib
include(ultimate/specific.pri)
include(base/client.pri)
include(base/multi.pri)

linux:QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"

LIBS += -L../build-qtcatchchallengerclient-Desktop-Debug/ -lqtcatchchallengerclient

# CATCHCHALLENGER_CACHE_HPS -> include hps.pri
TARGET = catchchallenger
