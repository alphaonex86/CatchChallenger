include(../../general/general.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)
include(../bot/simple/Simple.pri)

QT       += core xml network sql
QT -= gui widgets

TARGET = bot-test-connect-to-gameserver
TEMPLATE = app


SOURCES += main.cpp\
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    GlobalControler.cpp
HEADERS  += \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    GlobalControler.h

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
