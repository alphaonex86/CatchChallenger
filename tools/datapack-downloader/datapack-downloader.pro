include(../../general/general.pri)
include(../../server/catchchallenger-server.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)

QT       += core network
QT       -= gui widgets

TARGET = datapack-downloader
TEMPLATE = app

SOURCES += main.cpp\
        MainWindow.cpp
HEADERS  += MainWindow.h

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
