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
    pokecraft-server/base/EventDispatcher.cpp \
    pokecraft-server/base/EventThreader.cpp \
    pokecraft-server/base/Client.cpp \
    pokecraft-server/base/ClientHeavyLoad.cpp \
    pokecraft-server/base/ClientNetworkWrite.cpp \
    pokecraft-server/base/ClientNetworkRead.cpp \
    pokecraft-server/base/ClientBroadCast.cpp \
    pokecraft-server/base/ClientMapManagement/ClientMapManagement.cpp \
    pokecraft-server/base/Map_loader.cpp \
    pokecraft-server/base/PlayerUpdater.cpp \
    pokecraft-server/base/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp \
    pokecraft-server/base/LatencyChecker.cpp \
    pokecraft-server/base/FakeBot.cpp \
    pokecraft-general/base/DebugClass.cpp \
    pokecraft-general/base/MoveClient.cpp \
    pokecraft-client/base/pokecraft-client-chat.cpp

HEADERS += \
    ProcessControler.h \
    VariableServer.h \
    pokecraft-server/base/EventDispatcher.h \
    pokecraft-server/base/ServerStructures.h \
    pokecraft-server/base/EventThreader.h \
    pokecraft-server/base/Client.h \
    pokecraft-server/base/ClientHeavyLoad.h \
    pokecraft-server/base/ClientNetworkWrite.h \
    pokecraft-server/base/ClientNetworkRead.h \
    pokecraft-server/base/ClientBroadCast.h \
    pokecraft-server/base/ClientMapManagement/ClientMapManagement.h \
    pokecraft-server/base/Map_loader.h \
    pokecraft-server/base/LatencyChecker.h \
    pokecraft-server/base/FakeBot.h \
    pokecraft-server/base/PlayerUpdater.h \
    pokecraft-server/base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    pokecraft-client/base/ClientStructures.h \
    pokecraft-client/base/pokecraft-client-chat.h \
    pokecraft-general/base/MoveClient.h \
    pokecraft-general/base/DebugClass.h \
    pokecraft-general/base/GeneralStructures.h


RESOURCES += \
    resources-server.qrc

