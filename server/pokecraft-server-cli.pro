#-------------------------------------------------
#
# Project created by QtCreator 2011-08-01T13:49:06
#
#-------------------------------------------------

QT       += core network xml sql
QT       -= gui

TARGET = pokecraft-server-cli
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main-cli.cpp \
    ProcessControler.cpp \
    pokecraft-server/Map_loader.cpp \
    pokecraft-server/Map_custom.cpp \
    pokecraft-server/LatencyChecker.cpp \
    pokecraft-server/EventThreader.cpp \
    pokecraft-server/ClientNetworkWrite.cpp \
    pokecraft-server/ClientNetworkRead.cpp \
    pokecraft-server/ClientMapManagement.cpp \
    pokecraft-server/EventDispatcher.cpp \
    pokecraft-server/ClientHeavyLoad.cpp \
    pokecraft-server/ClientBroadCast.cpp \
    pokecraft-server/Client.cpp \
    pokecraft-server/FakeBot.cpp \
    pokecraft-general/MoveClient.cpp \
    pokecraft-general/DebugClass.cpp

HEADERS += \
    ProcessControler.h \
    pokecraft-server/ServerStructures.h \
    pokecraft-server/Map_loader.h \
    pokecraft-server/Map_custom.h \
    pokecraft-server/LatencyChecker.h \
    pokecraft-server/EventThreader.h \
    pokecraft-server/ClientMapManagement.h \
    pokecraft-server/EventDispatcher.h \
    pokecraft-server/ClientNetworkWrite.h \
    pokecraft-server/ClientNetworkRead.h \
    pokecraft-server/ClientHeavyLoad.h \
    pokecraft-server/ClientBroadCast.h \
    pokecraft-server/Client.h \
    pokecraft-server/FakeBot.h \
    pokecraft-general/MoveClient.h \
    pokecraft-general/DebugClass.h \
    pokecraft-general/GeneralStructures.h \
    VariableServer.h


RESOURCES += \
    resources-server.qrc

