INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
DEFINES += CATCHCHALLENGER_NOAUDIO NOWEBSOCKET

include(../../client/qt/client.pri)
include(../../general/general.pri)
include(../../client/qt/multi.pri)
#include(../../server/catchchallenger-server-qt.pri)

TEMPLATE = app
TARGET = testOpenGL

wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
    DEFINES -= NOWEBSOCKET
    QT += websockets
}

QT += xml opengl network widgets
QT -= sql

SOURCES += main.cpp \
    MapControllerV.cpp \
    ../../client/qt/ConnectedSocket.cpp

HEADERS += \
    MapControllerV.h \
    ../../client/qt/ConnectedSocket.h

RESOURCES += \
    resources.qrc

DEFINES += NOTHREADS MAPVISUALISER
