CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
QMAKE_CFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"

QT       -= core

wasm: {
    #LIBS += -Lcrypto
    QMAKE_LFLAGS += -s TOTAL_MEMORY=200015872
    #QMAKE_LFLAGS += -s EMTERPRETIFY_ASYNC=1 -s EMTERPRETIFY=1
    QMAKE_CLFLAGS += -g
    QMAKE_CXXFLAGS += -g
    QMAKE_LFLAGS += -s ASSERTIONS=1 -g
    #CONFIG+=debug
}
android: {
INCLUDEPATH += /opt/android-sdk/ndk-r18b/platforms/android-21/arch-arm/usr/include
}

TEMPLATE = app

SOURCES += $$PWD/base/ChatParsing.cpp \
    $$PWD/base/ProtocolParsingGeneral.cpp \
    $$PWD/base/ProtocolParsingInput.cpp \
    $$PWD/base/ProtocolParsingOutput.cpp \
    $$PWD/base/ProtocolParsingCheck.cpp \
    $$PWD/base/MoveOnTheMap.cpp \
    $$PWD/base/Map_loader.cpp \
    $$PWD/base/Map_loaderMain.cpp \
    $$PWD/base/FacilityLib.cpp \
    $$PWD/base/FacilityLibGeneral.cpp \
    $$PWD/base/DatapackGeneralLoader.cpp \
    $$PWD/base/DatapackGeneralLoaderCrafting.cpp \
    $$PWD/base/DatapackGeneralLoaderIndustry.cpp \
    $$PWD/base/DatapackGeneralLoaderItem.cpp \
    $$PWD/base/DatapackGeneralLoaderMap.cpp \
    $$PWD/base/DatapackGeneralLoaderMonsterDrop.cpp \
    $$PWD/base/DatapackGeneralLoaderPlant.cpp \
    $$PWD/base/DatapackGeneralLoaderQuest.cpp \
    $$PWD/base/DatapackGeneralLoaderReputation.cpp \
    $$PWD/base/cpp11addition.cpp \
    $$PWD/base/cpp11additionstringtointcpp.cpp \
    $$PWD/base/cpp11additionstringtointc.cpp \
    $$PWD/fight/FightLoader.cpp \
    $$PWD/fight/FightLoaderBuff.cpp \
    $$PWD/fight/FightLoaderFight.cpp \
    $$PWD/fight/FightLoaderMonster.cpp \
    $$PWD/fight/FightLoaderSkill.cpp \
    $$PWD/base/CommonMap.cpp \
    $$PWD/base/CommonDatapack.cpp \
    $$PWD/base/CommonDatapackServerSpec.cpp \
    $$PWD/base/CommonSettingsCommon.cpp \
    $$PWD/base/CommonSettingsServer.cpp \
    $$PWD/fight/CommonFightEngine.cpp \
    $$PWD/fight/CommonFightEngineEnd.cpp \
    $$PWD/fight/CommonFightEngineTurn.cpp \
    $$PWD/fight/CommonFightEngineBuff.cpp \
    $$PWD/fight/CommonFightEngineSkill.cpp \
    $$PWD/fight/CommonFightEngineWild.cpp \
    $$PWD/fight/CommonFightEngineBase.cpp \
    $$PWD/base/Version.cpp

HEADERS  += $$PWD/base/GeneralStructures.h \
    $$PWD/base/ClientBase.h \
    $$PWD/base/ChatParsing.h \
    $$PWD/base/ProtocolParsing.h \
    $$PWD/base/ProtocolParsingCheck.h \
    $$PWD/base/MoveOnTheMap.h \
    $$PWD/base/Map_loader.h \
    $$PWD/base/CommonMap.h \
    $$PWD/base/GeneralVariable.h \
    $$PWD/base/FacilityLib.h \
    $$PWD/base/FacilityLibGeneral.h \
    $$PWD/base/DatapackGeneralLoader.h \
    $$PWD/base/CommonDatapack.h \
    $$PWD/base/CommonDatapackServerSpec.h \
    $$PWD/base/CommonSettingsCommon.h \
    $$PWD/base/CommonSettingsServer.h \
    $$PWD/base/GeneralType.h \
    $$PWD/base/cpp11addition.h \
    $$PWD/base/GeneralStructuresXml.h \
    $$PWD/base/PortableEndian.h \
    $$PWD/fight/FightLoader.h \
    $$PWD/fight/CommonFightEngine.h \
    $$PWD/fight/CommonFightEngineBase.h

#only linux is C only, mac, windows, other is in Qt for compatibility
win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
mac:INCLUDEPATH += /usr/local/include/
mac:LIBS += -L/usr/local/lib/
linux:DEFINES += __linux__
