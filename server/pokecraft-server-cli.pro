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
    pokecraft-server/EventDispatcher.cpp \
    pokecraft-general/DebugClass.cpp \
    pokecraft-server/EventThreader.cpp \
    pokecraft-server/Client.cpp \
    pokecraft-server/ClientHeavyLoad.cpp \
    pokecraft-server/ClientNetworkWrite.cpp \
    pokecraft-server/ClientNetworkRead.cpp \
    pokecraft-server/ClientBroadCast.cpp \
    pokecraft-server/ClientMapManagement/ClientMapManagement.cpp \
    pokecraft-server/Map_loader.cpp \
    pokecraft-client/pokecraft-client-chat.cpp \
    pokecraft-server/LatencyChecker.cpp \
    pokecraft-server/FakeBot.cpp \
    pokecraft-general/MoveClient.cpp \
    pokecraft-server/PlayerUpdater.cpp \
    pokecraft-server/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp

HEADERS += \
    ProcessControler.h \
    VariableServer.h \
    pokecraft-server/EventDispatcher.h \
    pokecraft-general/DebugClass.h \
    pokecraft-server/ServerStructures.h \
    pokecraft-server/EventThreader.h \
    pokecraft-server/Client.h \
    pokecraft-server/ClientHeavyLoad.h \
    pokecraft-server/ClientNetworkWrite.h \
    pokecraft-server/ClientNetworkRead.h \
    pokecraft-server/ClientBroadCast.h \
    pokecraft-server/ClientMapManagement/ClientMapManagement.h \
    pokecraft-server/Map_loader.h \
    pokecraft-general/GeneralStructures.h \
    pokecraft-client/pokecraft-client-chat.h \
    pokecraft-server/LatencyChecker.h \
    pokecraft-server/FakeBot.h \
    pokecraft-general/MoveClient.h \
    pokecraft-client/ClientStructures.h \
    pokecraft-server/PlayerUpdater.h \
    pokecraft-server/ClientMapManagement/MapVisibilityAlgorithm_Simple.h


RESOURCES += \
    resources-server.qrc

