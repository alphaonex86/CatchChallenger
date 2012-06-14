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
    base/ClientLocalCalcule.cpp \
    base/Map_loader.cpp \
    base/PlayerUpdater.cpp \
    base/LatencyChecker.cpp \
    base/FakeBot.cpp \
    base/ClientMapManagement/ClientMapManagement.cpp \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp \
    base/ClientMapManagement/MapBasicMove.cpp \
    base/ClientMapManagement/MapVisibilityAlgorithm_None.cpp \
    ../general/base/DebugClass.cpp \
    ../general/base/MoveClient.cpp \
    ../general/base/ChatParsing.cpp \
    ../general/base/QFakeServer.cpp \
    ../general/base/QFakeSocket.cpp

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
    base/ClientLocalCalcule.h \
    base/Map_loader.h \
    base/LatencyChecker.h \
    base/FakeBot.h \
    base/PlayerUpdater.h \
    base/ClientMapManagement/ClientMapManagement.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    base/ClientMapManagement/MapBasicMove.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_None.h \
    ../client/base/ClientStructures.h \
    ../general/base/MoveClient.h \
    ../general/base/DebugClass.h \
    ../general/base/GeneralStructures.h \
    ../general/base/ChatParsing.h \
    ../general/base/QFakeServer.h \
    ../general/base/QFakeSocket.h


RESOURCES += \
    resources-server.qrc

