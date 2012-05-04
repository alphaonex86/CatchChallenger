#-------------------------------------------------
#
# Project created by QtCreator 2011-08-01T13:49:06
#
#-------------------------------------------------

QT       += core network xml sql
QT       -= gui

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"

TARGET = pokecraft-server-cli
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main-cli.cpp \
    ProcessControler.cpp \
    base/EventDispatcher.cpp \
    base/EventThreader.cpp \
    base/Client.cpp \
    base/ClientHeavyLoad.cpp \
    base/ClientNetworkWrite.cpp \
    base/ClientNetworkRead.cpp \
    base/ClientBroadCast.cpp \
    base/ClientMapManagement/ClientMapManagement.cpp \
    base/Map_loader.cpp \
    base/PlayerUpdater.cpp \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp \
    base/LatencyChecker.cpp \
    base/FakeBot.cpp \
    ../general/base/DebugClass.cpp \
    ../general/base/MoveClient.cpp \
    ../general/base/ChatParsing.cpp

HEADERS += \
    ProcessControler.h \
    VariableServer.h \
    base/EventDispatcher.h \
    base/ServerStructures.h \
    base/EventThreader.h \
    base/Client.h \
    base/ClientHeavyLoad.h \
    base/ClientNetworkWrite.h \
    base/ClientNetworkRead.h \
    base/ClientBroadCast.h \
    base/ClientMapManagement/ClientMapManagement.h \
    base/Map_loader.h \
    base/LatencyChecker.h \
    base/FakeBot.h \
    base/PlayerUpdater.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    ../client/base/ClientStructures.h \
    ../general/base/MoveClient.h \
    ../general/base/DebugClass.h \
    ../general/base/GeneralStructures.h \
    ../general/base/ChatParsing.h


RESOURCES += \
    resources-server.qrc

