CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion"
#QMAKE_CXXFLAGS+="-Wno-weak-vtables -Wno-non-virtual-dtor -Wno-gnu-statement-expression -Wno-implicit-fallthrough -Wno-float-equal -Wno-unreachable-code -Wno-missing-noreturn -Wno-unreachable-code-return -Wno-vla-extension -Wno-format-nonliteral -Wno-vla -Wno-embedded-directive -Wno-missing-variable-declarations -Wno-missing-braces"
QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
QMAKE_CFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
#QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-missing-prototypes -Wno-padded -Wno-weak-vtables -Wno-c++17-extensions -Wno-shadow-field-in-constructor -Wno-return-std-move-in-c++11"
#QMAKE_CFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-missing-prototypes -Wno-padded -Wno-weak-vtables -Wno-c++17-extensions -Wno-shadow-field-in-constructor -Wno-return-std-move-in-c++11"

QT       -= core

wasm: {
    LIBS += /mnt/data/perso/progs/qt/qt-everywhere-5.13.0-wasm/qtbase/lib/libzstd.a
    LIBS += -Lcrypto
    QMAKE_LFLAGS += -s TOTAL_MEMORY=200015872
    #QMAKE_LFLAGS += -s ASYNCIFY=1 -> buggy and obsolete
    #QMAKE_LFLAGS += -s EMTERPRETIFY_ASYNC=1 -s EMTERPRETIFY=1
    QMAKE_CLFLAGS += -g
    QMAKE_CXXFLAGS += -g
    QMAKE_LFLAGS += -s ASSERTIONS=1 -g
    CONFIG+=debug
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
mac:INCLUDEPATH += /usr/local/opt/openssl/include
mac:LIBS += -L/usr/local/opt/openssl/lib
#win32:LIBS += -lWs2_32
linux:DEFINES += __linux__
