#-------------------------------------------------
#
# Project created by QtCreator 2011-08-14T19:50:23
#
#-------------------------------------------------

QT       += core gui network xml sql widgets

QMAKE_CFLAGS += -pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops  -O2 -ftree-vectorize
QMAKE_CXXFLAGS += -pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -O2 -ftree-vectorize

TARGET = pokecraft-server-gui
TEMPLATE = app

win32:CONFIG   += console

SOURCES += MainWindow.cpp \
    main.cpp
HEADERS  += MainWindow.h

SOURCES += base/EventDispatcher.cpp \
    base/EventThreader.cpp \
    base/Client.cpp \
    base/ClientHeavyLoad.cpp \
    base/ClientNetworkWrite.cpp \
    base/ClientNetworkRead.cpp \
    base/ClientBroadCast.cpp \
    base/ClientLocalCalcule.cpp \
    base/PlayerUpdater.cpp \
    base/BroadCastWithoutSender.cpp \
    base/Map_server.cpp \
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
    ../general/base/Map.cpp \
    ../client/base/Api_protocol.cpp \
    ../client/base/Api_client_real.cpp \
    ../client/base/Api_client_virtual.cpp
HEADERS += VariableServer.h \
    base/EventDispatcher.h \
    base/ServerStructures.h \
    base/EventThreader.h \
    base/Client.h \
    base/ClientHeavyLoad.h \
    base/ClientNetworkWrite.h \
    base/ClientNetworkRead.h \
    base/ClientBroadCast.h \
    base/ClientLocalCalcule.h \
    base/BroadCastWithoutSender.h \
    base/PlayerUpdater.h \
    base/Map_server.h \
    base/Bot/FakeBot.h \
    base/ClientMapManagement/ClientMapManagement.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    base/ClientMapManagement/MapBasicMove.h \
    base/ClientMapManagement/MapVisibilityAlgorithm_None.h \
    ../general/base/MoveOnTheMap.h \
    ../general/base/DebugClass.h \
    ../general/base/GeneralStructures.h \
    ../general/base/ChatParsing.h \
    ../general/base/GeneralVariable.h \
    ../general/base/ProtocolParsing.h \
    ../general/base/QFakeServer.h \
    ../general/base/QFakeSocket.h \
    ../general/base/Map_loader.h \
    ../general/base/Map.h \
    ../client/base/ClientStructures.h \
    ../client/base/Api_protocol.h \
    ../client/base/Api_client_real.h \
    ../client/base/Api_client_virtual.h

FORMS    += MainWindow.ui

RESOURCES += \
    resources.qrc \
    resources-server.qrc

win32:RC_FILE += resources-windows.rc
