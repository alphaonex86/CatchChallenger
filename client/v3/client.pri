include(libraries/tiled/tiled.pri)
include(libraries/gifimage/qtgifimage.pri)

QT       += gui network core widgets opengl xml

DEFINES += CATCHCHALLENGER_CLIENT
CONFIG += object_parallel_to_source

wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
#android: DEFINES += CATCHCHALLENGER_NOAUDIO
# see the file ClientVariableAudio.h
#DEFINES += CATCHCHALLENGER_NOAUDIO
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia

    INCLUDEPATH += $$PWD/../libopus/include/
    #Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode.
    DEFINES += USE_ALLOCA OPUS_BUILD
}

SOURCES += \
    $$PWD/base/ConnectionInfo.cpp \
    $$PWD/ui/Label.cpp \
    $$PWD/ui/ListView.cpp \
    $$PWD/ui/Button.cpp \
    $$PWD/ui/RoundedButton.cpp \
    $$PWD/ui/Progressbar.cpp \
    $$PWD/ui/SlimProgressbar.cpp \
    $$PWD/ui/Input.cpp \
    $$PWD/ui/Combo.cpp \
    $$PWD/ui/Dialog.cpp \
    $$PWD/ui/Checkbox.cpp \
    $$PWD/ui/MessageDialog.cpp \
    $$PWD/ui/InputDialog.cpp \
    $$PWD/ui/ThemedRoundedButton.cpp \
    $$PWD/ui/ThemedButton.cpp \
    $$PWD/ui/ThemedLabel.cpp \
    $$PWD/ui/GridView.cpp \
    $$PWD/ui/LinkedDialog.cpp \
    $$PWD/ui/Slider.cpp \
    $$PWD/ui/Row.cpp \
    $$PWD/ui/Column.cpp \
    $$PWD/ui/SelectableItem.cpp \
    $$PWD/ui/ThemedItem.cpp \
    $$PWD/ui/Backdrop.cpp \
    $$PWD/ui/MessageBar.cpp \
    $$PWD/ui/PlainLabel.cpp \
    $$PWD/core/Node.cpp \
    $$PWD/core/Stateful.cpp \
    $$PWD/core/Sprite.cpp \
    $$PWD/core/ActionManager.cpp \
    $$PWD/core/EventManager.cpp \
    $$PWD/core/Scene.cpp \
    $$PWD/core/SceneManager.cpp \
    $$PWD/core/StackedScene.cpp \
    $$PWD/core/GraphicRenderer.cpp \
    $$PWD/core/ProgressNotifier.cpp \
    $$PWD/core/VirtualInput.cpp \
    $$PWD/action/Action.cpp \
    $$PWD/action/MoveTo.cpp \
    $$PWD/action/Delay.cpp \
    $$PWD/action/Animation.cpp \
    $$PWD/action/Sequence.cpp \
    $$PWD/action/Spawn.cpp \
    $$PWD/action/Tick.cpp \
    $$PWD/action/Blink.cpp \
    $$PWD/action/CallFunc.cpp \
    $$PWD/action/RepeatForever.cpp \
    $$PWD/entities/BlacklistPassword.cpp \
    $$PWD/FacilityLibClient.cpp \
    $$PWD/MyApplication.cpp \
    $$PWD/scenes/ParallaxForest.cpp \
    $$PWD/scenes/battle/Battle.cpp \
    $$PWD/scenes/battle/ActionBar.cpp \
    $$PWD/scenes/battle/SkillButton.cpp \
    $$PWD/scenes/battle/BattleActions.cpp \
    $$PWD/scenes/battle/StatusCard.cpp \
    $$PWD/scenes/battle/BattleStates.cpp \
    $$PWD/scenes/battle/MonsterSwap.cpp \
    $$PWD/scenes/map/CCMap.cpp \
    $$PWD/scenes/map/Map.cpp \
    $$PWD/scenes/map/MapMonsterPreview.cpp \
    $$PWD/scenes/map/MonsterDetails.cpp \
    $$PWD/scenes/map/OverMap.cpp \
    $$PWD/scenes/map/OverMapLogic.cpp \
    $$PWD/scenes/map/OverMapLogicBot.cpp \
    $$PWD/scenes/map/OverMapLogicClan.cpp \
    $$PWD/scenes/map/OverMapLogicTrade.cpp \
    $$PWD/scenes/map/PlayerPortrait.cpp \
    $$PWD/scenes/map/Tip.cpp \
    $$PWD/scenes/map/MonsterThumb.cpp \
    $$PWD/scenes/shared/player/Player.cpp \
    $$PWD/scenes/shared/player/Clan.cpp \
    $$PWD/scenes/shared/player/FinishedQuests.cpp \
    $$PWD/scenes/shared/player/Quests.cpp \
    $$PWD/scenes/shared/player/QuestItem.cpp \
    $$PWD/scenes/shared/player/Reputations.cpp \
    $$PWD/scenes/shared/inventory/MonsterItem.cpp \
    $$PWD/scenes/shared/inventory/MonsterBag.cpp \
    $$PWD/scenes/shared/npctalk/NpcTalk.cpp \
    $$PWD/scenes/shared/crafting/Crafting.cpp \
    $$PWD/scenes/shared/crafting/CraftingItem.cpp \
    $$PWD/scenes/shared/inventory/Inventory.cpp \
    $$PWD/scenes/shared/inventory/InventoryItem.cpp \
    $$PWD/scenes/shared/toast/Toast.cpp \
    $$PWD/scenes/shared/inventory/Plant.cpp \
    $$PWD/scenes/shared/inventory/PlantItem.cpp \
    $$PWD/scenes/shared/shop/Shop.cpp \
    $$PWD/scenes/shared/shop/ShopItem.cpp \
    $$PWD/scenes/shared/learn/Learn.cpp \
    $$PWD/scenes/shared/warehouse/Warehouse.cpp \
    $$PWD/scenes/shared/warehouse/WarehouseItem.cpp \
    $$PWD/scenes/shared/industry/Industry.cpp \
    $$PWD/scenes/shared/industry/IndustryItem.cpp \
    $$PWD/scenes/shared/zonecatch/ZoneCatch.cpp \
    $$PWD/scenes/shared/chat/Chat.cpp \
    $$PWD/scenes/test/Test.cpp \
    $$PWD/scenes/test/TestItem.cpp \
    $$PWD/scenes/leading/Leading.cpp \
    $$PWD/scenes/leading/Loading.cpp \
    $$PWD/scenes/leading/Multi.cpp \
    $$PWD/scenes/leading/MultiItem.cpp \
    $$PWD/scenes/leading/AddOrEditServer.cpp \
    $$PWD/scenes/leading/Login.cpp \
    $$PWD/scenes/leading/SubServer.cpp \
    $$PWD/scenes/leading/SubServerItem.cpp \
    $$PWD/scenes/leading/NewGame.cpp \
    $$PWD/scenes/leading/AddCharacter.cpp \
    $$PWD/scenes/leading/CharacterSelector.cpp \
    $$PWD/scenes/leading/CharacterItem.cpp \
    $$PWD/scenes/leading/Options.cpp \
    $$PWD/scenes/leading/Debug.cpp \
    $$PWD/scenes/leading/Menu.cpp
