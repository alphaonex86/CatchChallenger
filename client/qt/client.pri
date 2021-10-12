include(../libcatchchallenger/lib.pri)
include(../qt/tiled/tiled.pri)

QT       += gui network core widgets
QT       += qml quick opengl

DEFINES += CATCHCHALLENGER_CLIENT

wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
#android: DEFINES += CATCHCHALLENGER_NOAUDIO
# see the file ClientVariableAudio.h
#DEFINES += CATCHCHALLENGER_NOAUDIO
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
linux:LIBS += -lopus
macx:LIBS += -lopus
win32:LIBS += -lopus
wasm:LIBS += -Lopus
SOURCES += \
    $$PWD/../libogg/bitwise.c \
    $$PWD/../libogg/framing.c \
    $$PWD/../opusfile/info.c \
    $$PWD/../opusfile/internal.c \
    $$PWD/../opusfile/opusfile.c \
    $$PWD/../opusfile/stream.c \
    $$PWD/Audio.cpp \
    $$PWD/QInfiniteBuffer.cpp

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h \
    $$PWD/Audio.h \
    $$PWD/QInfiniteBuffer.h
}
wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
}
else
{
#    DEFINES += NOWEBSOCKET
}
android: {
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm64/usr/include/
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm/usr/include/
    LIBS += -L/opt/qt/5.12.4/android_arm64_v8a/lib/
    LIBS += -L/opt/qt/5.12.4/android_armv7/lib/
    QT += androidextras
    QMAKE_CFLAGS += -g
    QMAKE_CXXFLAGS += -g
}
!contains(DEFINES, NOWEBSOCKET) {
    QT += websockets
}

SOURCES += \
    $$PWD/Settings.cpp \
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
    $$PWD/interface/ChatParsing.cpp \
    $$PWD/interface/WithAnotherPlayer.cpp \
    $$PWD/interface/GetPrice.cpp \
    $$PWD/interface/NewProfile.cpp \
    $$PWD/interface/NewGame.cpp \
    $$PWD/crafting/interface/MapControllerCrafting.cpp \
    $$PWD/crafting/interface/QmlInterface/CraftingAnimation.cpp \
    $$PWD/crafting/interface/DatapackClientLoaderCrafting.cpp \
    $$PWD/crafting/interface/BaseWindowCrafting.cpp \
    $$PWD/fight/interface/QmlInterface/QmlMonsterGeneralInformations.cpp \
    $$PWD/fight/interface/QmlInterface/EvolutionControl.cpp \
    $$PWD/fight/interface/DatapackClientLoaderFight.cpp \
    $$PWD/fight/interface/BaseWindowFight.cpp \
    $$PWD/fight/interface/BaseWindowFightNextAction.cpp \
    $$PWD/fight/interface/ClientFightEngine.cpp \
    $$PWD/fight/interface/MapVisualiserPlayerWithFight.cpp \
    $$PWD/QmlInterface/AnimationControl.cpp \
    $$PWD/QtDatapackClientLoader.cpp \
    $$PWD/QtDatapackChecksum.cpp \
    $$PWD/Api_client_virtual.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/Api_client_real_base.cpp \
    $$PWD/Api_client_real_main.cpp \
    $$PWD/Api_client_real_sub.cpp \
    $$PWD/Options.cpp \
    $$PWD/LanguagesSelect.cpp \
    $$PWD/InternetUpdater.cpp \
    $$PWD/ExtraSocket.cpp \
    $$PWD/LocalListener.cpp \
    $$PWD/CachedString.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/FacilityLibClient.cpp \
    $$PWD/Api_protocol_Qt.cpp \
    $$PWD/Ultimate.cpp \
    $$PWD/LoadingScreen.cpp \
    $$PWD/CCBackground.cpp \
    $$PWD/CustomButton.cpp \
    $$PWD/CCWidget.cpp \
    $$PWD/CCprogressbar.cpp \
    $$PWD/ScreenTransition.cpp \
    $$PWD/GameLoader.cpp \
    $$PWD/GameLoaderThread.cpp \
    $$PWD/MainScreen.cpp \
    $$PWD/CCTitle.cpp \
    $$PWD/OptionsDialog.cpp \
    $$PWD/Solo.cpp \
    $$PWD/Multi.cpp \
    $$PWD/Login.cpp \
    $$PWD/AddServer.cpp \
    $$PWD/AskKey.cpp \
    $$PWD/ConnexionManager.cpp \
    $$PWD/ConnectedSocket.cpp \
    $$PWD/FeedNews.cpp \
    $$PWD/BlacklistPassword.cpp \
    $$PWD/../tarcompressed/ZstdDecode.cpp \
    $$PWD/../tarcompressed/TarDecode.cpp \
    $$PWD/QZstdDecodeThread.cpp

