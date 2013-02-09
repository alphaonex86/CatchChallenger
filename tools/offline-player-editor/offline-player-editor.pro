#-------------------------------------------------
#
# Project created by QtCreator 2012-11-14T16:54:19
#
#-------------------------------------------------

QT       += core gui network xml sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = offline-player-editor
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    ../../client/base/interface/DatapackClientLoader.cpp \
    ItemDialog.cpp

HEADERS  += MainWindow.h \
    ../../client/base/interface/DatapackClientLoader.h \
    ItemDialog.h

FORMS    += MainWindow.ui \
    ItemDialog.ui

RESOURCES += \
    resources.qrc

win32:RC_FILE += resources-windows.rc
win32:RESOURCES += ../../server/base/resources/resources-windows-qt-plugin.qrc
