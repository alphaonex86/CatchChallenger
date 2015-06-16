CONFIG += c++11

QT       += core gui network xml

LIBS += -lz -llzma

TEMPLATE = app

SOURCES += $$PWD/base/DebugClass.cpp \
    $$PWD/base/ChatParsing.cpp \
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
    $$PWD/fight/CommonFightEngineBase.cpp \
    $$PWD/base/CommonSettingsCommon.cpp \
    $$PWD/base/CommonSettingsServer.cpp

HEADERS  += $$PWD/base/DebugClass.h \
    $$PWD/base/GeneralStructures.h \
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
    $$PWD/base/GeneralType.h

win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
mac:INCLUDEPATH += /usr/local/include/
mac:LIBS += -L/usr/local/lib/
win32:LIBS += -lWs2_32
