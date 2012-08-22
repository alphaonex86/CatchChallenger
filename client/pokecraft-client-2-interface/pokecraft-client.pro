#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

include(libtiled/libtiled.pri)

QT       += core gui network opengl xml

TARGET = pokecraft-client
TEMPLATE = app


SOURCES += main.cpp\
	mainwindow.cpp \
    ../../general/base/DebugClass.cpp \
    ../../general/base/ChatParsing.cpp \
    pokecraft-clients/player.cpp \
    pokecraft-clients/MultiMap.cpp \
    pokecraft-clients/craft-clients.cpp \
    pokecraft-clients/objectgroupitem.cpp \
    pokecraft-clients/objectitem.cpp \
    pokecraft-clients/spriteimage.cpp \
    pokecraft-clients/tilelayeritem.cpp \
    pokecraft-clients/gamedata.cpp \
    pokecraft-clients/graphicsviewkeyinput.cpp \
    ../../general/base/QFakeSocket.cpp \
    ../../general/base/QFakeServer.cpp \
    ../../general/base/ProtocolParsing.cpp \
    ../../general/base/MoveOnTheMap.cpp \
    ../../general/base/Map_loader.cpp \
    ../../general/base/Map.cpp \
    ../base/MoveOnTheMap_Client.cpp \
    ../base/Api_protocol.cpp \
    ../base/Api_client_virtual.cpp \
    ../base/Api_client_real.cpp

HEADERS  += mainwindow.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/DebugClass.h \
    ../../general/base/ChatParsing.h \
    ../../general/base/VariableGeneral.h \
    pokecraft-clients/player.h \
    pokecraft-clients/graphicsviewkeyinput.h \
    pokecraft-clients/MultiMap.h \
    pokecraft-clients/craft-clients.h \
    pokecraft-clients/objectgroupitem.h \
    pokecraft-clients/objectitem.h \
    pokecraft-clients/spriteimage.h \
    pokecraft-clients/tilelayeritem.h \
    pokecraft-clients/gamedata.h \
    ../../general/base/QFakeServer.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/MoveOnTheMap.h \
    ../../general/base/Map_loader.h \
    ../../general/base/Map.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/QFakeSocket.h \
    ../base/MoveOnTheMap_Client.h \
    ../base/ClientStructures.h \
    ../base/Api_protocol.h \
    ../base/Api_client_virtual.h \
    ../base/Api_client_real.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    Pokecraft_client

win32:RC_FILE += resources-windows.rc

