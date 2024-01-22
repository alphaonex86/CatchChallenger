CONFIG += c++17
QMAKE_CXXFLAGS+="-std=c++0x"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT       -= core

wasm: {
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

SOURCES += $$PWD/base/GeneralStructures.cpp \
    $$PWD/base/ProtocolParsingGeneral.cpp \
    $$PWD/base/CompressionProtocol.cpp \
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
    $$PWD/base/Version.cpp \
    $$PWD/sha224/sha224.cpp

HEADERS  += $$PWD/base/GeneralStructures.hpp \
    $$PWD/base/ClientBase.hpp \
    $$PWD/base/CompressionProtocol.hpp \
    $$PWD/base/ProtocolParsing.hpp \
    $$PWD/base/ProtocolParsingCheck.hpp \
    $$PWD/base/MoveOnTheMap.hpp \
    $$PWD/base/Map_loader.hpp \
    $$PWD/base/CommonMap.hpp \
    $$PWD/base/GeneralVariable.hpp \
    $$PWD/base/FacilityLib.hpp \
    $$PWD/base/FacilityLibGeneral.hpp \
    $$PWD/base/DatapackGeneralLoader.hpp \
    $$PWD/base/CommonDatapack.hpp \
    $$PWD/base/CommonDatapackServerSpec.hpp \
    $$PWD/base/CommonSettingsCommon.hpp \
    $$PWD/base/CommonSettingsServer.hpp \
    $$PWD/base/GeneralType.hpp \
    $$PWD/base/cpp11addition.hpp \
    $$PWD/base/GeneralStructuresXml.hpp \
    $$PWD/base/PortableEndian.hpp \
    $$PWD/fight/FightLoader.hpp \
    $$PWD/fight/CommonFightEngine.hpp \
    $$PWD/fight/CommonFightEngineBase.hpp \
    $$PWD/sha224/sha224.hpp

LIBS    += -lzstd -lxxhash

#only linux is C only, mac, windows, other is in Qt for compatibility
win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
linux:DEFINES += __linux__
win32: {
#to debug for now
#CONFIG+=debug console
}
