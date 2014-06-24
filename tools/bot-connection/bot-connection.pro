include(../../general/general.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)

QT       += core gui xml network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bot-connection
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp
HEADERS  += MainWindow.h

FORMS    += MainWindow.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