!contains(DEFINES, NOSINGLEPLAYER) {
SOURCES += $$PWD/scenes/leading/Solo.cpp
HEADERS  += $$PWD/scenes/leading/Solo.hpp
}
HEADERS  += \
    $$PWD/base/ConnectionInfo.cpp \
    $$PWD/ui/Label.hpp \
    $$PWD/ui/ListView.hpp \
    $$PWD/ui/Button.hpp \
    $$PWD/ui/RoundedButton.hpp \
    $$PWD/ui/Progressbar.hpp \
    $$PWD/ui/SlimProgressbar.hpp \
    $$PWD/ui/Input.hpp \
    $$PWD/ui/Combo.hpp \
    $$PWD/ui/Dialog.hpp \
    $$PWD/ui/Checkbox.hpp \
    $$PWD/ui/MessageDialog.hpp \
    $$PWD/ui/InputDialog.hpp \
    $$PWD/ui/ThemedRoundedButton.hpp \
    $$PWD/ui/ThemedButton.hpp \
    $$PWD/ui/ThemedLabel.hpp \
    $$PWD/ui/GridView.hpp \
    $$PWD/ui/LinkedDialog.hpp \
    $$PWD/ui/Slider.hpp \
    $$PWD/ui/Row.hpp \
    $$PWD/ui/Column.hpp \
    $$PWD/ui/SelectableItem.hpp \
    $$PWD/ui/ThemedItem.hpp \
    $$PWD/ui/Backdrop.hpp \
    $$PWD/ui/MessageBar.hpp \
    $$PWD/ui/PlainLabel.hpp \
    $$PWD/core/uthash.h \
    $$PWD/core/Vector.hpp \
    $$PWD/core/Node.hpp \
    $$PWD/core/Stateful.hpp \
    $$PWD/core/Sprite.hpp \
    $$PWD/core/ActionManager.hpp \
    $$PWD/core/EventManager.hpp \
    $$PWD/core/Scene.hpp \
    $$PWD/core/SceneManager.hpp \
    $$PWD/core/StackedScene.hpp \
    $$PWD/core/GraphicRenderer.hpp \
    $$PWD/core/ProgressNotifier.hpp \
    $$PWD/core/VirtualInput.hpp \
    $$PWD/action/Action.hpp \
    $$PWD/action/MoveTo.hpp \
    $$PWD/action/Delay.hpp \
    $$PWD/action/Animation.hpp \
    $$PWD/action/Sequence.hpp \
    $$PWD/action/Spawn.hpp \
    $$PWD/action/Tick.hpp \
    $$PWD/action/Blink.hpp \
    $$PWD/action/CallFunc.hpp \
    $$PWD/action/RepeatForever.hpp \
    $$PWD/entities/BlacklistPassword.hpp \
    $$PWD/FacilityLibClient.hpp \
    $$PWD/MyApplication.h \
    $$PWD/scenes/ParallaxForest.hpp \
    $$PWD/scenes/battle/Battle.hpp \
    $$PWD/scenes/battle/ActionBar.hpp \
    $$PWD/scenes/battle/SkillButton.cpp \
    $$PWD/scenes/battle/BattleActions.hpp \
    $$PWD/scenes/battle/StatusCard.hpp \
    $$PWD/scenes/battle/BattleStates.hpp \
    $$PWD/scenes/battle/MonsterSwap.hpp \
    $$PWD/scenes/map/Map.hpp \
    $$PWD/scenes/map/CCMap.hpp \
    $$PWD/scenes/map/MapMonsterPreview.hpp \
    $$PWD/scenes/map/MonsterDetails.hpp \
    $$PWD/scenes/map/OverMap.hpp \
    $$PWD/scenes/map/OverMapLogic.hpp \
    $$PWD/scenes/map/PlayerPortrait.hpp \
    $$PWD/scenes/map/MonsterThumb.hpp \
    $$PWD/scenes/map/Tip.hpp \
    $$PWD/scenes/shared/crafting/Crafting.hpp \
    $$PWD/scenes/shared/crafting/CraftingItem.hpp \
    $$PWD/scenes/shared/player/Player.hpp \
    $$PWD/scenes/shared/player/Clan.hpp \
    $$PWD/scenes/shared/player/FinishedQuests.hpp \
    $$PWD/scenes/shared/player/Quests.hpp \
    $$PWD/scenes/shared/player/QuestItem.hpp \
    $$PWD/scenes/shared/player/Reputations.hpp \
    $$PWD/scenes/shared/inventory/MonsterItem.hpp \
    $$PWD/scenes/shared/inventory/MonsterBag.hpp \
    $$PWD/scenes/shared/npctalk/NpcTalk.hpp \
    $$PWD/scenes/shared/inventory/Inventory.hpp \
    $$PWD/scenes/shared/inventory/InventoryItem.hpp \
    $$PWD/scenes/shared/toast/Toast.hpp \
    $$PWD/scenes/shared/inventory/Plant.hpp \
    $$PWD/scenes/shared/inventory/PlantItem.hpp \
    $$PWD/scenes/shared/shop/Shop.hpp \
    $$PWD/scenes/shared/shop/ShopItem.hpp \
    $$PWD/scenes/shared/learn/Learn.hpp \
    $$PWD/scenes/shared/warehouse/Warehouse.hpp \
    $$PWD/scenes/shared/warehouse/WarehouseItem.hpp \
    $$PWD/scenes/shared/industry/Industry.hpp \
    $$PWD/scenes/shared/industry/IndustryItem.hpp \
    $$PWD/scenes/shared/zonecatch/ZoneCatch.hpp \
    $$PWD/scenes/shared/chat/Chat.hpp \
    $$PWD/scenes/test/Test.hpp \
    $$PWD/scenes/test/TestItem.hpp \
    $$PWD/scenes/leading/Leading.hpp \
    $$PWD/scenes/leading/Loading.hpp \
    $$PWD/scenes/leading/MultiItem.hpp \
    $$PWD/scenes/leading/Multi.hpp \
    $$PWD/scenes/leading/AddOrEditServer.hpp \
    $$PWD/scenes/leading/Login.hpp \
    $$PWD/scenes/leading/SubServer.hpp \
    $$PWD/scenes/leading/SubServerItem.hpp \
    $$PWD/scenes/leading/NewGame.hpp \
    $$PWD/scenes/leading/AddCharacter.hpp \
    $$PWD/scenes/leading/CharacterSelector.hpp \
    $$PWD/scenes/leading/CharacterItem.hpp \
    $$PWD/scenes/leading/Options.hpp \
    $$PWD/scenes/leading/Debug.hpp \
    $$PWD/scenes/leading/Menu.hpp

