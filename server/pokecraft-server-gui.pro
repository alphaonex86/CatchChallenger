#-------------------------------------------------
#
# Project created by QtCreator 2011-08-14T19:50:23
#
#-------------------------------------------------

QT       += core gui network xml sql

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"

TARGET = pokecraft-server-gui
TEMPLATE = app

CONFIG   += console

SOURCES += main.cpp\
	MainWindow.cpp \
    base/EventDispatcher.cpp \
    base/EventThreader.cpp \
    base/Client.cpp \
    base/ClientHeavyLoad.cpp \
    base/ClientNetworkWrite.cpp \
    base/ClientNetworkRead.cpp \
    base/ClientBroadCast.cpp \
    base/ClientLocalCalcule.cpp \
    base/PlayerUpdater.cpp \
    base/Bot/FakeBot.cpp \
    base/ClientMapManagement/ClientMapManagement.cpp \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp \
    base/ClientMapManagement/MapBasicMove.cpp \
    base/ClientMapManagement/MapVisibilityAlgorithm_None.cpp \
    ../general/base/DebugClass.cpp \
    ../general/base/MoveOnTheMap.cpp \
    ../general/base/ChatParsing.cpp \
    ../general/base/ProtocolParsing.cpp \
    ../general/base/QFakeServer.cpp \
    ../general/base/QFakeSocket.cpp \
    ../general/base/Map_loader.cpp \
    ../client/base/Api_protocol.cpp \
    ../client/base/Api_client_real.cpp \
    ../client/base/Api_client_virtual.cpp \
    base/Bot/MoveOnTheMap_Server.cpp \
    base/BroadCastWithoutSender.cpp \
    ../general/base/Map.cpp \
    base/Map_server.cpp


HEADERS += \
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
    base/Bot/FakeBot.h \
    base/PlayerUpdater.h \
    base/ClientMapManagement/ClientMapManagement.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    base/ClientMapManagement/MapBasicMove.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_None.h \
    ../client/base/ClientStructures.h \
    ../general/base/MoveOnTheMap.h \
    ../general/base/DebugClass.h \
    ../general/base/GeneralStructures.h \
    ../general/base/ChatParsing.h \
    ../general/base/GeneralVariable.h \
    ../general/base/ProtocolParsing.h \
    ../general/base/QFakeServer.h \
    ../general/base/QFakeSocket.h \
    ../general/base/Map_loader.h \
    ../client/base/Api_protocol.h \
    ../client/base/Api_client_real.h \
    ../client/base/Api_client_virtual.h \
    base/Bot/MoveOnTheMap_Server.h \
    base/BroadCastWithoutSender.h \
    ../general/base/Map.h \
    base/Map_server.h

HEADERS  += MainWindow.h

FORMS    += MainWindow.ui

RESOURCES += \
    resources.qrc \
    resources-server.qrc

win32:RC_FILE += resources-windows.rc
