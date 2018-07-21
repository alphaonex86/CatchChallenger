DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../bot/simple/Simple.pri)

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"

QT       += core network
QT -= gui widgets script opengl qml quick sql
DEFINES += BOTTESTCONNECT CATCHCHALLENGER_BOT

TARGET = bot-test-connect-to-gameserver
TEMPLATE = app


SOURCES += main.cpp\
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    GlobalControler.cpp \
    ../../client/base/Api_client_real_base.cpp \
    ../../client/base/Api_client_real_main.cpp \
    ../../client/base/Api_client_real_sub.cpp \
    ../../client/base/Api_client_real.cpp \
    ../../client/base/Api_client_virtual.cpp \
    ../../client/base/Api_protocol_loadchar.cpp \
    ../../client/base/Api_protocol_message.cpp \
    ../../client/base/Api_protocol_query.cpp \
    ../../client/base/Api_protocol_reply.cpp \
    ../../client/base/Api_protocol.cpp \
    ../../client/base/qt-tar-compressed/QTarDecode.cpp \
    ../../client/base/qt-tar-compressed/QZstdDecodeThread.cpp \
    ../../client/base/DatapackChecksum.cpp \
    ../bot/BotInterface.cpp \
    ../../client/base/FacilityLibClient.cpp \
    ../../client/base/Api_protocol_Qt.cpp
HEADERS  += \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    GlobalControler.h \
    ../../client/base/Api_client_real.h \
    ../../client/base/Api_client_virtual.h \
    ../../client/base/Api_protocol.h \
    ../../client/base/ClientVariable.h \
    ../../client/base/ClientStructures.h \
    ../../client/base/qt-tar-compressed/QTarDecode.h \
    ../../client/base/qt-tar-compressed/QZstdDecodeThread.h \
    ../../client/base/DatapackChecksum.h \
    ../../client/base/FacilityLibClient.h \
    ../../client/base/Api_protocol_Qt.h

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