SOURCES += \
    $$PWD/entities/render/MapController.cpp \
    $$PWD/entities/render/MapControllerCrafting.cpp \
    $$PWD/entities/render/MapControllerMP.cpp \
    $$PWD/entities/render/MapControllerMPAPI.cpp \
    $$PWD/entities/render/MapControllerMPMove.cpp \
    $$PWD/entities/render/MapDoor.cpp \
    $$PWD/entities/render/MapItem.cpp \
    $$PWD/entities/render/MapMark.cpp \
    $$PWD/entities/render/MapObjectItem.cpp \
    $$PWD/entities/render/MapVisualiser-map.cpp \
    $$PWD/entities/render/MapVisualiser.cpp \
    $$PWD/entities/render/MapVisualiserOrder.cpp \
    $$PWD/entities/render/MapVisualiserPlayer.cpp \
    $$PWD/entities/render/MapVisualiserPlayerWithFight.cpp \
    $$PWD/entities/render/MapVisualiserThread.cpp \
    $$PWD/entities/render/ObjectGroupItem.cpp \
    $$PWD/entities/render/PathFinding.cpp \
    $$PWD/entities/render/PreparedLayer.cpp \
    $$PWD/entities/render/TemporaryTile.cpp \
    $$PWD/entities/render/TileLayerItem.cpp \
    $$PWD/entities/render/TriggerAnimation.cpp \
    $$PWD/entities/Map_client.cpp \
    $$PWD/entities/PlayerInfo.cpp \
    $$PWD/entities/ActionUtils.cpp \
    $$PWD/entities/Shapes.cpp \
    $$PWD/entities/Utils.cpp
