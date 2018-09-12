include(../tiled/tiled.pri)

QT       += gui network core widgets
QT       += qml quick

DEFINES += CATCHCHALLENGER_CLIENT

# see the file ClientVariableAudio.h
DEFINES += CATCHCHALLENGER_NOAUDIO
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
linux:LIBS += -lvlc
macx:LIBS += -lvlc
win32:LIBS += -lvlc
}

SOURCES += \
    $$PWD/render/TileLayerItem.cpp \
    $$PWD/render/ObjectGroupItem.cpp \
    $$PWD/render/MapVisualiserPlayer.cpp \
    $$PWD/render/MapVisualiser.cpp \
    $$PWD/render/MapVisualiser-map.cpp \
    $$PWD/render/MapObjectItem.cpp \
    $$PWD/render/MapItem.cpp \
    $$PWD/render/MapVisualiserThread.cpp \
    $$PWD/render/MapVisualiserOrder.cpp \
    $$PWD/render/PreparedLayer.cpp \
    $$PWD/render/MapMark.cpp \
    $$PWD/render/MapDoor.cpp \
    $$PWD/render/MapControllerMP.cpp \
    $$PWD/render/MapControllerMPAPI.cpp \
    $$PWD/render/MapControllerMPMove.cpp \
    $$PWD/render/MapController.cpp \
    $$PWD/render/TriggerAnimation.cpp \
    $$PWD/render/TemporaryTile.cpp \
    $$PWD/render/PathFinding.cpp \
    $$PWD/interface/BaseWindow.cpp \
    $$PWD/interface/BaseWindowMap.cpp \
    $$PWD/interface/BaseWindowBot.cpp \
    $$PWD/interface/BaseWindowSelection.cpp \
    $$PWD/interface/BaseWindowCharacter.cpp \
    $$PWD/interface/BaseWindowCity.cpp \
    $$PWD/interface/BaseWindowClan.cpp \
    $$PWD/interface/BaseWindowFactory.cpp \
    $$PWD/interface/BaseWindowLoad.cpp \
    $$PWD/interface/BaseWindowMarket.cpp \
    $$PWD/interface/BaseWindowOptions.cpp \
    $$PWD/interface/BaseWindowShop.cpp \
    $$PWD/interface/BaseWindowWarehouse.cpp \
    $$PWD/interface/ListEntryEnvolued.cpp \
    $$PWD/interface/Chat.cpp \
    $$PWD/interface/WithAnotherPlayer.cpp \
    $$PWD/interface/GetPrice.cpp \
    $$PWD/interface/NewProfile.cpp \
    $$PWD/interface/NewGame.cpp \
    $$PWD/../crafting/interface/MapControllerCrafting.cpp \
    $$PWD/../crafting/interface/QmlInterface/CraftingAnimation.cpp \
    $$PWD/../crafting/interface/DatapackClientLoaderCrafting.cpp \
    $$PWD/../crafting/interface/BaseWindowCrafting.cpp \
    $$PWD/../fight/interface/QmlInterface/QmlMonsterGeneralInformations.cpp \
    $$PWD/../fight/interface/QmlInterface/EvolutionControl.cpp \
    $$PWD/../fight/interface/DatapackClientLoaderFight.cpp \
    $$PWD/../fight/interface/BaseWindowFight.cpp \
    $$PWD/../fight/interface/BaseWindowFightNextAction.cpp \
    $$PWD/../fight/interface/ClientFightEngine.cpp \
    $$PWD/../fight/interface/MapVisualiserPlayerWithFight.cpp \
    $$PWD/QmlInterface/AnimationControl.cpp \
    $$PWD/DatapackClientLoader.cpp \
    $$PWD/DatapackChecksum.cpp \
    $$PWD/Api_client_virtual.cpp \
    $$PWD/Api_protocol.cpp \
    $$PWD/Api_protocol_loadchar.cpp \
    $$PWD/Api_protocol_message.cpp \
    $$PWD/Api_protocol_query.cpp \
    $$PWD/Api_protocol_reply.cpp \
    $$PWD/Options.cpp \
    $$PWD/LanguagesSelect.cpp \
    $$PWD/InternetUpdater.cpp \
    $$PWD/ExtraSocket.cpp \
    $$PWD/LocalListener.cpp \
    $$PWD/Audio.cpp \
    $$PWD/CachedString.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/FacilityLibClient.cpp \
    $$PWD/Api_protocol_Qt.cpp

