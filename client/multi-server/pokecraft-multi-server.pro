#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
LIBS *= -ltiled

QMAKE_CFLAGS += -O0
QMAKE_CXXFLAGS += -O0

QT       += core gui network opengl xml

TARGET = pokecraft-multi-server
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
    ../base/interface/BaseWindow.cpp \
    ../base/interface/MapController.cpp \
    ../../general/base/ConnectedSocket.cpp \
    ../base/interface/DatapackClientLoader.cpp

HEADERS  += mainwindow.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/DebugClass.h \
    ../../general/base/ChatParsing.h \
    ../../general/base/VariableGeneral.h \
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
    ../base/Api_client_real.h \
    ../../general/base/FacilityLib.h \
    ../base/render/TileLayerItem.h \
    ../base/render/ObjectGroupItem.h \
    ../base/render/MapVisualiserPlayer.h \
    ../base/render/MapVisualiser.h \
    ../base/render/MapObjectItem.h \
    ../base/render/MapItem.h \
    ../base/Map_client.h \
    ../base/interface/BaseWindow.h \
    ../base/interface/MapController.h \
    ../../general/base/ConnectedSocket.h \
    ../base/interface/DatapackClientLoader.h \
    ../base/ClientVariable.h

FORMS    += mainwindow.ui \
    ../base/interface/BaseWindow.ui

OTHER_FILES += \
    Pokecraft_client

win32:RC_FILE += ../base/resources/resources-windows.rc

RESOURCES += \
    ../base/resources/resources.qrc \
    ../base/resources/resources-multi.qrc