HEADERS  += \
    $$PWD/entities/render/MapController.hpp \
    $$PWD/entities/render/MapControllerMP.hpp \
    $$PWD/entities/render/MapDoor.hpp \
    $$PWD/entities/render/MapItem.hpp \
    $$PWD/entities/render/MapMark.hpp \
    $$PWD/entities/render/MapObjectItem.hpp \
    $$PWD/entities/render/MapVisualiser.hpp \
    $$PWD/entities/render/MapVisualiserOrder.hpp \
    $$PWD/entities/render/MapVisualiserPlayer.hpp \
    $$PWD/entities/render/MapVisualiserPlayerWithFight.hpp \
    $$PWD/entities/render/MapVisualiserThread.hpp \
    $$PWD/entities/render/ObjectGroupItem.hpp \
    $$PWD/entities/render/PathFinding.hpp \
    $$PWD/entities/render/PreparedLayer.hpp \
    $$PWD/entities/render/TemporaryTile.hpp \
    $$PWD/entities/render/TileLayerItem.hpp \
    $$PWD/entities/render/TriggerAnimation.hpp \
    $$PWD/entities/Map_client.hpp \
    $$PWD/entities/PlayerInfo.hpp \
    $$PWD/entities/ActionUtils.hpp \
    $$PWD/entities/Shapes.hpp \
    $$PWD/entities/Utils.hpp

