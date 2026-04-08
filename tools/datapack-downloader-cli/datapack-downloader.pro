DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)
include(../../client/libcatchchallenger/lib.pri)
include(../../client/libcatchchallenger/libheader.pri)
include(../../client/libqtcatchchallenger/libqt.pri)
include(../../client/libqtcatchchallenger/libqtheader.pri)

QT       += core network sql websockets
QT       -= gui widgets

TARGET = datapack-downloader
TEMPLATE = app

SOURCES += main.cpp\
        MainWindow.cpp
HEADERS  += MainWindow.h

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
