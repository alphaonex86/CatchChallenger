INCLUDEPATH += $$PWD/../../general/libtiled/
DEPENDPATH += $$PWD/../../general/libtiled/
LIBS *= -ltiled

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += script

win32:CONFIG   += console

SOURCES += $$PWD/Api_client_real.cpp \
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
    $$PWD/../fight/interface/DatapackClientLoaderFight.cpp \
    $$PWD/../crafting/interface/BaseWindowCrafting.cpp \
    $$PWD/Options.cpp \
    $$PWD/interface/BaseWindowLoad.cpp \
    $$PWD/../fight/interface/BaseWindowFight.cpp \
    $$PWD/interface/Chat.cpp \
    $$PWD/../fight/interface/FightEngine.cpp \
    $$PWD/../fight/interface/MapVisualiserPlayerWithFight.cpp \
    $$PWD/interface/WithAnotherPlayer.cpp \
    ../base/render/MapVisualiserThread.cpp

HEADERS  += $$PWD/MoveOnTheMap_Client.h \
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
    $$PWD/interface/MapController.h \
    $$PWD/interface/DatapackClientLoader.h \
    $$PWD/ClientVariable.h \
    $$PWD/Options.h \
    $$PWD/interface/Chat.h \
    $$PWD/../fight/interface/FightEngine.h \
    $$PWD/../fight/interface/MapVisualiserPlayerWithFight.h \
    $$PWD/interface/WithAnotherPlayer.h \
    ../base/render/MapVisualiserThread.h

FORMS    += $$PWD/interface/BaseWindow.ui \
    $$PWD/interface/Chat.ui \
    $$PWD/interface/WithAnotherPlayer.ui

win32:RC_FILE += $$PWD/resources/resources-windows.rc

RESOURCES += $$PWD/resources/client-resources.qrc \
    $$PWD/../crafting/resources/client-resources-plant.qrc \
    $$PWD/../fight/resources/client-resources-fight.qrc