wasm: {
    DEFINES += NOTCPSOCKET
}
else
{
#    DEFINES += NOWEBSOCKET
}


android: {
    INCLUDEPATH += /var/lib/docker/overlay2/e914fb178b67f59eee382992f7f454c694a769808d18e8f452675ee1959aae30/diff/opt/android-sdk/ndk-r19c/platforms/android-26/arch-arm64/usr/include/
    INCLUDEPATH += /var/lib/docker/overlay2/e914fb178b67f59eee382992f7f454c694a769808d18e8f452675ee1959aae30/diff/opt/android-sdk/ndk-r19c/platforms/android-26/arch-arm/usr/include/
    LIBS += -L/opt/Qt5.12.4/5.12.4/android_arm64_v8a/lib/
    LIBS += -L/opt/Qt5.12.4/5.12.4/android_armv7/lib/
    QT += androidextras
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
    $$PWD/base/ConnectionManager.cpp \
    $$PWD/Globals.cpp \
    $$PWD/Ultimate.cpp \
    $$PWD/Options.cpp \
    $$PWD/entities/InternetUpdater.cpp \
    $$PWD/entities/FeedNews.cpp \
    $$PWD/base/ExtraSocket.cpp \
    $$PWD/base/LocalListener.cpp \
    $$PWD/core/AssetsLoader.cpp \
    $$PWD/core/AssetsLoaderThread.cpp \
    $$PWD/core/Logger.cpp \
    $$PWD/core/AudioPlayer.cpp

HEADERS  += \
    $$PWD/base/ConnectionManager.hpp \
    $$PWD/Globals.hpp \
    $$PWD/Ultimate.hpp \
    $$PWD/Options.hpp \
    $$PWD/entities/InternetUpdater.hpp \
    $$PWD/entities/FeedNews.hpp \
    $$PWD/base/ExtraSocket.hpp \
    $$PWD/base/LocalListener.hpp \
    $$PWD/core/AssetsLoader.hpp \
    $$PWD/core/AssetsLoaderThread.hpp \
    $$PWD/core/Logger.hpp \
    $$PWD/core/AudioPlayer.hpp

DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/../resources/resources-windows.rc
ICON = $$PWD/../resources/client.icns

RESOURCES += $$PWD/../resources/client-resources.qrc

deployment.files=$$PWD/../datapack.tar
android {
    deployment.path=/assets
}
INSTALLS += deployment

TRANSLATIONS    = $$PWD/../resources/languages/en/translation.ts \
    $$PWD/../languages/fr/translation.ts

include(viewports/viewport.pri)

DISTFILES += \
    $$PWD/client.pri

#FORMS += \
#    $$PWD/../qt/LanguagesSelect.ui
