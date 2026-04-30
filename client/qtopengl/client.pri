QT       += gui network core widgets opengl openglwidgets xml

DEFINES += CATCHCHALLENGER_CLIENT

wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
#android: DEFINES += CATCHCHALLENGER_NOAUDIO
# see the file ClientVariableAudio.h
#DEFINES += CATCHCHALLENGER_NOAUDIO
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
}

SOURCES += \
    $$PWD/CCGraphicsTextItem.cpp \
    $$PWD/CCSliderH.cpp \
    $$PWD/MultiItem.cpp \
    $$PWD/ConnexionInfo.cpp \
    $$PWD/CCScrollZone.cpp \
    $$PWD/LineEdit.cpp \
    $$PWD/ComboBox.cpp \
    $$PWD/SpinBox.cpp \
    $$PWD/above/AddOrEditServer.cpp \
    $$PWD/above/OptionsDialog.cpp \
    $$PWD/above/Login.cpp \
    $$PWD/CheckBox.cpp \
    $$PWD/foreground/Battle.cpp \
    $$PWD/foreground/MainScreen.cpp \
    $$PWD/foreground/Multi.cpp \
    $$PWD/foreground/CharacterList.cpp \
    $$PWD/foreground/SubServer.cpp \
    $$PWD/foreground/QGraphicsTextItemOutline.cpp \
    $$PWD/above/AddCharacter.cpp \
    $$PWD/above/NewGame.cpp \
    $$PWD/FacilityLibClient.cpp \
    $$PWD/background/CCMap.cpp \
    $$PWD/foreground/OverMap.cpp \
    $$PWD/CustomText.cpp \
    $$PWD/foreground/widgets/MapMonsterPreview.cpp \
    $$PWD/ImagesStrechMiddle.cpp \
    $$PWD/MyApplication.cpp \
    $$PWD/above/DebugDialog.cpp \
    $$PWD/foreground/OverMapLogic.cpp \
    $$PWD/foreground/OverMapLogicTrade.cpp \
    $$PWD/foreground/OverMapLogicShop.cpp \
    $$PWD/foreground/OverMapLogicBot.cpp \
    $$PWD/foreground/OverMapLogicFactory.cpp \
    $$PWD/foreground/OverMapLogicClan.cpp \
    $$PWD/above/inventory/Inventory.cpp \
    $$PWD/above/inventory/Plant.cpp \
    $$PWD/above/inventory/Crafting.cpp \
    $$PWD/above/Animation.cpp \
    $$PWD/above/Encyclopedia.cpp \
    $$PWD/above/Shop.cpp \
    $$PWD/above/Factory.cpp \
    $$PWD/above/Trade.cpp \
    $$PWD/above/Warehouse.cpp \
    $$PWD/above/player/Player.cpp \
    $$PWD/above/player/Reputations.cpp \
    $$PWD/above/player/Quests.cpp \
    $$PWD/above/player/FinishedQuests.cpp \
    $$PWD/QGraphicsPixmapItemClick.cpp \
    $$PWD/above/MonsterDetails.cpp \
    $$PWD/GameLoader.cpp \
    $$PWD/ChatParsing.cpp \
    $$PWD/GameLoaderThread.cpp \
    $$PWD/ProgressBarPixel.cpp