HEADERS  += \
    $$PWD/Settings.hpp \
    $$PWD/render/TileLayerItem.hpp \
    $$PWD/render/ObjectGroupItem.hpp \
    $$PWD/render/MapVisualiserPlayer.hpp \
    $$PWD/render/MapVisualiser.hpp \
    $$PWD/render/MapObjectItem.hpp \
    $$PWD/render/MapItem.hpp \
    $$PWD/render/PreparedLayer.hpp \
    $$PWD/render/MapMark.hpp \
    $$PWD/render/MapVisualiserThread.hpp \
    $$PWD/render/MapVisualiserOrder.hpp \
    $$PWD/render/MapDoor.hpp \
    $$PWD/render/MapControllerMP.hpp \
    $$PWD/render/MapController.hpp \
    $$PWD/render/TriggerAnimation.hpp \
    $$PWD/render/TemporaryTile.hpp \
    $$PWD/render/PathFinding.hpp \
    $$PWD/interface/BaseWindow.hpp \
    $$PWD/interface/ListEntryEnvolued.hpp \
    $$PWD/interface/Chat.hpp \
    $$PWD/interface/WithAnotherPlayer.hpp \
    $$PWD/interface/GetPrice.hpp \
    $$PWD/interface/NewProfile.hpp \
    $$PWD/interface/NewGame.hpp \
    $$PWD/crafting/interface/QmlInterface/CraftingAnimation.hpp \
    $$PWD/fight/interface/QmlInterface/QmlMonsterGeneralInformations.hpp \
    $$PWD/fight/interface/QmlInterface/EvolutionControl.hpp \
    $$PWD/fight/interface/ClientFightEngine.hpp \
    $$PWD/fight/interface/MapVisualiserPlayerWithFight.hpp \
    $$PWD/QmlInterface/AnimationControl.hpp \
    $$PWD/QtDatapackClientLoader.hpp \
    $$PWD/QtDatapackChecksum.hpp \
    $$PWD/Api_protocol_Qt.hpp \
    $$PWD/LanguagesSelect.hpp \
    $$PWD/CachedString.hpp \
    $$PWD/InternetUpdater.hpp \
    $$PWD/ExtraSocket.hpp \
    $$PWD/LocalListener.hpp \
    $$PWD/Map_client.hpp \
    $$PWD/ClientVariable.hpp \
    $$PWD/Options.hpp \
    $$PWD/Api_client_virtual.hpp \
    $$PWD/Api_client_real.hpp \
    $$PWD/FacilityLibClient.hpp \
    $$PWD/ClientVariableAudio.hpp \
    $$PWD/Ultimate.hpp \
    $$PWD/LoadingScreen.hpp \
    $$PWD/CCBackground.hpp \
    $$PWD/CustomButton.hpp \
    $$PWD/DisplayStructures.hpp \
    $$PWD/ClientVariableAudio.hpp \
    $$PWD/CCWidget.hpp \
    $$PWD/CCprogressbar.hpp \
    $$PWD/ScreenTransition.hpp \
    $$PWD/GameLoader.hpp \
    $$PWD/GameLoaderThread.hpp \
    $$PWD/MainScreen.hpp \
    $$PWD/CCTitle.hpp \
    $$PWD/OptionsDialog.hpp \
    $$PWD/Solo.hpp \
    $$PWD/Multi.hpp \
    $$PWD/Login.hpp \
    $$PWD/AddServer.hpp \
    $$PWD/AskKey.hpp \
    $$PWD/ConnexionManager.hpp \
    $$PWD/ConnectedSocket.hpp \
    $$PWD/FeedNews.hpp \
    $$PWD/BlacklistPassword.hpp \
    $$PWD/../tarcompressed/ZstdDecode.hpp \
    $$PWD/../tarcompressed/TarDecode.hpp \
    $$PWD/QZstdDecodeThread.hpp

FORMS    += $$PWD/interface/BaseWindow.ui \
    $$PWD/interface/Chat.ui \
    $$PWD/interface/WithAnotherPlayer.ui \
    $$PWD/interface/GetPrice.ui \
    $$PWD/interface/NewProfile.ui \
    $$PWD/interface/NewGame.ui \
    $$PWD/LanguagesSelect.ui \
    $$PWD/LoadingScreen.ui \
    $$PWD/MainScreen.ui \
    $$PWD/OptionsDialog.ui \
    $$PWD/Solo.ui \
    $$PWD/Multi.ui \
    $$PWD/Login.ui \
    $$PWD/AddServer.ui \
    $$PWD/AskKey.ui

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/../resources/resources-windows.rc
ICON = $$PWD/../resources/client.icns
macx:INCLUDEPATH += /Users/user/Desktop/VLC.app/Contents/MacOS/include/
macx:LIBS += -L/Users/user/Desktop/VLC.app/Contents/MacOS/lib/

RESOURCES += $$PWD/../resources/client-resources.qrc \
    $$PWD/crafting/resources/client-resources-plant.qrc \
    $$PWD/fight/resources/client-resources-fight.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/translation.ts \
    $$PWD/languages/fr/translation.ts

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
