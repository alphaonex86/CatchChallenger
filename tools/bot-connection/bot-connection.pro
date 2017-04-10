DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)
include(../bot/simple/Simple.pri)

QT       += core gui xml network sql
QT += widgets

TARGET = bot-connection
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    ../bot/BotInterface.cpp
HEADERS  += MainWindow.h \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h

FORMS    += MainWindow.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
