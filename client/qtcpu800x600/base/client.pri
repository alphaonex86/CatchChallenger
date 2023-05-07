QT       += gui network core widgets
QT       += qml quick opengl

DEFINES += CATCHCHALLENGER_CLIENT

wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
}
else
{
#    DEFINES += NOWEBSOCKET
}
!contains(DEFINES, NOWEBSOCKET) {
    QT += websockets
}

SOURCES += \
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
    $$PWD/../crafting/interface/QmlInterface/CraftingAnimation.cpp \
    $$PWD/../crafting/interface/BaseWindowCrafting.cpp \
    $$PWD/../fight/interface/QmlInterface/QmlMonsterGeneralInformations.cpp \
    $$PWD/../fight/interface/QmlInterface/EvolutionControl.cpp \
    $$PWD/../fight/interface/BaseWindowFight.cpp \
    $$PWD/../fight/interface/BaseWindowFightNextAction.cpp \
    $$PWD/QmlInterface/AnimationControl.cpp \
    $$PWD/Options.cpp \
    $$PWD/LanguagesSelect.cpp \
    $$PWD/CachedString.cpp \
    $$PWD/FacilityLibClient.cpp

HEADERS  += \
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
    $$PWD/QmlInterface/AnimationControl.h \
    $$PWD/LanguagesSelect.h \
    $$PWD/CachedString.h \
    $$PWD/Options.h \
    $$PWD/FacilityLibClient.h

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

RESOURCES += $$PWD/resources/client-resources.qrc \
    $$PWD/../crafting/resources/client-resources-plant.qrc \
    $$PWD/../fight/resources/client-resources-fight.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/translation.ts \
    $$PWD/languages/fr/translation.ts
