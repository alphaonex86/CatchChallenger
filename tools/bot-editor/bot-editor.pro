#-------------------------------------------------
#
# Project created by QtCreator 2013-07-24T18:46:12
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bot-editor
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    StepType.cpp

HEADERS  += MainWindow.h \
    StepType.h

FORMS    += MainWindow.ui \
    StepType.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

RESOURCES += resources.qrc
win32:RC_FILE += $$PWD/resources-windows.rc
