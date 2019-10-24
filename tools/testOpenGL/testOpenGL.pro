INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
DEFINES += CATCHCHALLENGER_NOAUDIO NOWEBSOCKET

include(../../client/qt/client.pri)
include(../../general/general.pri)
include(../../client/qt/solo.pri)
include(../../client/qt/multi.pri)
include(../../server/catchchallenger-server-qt.pri)

TEMPLATE = app
TARGET = testOpenGL

QT += xml opengl network widgets

SOURCES += main.cpp \
    MapControllerV.cpp

HEADERS += \
    MapControllerV.h

RESOURCES += \
    resources.qrc

DEFINES += NOTHREADS MAPVISUALISER
