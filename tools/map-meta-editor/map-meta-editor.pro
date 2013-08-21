#-------------------------------------------------
#
# Project created by QtCreator 2013-07-24T18:46:12
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = map-meta-editor
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp

HEADERS  += MainWindow.h

FORMS    += MainWindow.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

RESOURCES += resources.qrc
win32:RC_FILE += $$PWD/resources-windows.rc
