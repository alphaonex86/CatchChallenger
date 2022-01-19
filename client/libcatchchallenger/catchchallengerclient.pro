DEFINES += CATCHCHALLENGER_SOLO
include(lib.pri)
include(libheader.pri)
QT       -= core gui
LIBS -= -lpthread
TARGET = catchchallengerclient
TEMPLATE = lib
linux:QMAKE_LFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
linux:QMAKE_CXXFLAGS+="-fvisibility=hidden -fvisibility-inlines-hidden"
