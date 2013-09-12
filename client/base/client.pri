LIBS += -lvorbis -lvorbisfile -ltiled
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += script multimedia opengl

win32:CONFIG   += console

SOURCES += $$PWD/Api_client_real.cpp \
    $$PWD/render/TileLayerItem.cpp \
    $$PWD/render/ObjectGroupItem.cpp \
    $$PWD/audio/QOggAudioBuffer.cpp \
    $$PWD/audio/QOggSimplePlayer.cpp \
    $$PWD/render/MapVisualiserPlayer.cpp \
    $$PWD/render/MapVisualiser.cpp \
    $$PWD/render/MapVisualiser-map.cpp \
    $$PWD/render/MapObjectItem.cpp \
    $$PWD/render/MapItem.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/interface/BaseWindow.cpp \
    $$PWD/interface/BaseWindowCity.cpp \
    $$PWD/interface/BaseWindowClan.cpp \
    $$PWD/interface/BaseWindowFactory.cpp \
    $$PWD/interface/BaseWindowLoad.cpp \
    $$PWD/interface/BaseWindowMarket.cpp \
    $$PWD/interface/BaseWindowOptions.cpp \
    $$PWD/interface/BaseWindowShop.cpp \
    $$PWD/interface/BaseWindowWarehouse.cpp \
    $$PWD/interface/MapControllerMP.cpp \
    $$PWD/interface/ListEntryEnvolued.cpp \
    $$PWD/../crafting/interface/MapControllerCrafting.cpp \
    $$PWD/interface/MapController.cpp \
    $$PWD/interface/DatapackClientLoader.cpp \
    $$PWD/../crafting/interface/DatapackClientLoaderCrafting.cpp \
    $$PWD/../fight/interface/DatapackClientLoaderFight.cpp \
    $$PWD/../crafting/interface/BaseWindowCrafting.cpp \
    $$PWD/Options.cpp \
    $$PWD/../fight/interface/BaseWindowFight.cpp \
    $$PWD/interface/Chat.cpp \
    $$PWD/../fight/interface/ClientFightEngine.cpp \
    $$PWD/../fight/interface/MapVisualiserPlayerWithFight.cpp \
    $$PWD/interface/WithAnotherPlayer.cpp \
    $$PWD/render/MapVisualiserThread.cpp \
    $$PWD/interface/QuestJS.cpp \
    $$PWD/interface/GetPrice.cpp

HEADERS  += $$PWD/MoveOnTheMap_Client.h \
    $$PWD/audio/QOggAudioBuffer.h \
    $$PWD/audio/QOggSimplePlayer.h \
    $$PWD/ClientStructures.h \
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
    $$PWD/interface/ListEntryEnvolued.h \
    $$PWD/interface/MapController.h \
    $$PWD/interface/DatapackClientLoader.h \
    $$PWD/ClientVariable.h \
    $$PWD/Options.h \
    $$PWD/interface/Chat.h \
    $$PWD/../fight/interface/ClientFightEngine.h \
    $$PWD/../fight/interface/MapVisualiserPlayerWithFight.h \
    $$PWD/interface/WithAnotherPlayer.h \
    $$PWD/render/MapVisualiserThread.h \
    $$PWD/interface/QuestJS.h \
    $$PWD/interface/GetPrice.h

FORMS    += $$PWD/interface/BaseWindow.ui \
    $$PWD/interface/Chat.ui \
    $$PWD/interface/WithAnotherPlayer.ui \
    $$PWD/interface/GetPrice.ui

win32:RC_FILE += $$PWD/resources/resources-windows.rc

RESOURCES += $$PWD/resources/client-resources.qrc \
    $$PWD/../crafting/resources/client-resources-plant.qrc \
    $$PWD/../fight/resources/client-resources-fight.qrc
