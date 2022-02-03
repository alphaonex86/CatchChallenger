include(../../general/general.pri)
include(../../server/catchchallenger-server.pri)

QT       += core network
QT       -= gui widgets

TARGET = datapack-downloader
TEMPLATE = app

SOURCES += main.cpp\
        MainWindow.cpp \
    ../../client/base/Api_client_real.cpp
HEADERS  += MainWindow.h \
    ../../client/base/Api_client_real.h

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
