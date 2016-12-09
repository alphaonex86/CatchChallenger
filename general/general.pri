CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT       -= core

LIBS += -lz -llzma
LIBS += -lcrypto

TEMPLATE = app

SOURCES += $$PWD/base/ChatParsing.cpp \
    $$PWD/base/QFakeSocket.cpp \
    $$PWD/base/QFakeServer.cpp \
    $$PWD/base/ProtocolParsingGeneral.cpp \
    $$PWD/base/ProtocolParsingInput.cpp \
    $$PWD/base/ProtocolParsingOutput.cpp \
    $$PWD/base/ProtocolParsingCheck.cpp \
    $$PWD/base/MoveOnTheMap.cpp \
    $$PWD/base/Map_loader.cpp \
    $$PWD/base/CommonMap.cpp \
    $$PWD/base/FacilityLib.cpp \
    $$PWD/base/FacilityLibGeneral.cpp \
    $$PWD/base/ConnectedSocket.cpp \
    $$PWD/fight/FightLoader.cpp \
    $$PWD/base/DatapackGeneralLoader.cpp \
    $$PWD/base/CommonDatapack.cpp \
    $$PWD/base/CommonDatapackServerSpec.cpp \
    $$PWD/fight/CommonFightEngine.cpp \
    $$PWD/fight/CommonFightEngineTurn.cpp \
    $$PWD/fight/CommonFightEngineBuff.cpp \
    $$PWD/fight/CommonFightEngineSkill.cpp \
    $$PWD/fight/CommonFightEngineWild.cpp \
    $$PWD/fight/CommonFightEngineBase.cpp \
    $$PWD/base/CommonSettingsCommon.cpp \
    $$PWD/base/CommonSettingsServer.cpp \
    $$PWD/base/cpp11addition.cpp \
    $$PWD/base/cpp11additionstringtointcpp.cpp \
    $$PWD/base/cpp11additionstringtointc.cpp \
    $$PWD/base/lz4/lz4.c \
    $$PWD/base/CachedString.cpp

HEADERS  += $$PWD/base/GeneralStructures.h \
    $$PWD/base/ClientBase.h \
    $$PWD/base/ChatParsing.h \
    $$PWD/base/QFakeServer.h \
    $$PWD/base/ProtocolParsing.h \
    $$PWD/base/ProtocolParsingCheck.h \
    $$PWD/base/MoveOnTheMap.h \
    $$PWD/base/Map_loader.h \
    $$PWD/base/CommonMap.h \
    $$PWD/base/GeneralVariable.h \
    $$PWD/base/QFakeSocket.h \
    $$PWD/base/FacilityLib.h \
    $$PWD/base/FacilityLibGeneral.h \
    $$PWD/base/ConnectedSocket.h \
    $$PWD/fight/FightLoader.h \
    $$PWD/base/DatapackGeneralLoader.h \
    $$PWD/base/CommonDatapack.h \
    $$PWD/base/CommonDatapackServerSpec.h \
    $$PWD/fight/CommonFightEngine.h \
    $$PWD/fight/CommonFightEngineBase.h \
    $$PWD/base/CommonSettingsCommon.h \
    $$PWD/base/CommonSettingsServer.h \
    $$PWD/base/GeneralType.h \
    $$PWD/base/cpp11addition.h \
    $$PWD/base/GeneralStructuresXml.h \
    $$PWD/base/PortableEndian.h \
    $$PWD/base/lz4/lz4.h \
    $$PWD/base/CachedString.h

#only linux is C only, mac, windows, other is in Qt for compatibility
win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
mac:INCLUDEPATH += /usr/local/include/
mac:LIBS += -L/usr/local/lib/
win32:LIBS += -lWs2_32
linux:DEFINES += __linux__
