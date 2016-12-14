include(../../general/general.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)
include(../bot/actions/Actions.pri)

QT       += core gui xml network sql
QT += widgets

TARGET = bot-actions
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    BotTargetList.cpp
HEADERS  += MainWindow.h \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    BotTargetList.h

FORMS    += MainWindow.ui \
    BotTargetList.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
