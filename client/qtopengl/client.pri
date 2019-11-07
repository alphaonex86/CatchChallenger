include(../libcatchchallenger/lib.pri)
include(../qt/tiled/tiled.pri)

QT       += gui network core widgets opengl

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
    $$PWD/../qt/Audio.cpp \
    $$PWD/../qt/QInfiniteBuffer.cpp

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h \
    $$PWD/../qt/Audio.h \
    $$PWD/../qt/QInfiniteBuffer.h
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
    $$PWD/main.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp \
    $$PWD/LanguagesSelect.cpp \
    $$PWD/CCTitle.cpp \
    $$PWD/ConnexionManager.cpp \
    $$PWD/LoadingScreen.cpp \
    $$PWD/CCWidget.cpp \
    $$PWD/CustomButton.cpp \
    $$PWD/MainScreen.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/ScreenTransition.cpp \
    $$PWD/Multi.cpp \
    $$PWD/OptionsDialog.cpp \
    $$PWD/CCBackground.cpp \
    $$PWD/CCprogressbar.cpp \
    $$PWD/render/MapController.cpp \
    $$PWD/render/MapControllerCrafting.cpp \
    $$PWD/render/MapControllerMP.cpp \
    $$PWD/render/MapControllerMPAPI.cpp \
    $$PWD/render/MapControllerMPMove.cpp \
    $$PWD/render/MapDoor.cpp \
    $$PWD/render/MapItem.cpp \
    $$PWD/render/MapMark.cpp \
    $$PWD/render/MapObjectItem.cpp \
    $$PWD/render/MapVisualiser-map.cpp \
    $$PWD/render/MapVisualiser.cpp \
    $$PWD/render/MapVisualiserOrder.cpp \
    $$PWD/render/MapVisualiserPlayer.cpp \
    $$PWD/render/MapVisualiserPlayerWithFight.cpp \
    $$PWD/render/MapVisualiserThread.cpp \
    $$PWD/render/ObjectGroupItem.cpp \
    $$PWD/render/PathFinding.cpp \
    $$PWD/render/PreparedLayer.cpp \
    $$PWD/render/TemporaryTile.cpp \
    $$PWD/render/TileLayerItem.cpp \
    $$PWD/render/TriggerAnimation.cpp \
    $$PWD/interface/BaseWindow.cpp \
    $$PWD/interface/BaseWindowBot.cpp \
    $$PWD/interface/BaseWindowCharacter.cpp \
    $$PWD/interface/BaseWindowCity.cpp \
    $$PWD/interface/BaseWindowClan.cpp \
    $$PWD/interface/BaseWindowCrafting.cpp \
    $$PWD/interface/BaseWindowFactory.cpp \
    $$PWD/interface/BaseWindowFight.cpp \
    $$PWD/interface/BaseWindowFightNextAction.cpp \
    $$PWD/interface/BaseWindowLoad.cpp \
    $$PWD/interface/BaseWindowMap.cpp \
    $$PWD/interface/BaseWindowMarket.cpp \
    $$PWD/interface/BaseWindowOptions.cpp \
    $$PWD/interface/BaseWindowSelection.cpp \
    $$PWD/interface/BaseWindowShop.cpp \
    $$PWD/interface/BaseWindowWarehouse.cpp \
    $$PWD/interface/Chat.cpp \
    $$PWD/interface/ClientFightEngine.cpp \
    $$PWD/interface/DatapackClientLoaderCrafting.cpp \
    $$PWD/interface/DatapackClientLoaderFight.cpp \
    $$PWD/interface/GetPrice.cpp \
    $$PWD/interface/ListEntryEnvolued.cpp \
    $$PWD/interface/NewGame.cpp \
    $$PWD/interface/NewProfile.cpp \
    $$PWD/interface/WithAnotherPlayer.cpp

HEADERS  += \
    $$PWD/../../general/base/tinyXML2/tinyxml2.h \
    $$PWD/LanguagesSelect.h \
    $$PWD/CCprogressbar.h \
    $$PWD/ClientStructures.h \
    $$PWD/CCTitle.h \
    $$PWD/ConnexionManager.h \
    $$PWD/CCWidget.h \
    $$PWD/CustomButton.h \
    $$PWD/LoadingScreen.h \
    $$PWD/DisplayStructures.h \
    $$PWD/MainScreen.h \
    $$PWD/Map_client.h \
    $$PWD/OptionsDialog.h \
    $$PWD/ScreenTransition.h \
    $$PWD/Multi.h \
    $$PWD/CCBackground.h \
    $$PWD/render/MapController.h \
    $$PWD/render/MapControllerMP.h \
    $$PWD/render/MapDoor.h \
    $$PWD/render/MapItem.h \
    $$PWD/render/MapMark.h \
    $$PWD/render/MapObjectItem.h \
    $$PWD/render/MapVisualiser.h \
    $$PWD/render/MapVisualiserOrder.h \
    $$PWD/render/MapVisualiserPlayer.h \
    $$PWD/render/MapVisualiserPlayerWithFight.h \
    $$PWD/render/MapVisualiserThread.h \
    $$PWD/render/ObjectGroupItem.h \
    $$PWD/render/PathFinding.h \
    $$PWD/render/PreparedLayer.h \
    $$PWD/render/TemporaryTile.h \
    $$PWD/render/TileLayerItem.h \
    $$PWD/render/TriggerAnimation.h \
    $$PWD/interface/BaseWindow.h \
    $$PWD/interface/Chat.h \
    $$PWD/interface/ClientFightEngine.h \
    $$PWD/interface/GetPrice.h \
    $$PWD/interface/ListEntryEnvolued.h \
    $$PWD/interface/NewGame.h \
    $$PWD/interface/NewProfile.h \
    $$PWD/interface/WithAnotherPlayer.h

RESOURCES += $$PWD/../resources/client-resources-multi.qrc
DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/../resources/resources-windows.rc
ICON = $$PWD/../resources/client.icns
macx:INCLUDEPATH += /Users/user/Desktop/VLC.app/Contents/MacOS/include/
macx:LIBS += -L/Users/user/Desktop/VLC.app/Contents/MacOS/lib/

RESOURCES += $$PWD/../resources/client-resources.qrc \
    $$PWD/../qt/crafting/resources/client-resources-plant.qrc \
    $$PWD/../qt/fight/resources/client-resources-fight.qrc

TRANSLATIONS    = $$PWD/../resources/languages/en/translation.ts \
    $$PWD/../languages/fr/translation.ts

#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-server-qt.pri)
QT       += sql
SOURCES += $$PWD/../qt/solo/InternalServer.cpp \
    $$PWD/../qt/solo/SoloWindow.cpp
HEADERS  += $$PWD/../qt/solo/InternalServer.h \
    $$PWD/../qt/solo/SoloWindow.h
}

DISTFILES += \
    $$PWD/client.pri
