CONFIG += c++11

QT       -= core
DEFINES += TIXML_USE_STL

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
    $$PWD/fight/CommonFightEngineBase.cpp \
    $$PWD/base/CommonSettingsCommon.cpp \
    $$PWD/base/CommonSettingsServer.cpp \
    $$PWD/base/cpp11addition.cpp

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
    $$PWD/base/PortableEndian.h

HEADERS += \
    $$PWD/base/lz4/lz4.h \
    $$PWD/base/tinyXML/tinystr.h \
    $$PWD/base/tinyXML/tinyxml.h

SOURCES += \
    $$PWD/base/lz4/lz4.c \
    $$PWD/base/tinyXML/tinystr.cpp \
    $$PWD/base/tinyXML/tinyxml.cpp \
    $$PWD/base/tinyXML/tinyxmlerror.cpp \
    $$PWD/base/tinyXML/tinyxmlparser.cpp

win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
mac:INCLUDEPATH += /usr/local/include/
mac:LIBS += -L/usr/local/lib/
win32:LIBS += -lWs2_32
