#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
LIBS *= -ltiled

QT       += core gui network opengl xml sql

TARGET = pokecraft-single-player
TEMPLATE = app


SOURCES += main.cpp\
	mainwindow.cpp \
    ../../general/base/DebugClass.cpp \
    ../../general/base/ChatParsing.cpp \
    ../../general/base/QFakeSocket.cpp \
    ../../general/base/QFakeServer.cpp \
    ../../general/base/ProtocolParsing.cpp \
    ../../general/base/MoveOnTheMap.cpp \
    ../../general/base/Map_loader.cpp \
    ../../general/base/Map.cpp \
    ../base/Api_protocol.cpp \
    ../base/Api_client_virtual.cpp \
    ../base/Api_client_real.cpp \
    ../../general/base/FacilityLib.cpp \
    ../base/render/TileLayerItem.cpp \
    ../base/render/ObjectGroupItem.cpp \
    ../base/render/MapVisualiserPlayer.cpp \
    ../base/render/MapVisualiser.cpp \
    ../base/render/MapVisualiser-map.cpp \
    ../base/render/MapObjectItem.cpp \
    ../base/render/MapItem.cpp \
    ../base/Map_client.cpp \
    ../../server/base/BroadCastWithoutSender.cpp \
    ../../server/base/PlayerUpdater.cpp \
    ../../server/base/Map_server.cpp \
    ../../server/base/EventThreader.cpp \
    ../../server/base/ClientNetworkWrite.cpp \
    ../../server/base/ClientNetworkRead.cpp \
    ../../server/base/ClientLocalCalcule.cpp \
    ../../server/base/ClientHeavyLoad.cpp \
    ../../server/base/ClientBroadCast.cpp \
    ../../server/base/Client.cpp \
    ../../server/base/ClientMapManagement/MapVisibilityAlgorithm_None.cpp \
    ../../server/base/ClientMapManagement/MapBasicMove.cpp \
    ../../server/base/ClientMapManagement/ClientMapManagement.cpp \
    ../../server/base/Bot/FakeBot.cpp \
    ../../server/base/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp \
    ../../server/base/GlobalData.cpp \
    ../../server/base/SqlFunction.cpp \
    InternalServer.cpp \
    NewGame.cpp \
    ../base/interface/MapController.cpp \
    ../base/interface/BaseWindow.cpp \
    SaveGameLabel.cpp

HEADERS  += mainwindow.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/DebugClass.h \
    ../../general/base/ChatParsing.h \
    ../../general/base/QFakeServer.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/MoveOnTheMap.h \
    ../../general/base/Map_loader.h \
    ../../general/base/Map.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/QFakeSocket.h \
    ../base/ClientStructures.h \
    ../base/Api_protocol.h \
    ../base/Api_client_virtual.h \
    ../base/Api_client_real.h \
    ../../general/base/FacilityLib.h \
    ../base/render/TileLayerItem.h \
    ../base/render/ObjectGroupItem.h \
    ../base/render/MapVisualiserPlayer.h \
    ../base/render/MapVisualiser.h \
    ../base/render/MapObjectItem.h \
    ../base/render/MapItem.h \
    ../base/Map_client.h \
    ../../server/base/ServerStructures.h \
    ../../server/base/PlayerUpdater.h \
    ../../server/base/Map_server.h \
    ../../server/base/EventThreader.h \
    ../../server/base/ClientNetworkWrite.h \
    ../../server/base/ClientNetworkRead.h \
    ../../server/base/ClientLocalCalcule.h \
    ../../server/base/ClientHeavyLoad.h \
    ../../server/base/ClientBroadCast.h \
    ../../server/base/Client.h \
    ../../server/base/BroadCastWithoutSender.h \
    ../../server/base/ClientMapManagement/MapVisibilityAlgorithm_None.h \
    ../../server/base/ClientMapManagement/MapBasicMove.h \
    ../../server/base/ClientMapManagement/ClientMapManagement.h \
    ../../server/base/Bot/FakeBot.h \
    ../../server/base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    ../../server/base/GlobalData.h \
    ../../server/base/SqlFunction.h \
    InternalServer.h \
    NewGame.h \
    ../base/interface/BaseWindow.h \
    ../base/interface/MapController.h \
    SaveGameLabel.h

FORMS    += mainwindow.ui \
    NewGame.ui \
    ../base/interface/BaseWindow.ui

RESOURCES += \
    resources/resources-single-player.qrc \
    ../base/resources/resources.qrc \
    ../base/resources/resources-multi.qrc

OTHER_FILES += \
    Pokecraft_client

win32:RC_FILE += ../base/resources/resources-windows.rc

