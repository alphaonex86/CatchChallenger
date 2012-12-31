INCLUDEPATH += $$PWD/../../general/libtiled/
DEPENDPATH += $$PWD/../../general/libtiled/
LIBS *= -ltiled

SOURCES += $$PWD/Api_protocol.cpp \
    $$PWD/Api_client_virtual.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/render/TileLayerItem.cpp \
    $$PWD/render/ObjectGroupItem.cpp \
    $$PWD/render/MapVisualiserPlayer.cpp \
    $$PWD/render/MapVisualiser.cpp \
    $$PWD/render/MapVisualiser-map.cpp \
    $$PWD/render/MapObjectItem.cpp \
    $$PWD/render/MapItem.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/interface/BaseWindow.cpp \
    $$PWD/interface/BaseWindowOptions.cpp \
    $$PWD/interface/MapControllerMP.cpp \
    $$PWD/../crafting/interface/MapControllerCrafting.cpp \
    $$PWD/interface/MapController.cpp \
    $$PWD/interface/DatapackClientLoader.cpp \
    $$PWD/../crafting/interface/DatapackClientLoaderCrafting.cpp \
    ../crafting/interface/BaseWindowCrafting.cpp \
    ../base/Options.cpp

HEADERS  += $$PWD/MoveOnTheMap_Client.h \
    $$PWD/ClientStructures.h \
    $$PWD/Api_protocol.h \
    $$PWD/Api_client_virtual.h \
    $$PWD/Api_client_real.h \
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
    $$PWD/interface/DatapackClientLoader.h \
    $$PWD/ClientVariable.h \
    ../base/Options.h

FORMS    += $$PWD/interface/BaseWindow.ui

win32:RC_FILE += $$PWD/resources/resources-windows.rc

RESOURCES += $$PWD/resources/client-resources.qrc \
    $$PWD/../crafting/resources/client-resources-plant.qrc \
    ../fight/resources/client-resources-fight.qrc