HEADERS  += $$PWD/ClientStructures.h \
    $$PWD/render/TileLayerItem.h \
    $$PWD/render/ObjectGroupItem.h \
    $$PWD/render/MapVisualiserPlayer.h \
    $$PWD/render/MapVisualiser.h \
    $$PWD/render/MapObjectItem.h \
    $$PWD/render/MapItem.h \
    $$PWD/render/PreparedLayer.h \
    $$PWD/render/MapMark.h \
    $$PWD/render/MapVisualiserThread.h \
    $$PWD/render/MapVisualiserOrder.h \
    $$PWD/render/MapDoor.h \
    $$PWD/render/MapControllerMP.h \
    $$PWD/render/MapController.h \
    $$PWD/render/TriggerAnimation.h \
    $$PWD/render/TemporaryTile.h \
    $$PWD/render/PathFinding.h \
    $$PWD/interface/BaseWindow.h \
    $$PWD/interface/ListEntryEnvolued.h \
    $$PWD/interface/Chat.h \
    $$PWD/interface/WithAnotherPlayer.h \
    $$PWD/interface/GetPrice.h \
    $$PWD/interface/NewProfile.h \
    $$PWD/interface/NewGame.h \
    $$PWD/../crafting/interface/QmlInterface/CraftingAnimation.h \
    $$PWD/../fight/interface/QmlInterface/QmlMonsterGeneralInformations.h \
    $$PWD/../fight/interface/QmlInterface/EvolutionControl.h \
    $$PWD/../fight/interface/ClientFightEngine.h \
    $$PWD/../fight/interface/MapVisualiserPlayerWithFight.h \
    $$PWD/QmlInterface/AnimationControl.h \
    $$PWD/DatapackClientLoader.h \
    $$PWD/DatapackChecksum.h \
    $$PWD/Api_protocol_Qt.h \
    $$PWD/LanguagesSelect.h \
    $$PWD/CachedString.h \
    $$PWD/InternetUpdater.h \
    $$PWD/ExtraSocket.h \
    $$PWD/LocalListener.h \
    $$PWD/DisplayStructures.h \
    $$PWD/Audio.h \
    $$PWD/Map_client.h \
    $$PWD/ClientVariable.h \
    $$PWD/Options.h \
    $$PWD/Api_client_virtual.h \
    $$PWD/Api_protocol.h \
    $$PWD/FacilityLibClient.h \
    $$PWD/ClientVariableAudio.h

FORMS    += $$PWD/interface/BaseWindow.ui \
    $$PWD/interface/Chat.ui \
    $$PWD/interface/WithAnotherPlayer.ui \
    $$PWD/interface/GetPrice.ui \
    $$PWD/interface/NewProfile.ui \
    $$PWD/interface/NewGame.ui \
    $$PWD/LanguagesSelect.ui

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/resources/resources-windows.rc
ICON = $$PWD/resources/client.icns
macx:INCLUDEPATH += /Users/user/Desktop/VLC.app/Contents/MacOS/include/
macx:LIBS += -L/Users/user/Desktop/VLC.app/Contents/MacOS/lib/

RESOURCES += $$PWD/resources/client-resources.qrc \
    $$PWD/../crafting/resources/client-resources-plant.qrc \
    $$PWD/../fight/resources/client-resources-fight.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/translation.ts \
    $$PWD/languages/fr/translation.ts

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
