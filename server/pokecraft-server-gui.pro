#-------------------------------------------------
#
# Project created by QtCreator 2011-08-14T19:50:23
#
#-------------------------------------------------

QT       += core gui network xml sql

TARGET = pokecraft-server-gui
TEMPLATE = app

CONFIG   += console

SOURCES += main.cpp\
	MainWindow.cpp \
    pokecraft-server/EventDispatcher.cpp \
    pokecraft-general/DebugClass.cpp \
    pokecraft-server/EventThreader.cpp \
    pokecraft-server/Client.cpp \
    pokecraft-server/ClientHeavyLoad.cpp \
    pokecraft-server/ClientNetworkWrite.cpp \
    pokecraft-server/ClientNetworkRead.cpp \
    pokecraft-server/ClientBroadCast.cpp \
    pokecraft-server/ClientMapManagement.cpp \
    pokecraft-server/Map_loader.cpp \
    pokecraft-client/pokecraft-client-chat.cpp \
    pokecraft-server/LatencyChecker.cpp \
    pokecraft-server/FakeBot.cpp \
    pokecraft-general/MoveClient.cpp

HEADERS += \
    pokecraft-server/EventDispatcher.h \
    pokecraft-general/DebugClass.h \
    pokecraft-server/ServerStructures.h \
    pokecraft-server/EventThreader.h \
    pokecraft-server/Client.h \
    pokecraft-server/ClientHeavyLoad.h \
    pokecraft-server/ClientNetworkWrite.h \
    pokecraft-server/ClientNetworkRead.h \
    pokecraft-server/ClientBroadCast.h \
    pokecraft-server/ClientMapManagement.h \
    pokecraft-server/Map_loader.h \
    pokecraft-general/GeneralStructures.h \
    pokecraft-client/pokecraft-client-chat.h \
    pokecraft-server/LatencyChecker.h \
    VariableServer.h \
    pokecraft-server/FakeBot.h \
    pokecraft-general/MoveClient.h \
    pokecraft-client/ClientStructures.h

HEADERS  += MainWindow.h

FORMS    += MainWindow.ui

RESOURCES += \
    resources.qrc \
    resources-server.qrc

win32:RC_FILE += resources-windows.rc