HEADERS  += \
    $$PWD/CCGraphicsTextItem.hpp \
    $$PWD/CCSliderH.hpp \
    $$PWD/MultiItem.hpp \
    $$PWD/ConnexionInfo.hpp \
    $$PWD/CCScrollZone.hpp \
    $$PWD/LineEdit.hpp \
    $$PWD/ComboBox.hpp \
    $$PWD/SpinBox.hpp \
    $$PWD/above/AddOrEditServer.hpp \
    $$PWD/above/OptionsDialog.hpp \
    $$PWD/above/Login.hpp \
    $$PWD/CheckBox.hpp \
    $$PWD/foreground/Battle.hpp \
    $$PWD/foreground/MainScreen.hpp \
    $$PWD/foreground/Multi.hpp \
    $$PWD/foreground/CharacterList.hpp \
    $$PWD/foreground/SubServer.hpp \
    $$PWD/foreground/QGraphicsTextItemOutline.hpp \
    $$PWD/above/AddCharacter.hpp \
    $$PWD/above/NewGame.hpp \
    $$PWD/FacilityLibClient.hpp \
    $$PWD/background/CCMap.hpp \
    $$PWD/foreground/OverMap.hpp \
    $$PWD/CustomText.hpp \
    $$PWD/foreground/widgets/MapMonsterPreview.hpp \
    $$PWD/ImagesStrechMiddle.hpp \
    $$PWD/MyApplication.h \
    $$PWD/above/DebugDialog.hpp \
    $$PWD/foreground/OverMapLogic.hpp \
    $$PWD/above/inventory/Inventory.hpp \
    $$PWD/above/inventory/Plant.hpp \
    $$PWD/above/inventory/Crafting.hpp \
    $$PWD/above/Animation.hpp \
    $$PWD/above/Encyclopedia.hpp \
    $$PWD/above/Shop.hpp \
    $$PWD/above/Factory.hpp \
    $$PWD/above/Trade.hpp \
    $$PWD/above/Warehouse.hpp \
    $$PWD/above/player/Player.hpp \
    $$PWD/above/player/Quests.hpp \
    $$PWD/above/player/Reputations.hpp \
    $$PWD/above/player/FinishedQuests.hpp \
    $$PWD/QGraphicsPixmapItemClick.hpp \
    $$PWD/above/MonsterDetails.hpp \
    $$PWD/GameLoader.hpp \
    $$PWD/ChatParsing.hpp \
    $$PWD/GameLoaderThread.hpp \
    $$PWD/ProgressBarPixel.hpp


wasm: {
    DEFINES += NOTCPSOCKET
}
else
{
    DEFINES += CATCHCHALLENGER_SOLO
#    DEFINES += NOWEBSOCKET
}
android: {
    #Qt6 dropped QT_androidextras and the qmake feature pulls Qt5-only includes;
    #the previously-hardcoded NDK r19c / Qt5.12.4 paths no longer exist on this
    #host and androidextras isn't published as a Qt6 module either, so the
    #qmake invocation fails with "Unknown module(s) in QT: androidextras".
    #Drop it and keep just the Android packaging hooks (Qt6 has the JNI APIs
    #built into QtCore now).
    QMAKE_CFLAGS += -g
    QMAKE_CXXFLAGS += -g
    DISTFILES += $$PWD/../android-sources/AndroidManifest.xml
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../android-sources
}
!contains(DEFINES, NOWEBSOCKET) {
    QT += websockets
}

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/CCTitle.cpp \
    $$PWD/ConnexionManager.cpp \
    $$PWD/foreground/LoadingScreen.cpp \
    $$PWD/CustomButton.cpp \
    $$PWD/ScreenTransition.cpp \
    $$PWD/background/CCBackground.cpp \
    $$PWD/CCprogressbar.cpp \
    $$PWD/ScreenInput.cpp \
    $$PWD/AudioGL.cpp

HEADERS  += \
    $$PWD/CCprogressbar.hpp \
    $$PWD/ClientStructures.hpp \
    $$PWD/CCTitle.hpp \
    $$PWD/ConnexionManager.hpp \
    $$PWD/CustomButton.hpp \
    $$PWD/foreground/LoadingScreen.hpp \
    $$PWD/DisplayStructures.hpp \
    $$PWD/ScreenTransition.hpp \
    $$PWD/background/CCBackground.hpp \
    $$PWD/ScreenInput.hpp \
    $$PWD/AudioGL.hpp

DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/resources/resources-windows.rc
ICON = $$PWD/resources/client.icns

RESOURCES += $$PWD/resources/client-resources.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/translation.ts \
    $$PWD/../languages/fr/translation.ts


