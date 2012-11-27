INCLUDEPATH += $$PWD/../../general/libtiled/
DEPENDPATH += $$PWD/../../general/libtiled/
LIBS *= -ltiled

QT       += core gui network opengl xml

TEMPLATE = app

SOURCES += $$PWD/../../general/base/DebugClass.cpp \
    $$PWD/../../general/base/ChatParsing.cpp \
    $$PWD/../../general/base/QFakeSocket.cpp \
    $$PWD/../../general/base/QFakeServer.cpp \
    $$PWD/../../general/base/ProtocolParsing.cpp \
    $$PWD/../../general/base/MoveOnTheMap.cpp \
    $$PWD/../../general/base/Map_loader.cpp \
    $$PWD/../../general/base/Map.cpp \
    $$PWD/Api_protocol.cpp \
    $$PWD/Api_client_virtual.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/../../general/base/FacilityLib.cpp \
    $$PWD/render/TileLayerItem.cpp \
    $$PWD/render/ObjectGroupItem.cpp \
    $$PWD/render/MapVisualiserPlayer.cpp \
    $$PWD/render/MapVisualiser.cpp \
    $$PWD/render/MapVisualiser-map.cpp \
    $$PWD/render/MapObjectItem.cpp \
    $$PWD/render/MapItem.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/interface/BaseWindow.cpp \
    $$PWD/interface/MapControllerMP.cpp \
    $$PWD/../crafting/interface/MapControllerCrafting.cpp \
    $$PWD/interface/MapController.cpp \
    $$PWD/../../general/base/ConnectedSocket.cpp \
    $$PWD/interface/DatapackClientLoader.cpp \
    $$PWD/../crafting/interface/DatapackClientLoaderCrafting.cpp

HEADERS  += $$PWD/../../general/base/GeneralStructures.h \
    $$PWD/../../general/base/DebugClass.h \
    $$PWD/../../general/base/ChatParsing.h \
    $$PWD/../../general/base/VariableGeneral.h \
    $$PWD/../../general/base/QFakeServer.h \
    $$PWD/../../general/base/ProtocolParsing.h \
    $$PWD/../../general/base/MoveOnTheMap.h \
    $$PWD/../../general/base/Map_loader.h \
    $$PWD/../../general/base/Map.h \
    $$PWD/../../general/base/GeneralVariable.h \
    $$PWD/../../general/base/QFakeSocket.h \
    $$PWD/MoveOnTheMap_Client.h \
    $$PWD/ClientStructures.h \
    $$PWD/Api_protocol.h \
    $$PWD/Api_client_virtual.h \
    $$PWD/Api_client_real.h \
    $$PWD/../../general/base/FacilityLib.h \
    $$PWD/render/TileLayerItem.h \
    $$PWD/render/ObjectGroupItem.h \
    $$PWD/render/MapVisualiserPlayer.h \
    $$PWD/render/MapVisualiser.h \
    $$PWD/render/MapObjectItem.h \
    $$PWD/render/MapItem.h \
    $$PWD/Map_client.h \
    $$PWD/interface/BaseWindow.h \
    $$PWD/interface/MapControllerMP.h \
    $$PWD/interface/MapController.h \
    $$PWD/../../general/base/ConnectedSocket.h \
    $$PWD/interface/DatapackClientLoader.h \
    $$PWD/ClientVariable.h

FORMS    += $$PWD/interface/BaseWindow.ui

win32:RC_FILE += $$PWD/resources/resources-windows.rc

RESOURCES += $$PWD/resources/client-resources.qrc \
    $$PWD/../crafting/resources/client-resources-plant.qrc
