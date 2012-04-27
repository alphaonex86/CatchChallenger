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

HEADERS  += MainWindow.h

FORMS    += MainWindow.ui

RESOURCES += \
    resources.qrc \
    resources-server.qrc

win32:RC_FILE += resources-windows.rc
