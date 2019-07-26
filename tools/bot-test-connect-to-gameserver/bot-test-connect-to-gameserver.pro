DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../bot/simple/Simple.pri)

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"

QT       += core network
QT -= gui widgets script opengl qml quick sql
DEFINES += BOTTESTCONNECT CATCHCHALLENGER_BOT NOWEBSOCKET

TARGET = bot-test-connect-to-gameserver
TEMPLATE = app


SOURCES += main.cpp\
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    GlobalControler.cpp \
    ../../client/qt/Api_client_real_base.cpp \
    ../../client/qt/Api_client_real_main.cpp \
    ../../client/qt/Api_client_real_sub.cpp \
    ../../client/qt/Api_client_real.cpp \
    ../../client/qt/Api_client_virtual.cpp \
    ../../client/libcatchchallenger/Api_protocol_loadchar.cpp \
    ../../client/libcatchchallenger/Api_protocol_message.cpp \
    ../../client/libcatchchallenger/Api_protocol_query.cpp \
    ../../client/libcatchchallenger/Api_protocol_reply.cpp \
    ../../client/libcatchchallenger/Api_protocol.cpp \
    ../../client/qt/QZstdDecodeThread.cpp \
    ../../client/tarcompressed/TarDecode.cpp \
    ../../client/tarcompressed/ZstdDecode.cpp \
    ../../client/qt/QtDatapackChecksum.cpp \
    ../bot/BotInterface.cpp \
    ../../client/qt/FacilityLibClient.cpp \
    ../../client/qt/Api_protocol_Qt.cpp \
    ../../client/qt/ConnectedSocket.cpp \
    ../../client/libcatchchallenger/DatapackChecksum.cpp \
    ../../client/libcatchchallenger/DatapackClientLoader.cpp \
    ../../client/qt/QFakeServer.cpp \
    ../../client/qt/QFakeSocket.cpp
HEADERS  += \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    GlobalControler.h \
    ../../client/qt/Api_client_real.h \
    ../../client/qt/Api_client_virtual.h \
    ../../client/libcatchchallenger/Api_protocol.h \
    ../../client/qt/ClientVariable.h \
    ../../client/qt/QZstdDecodeThread.h \
    ../../client/tarcompressed/TarDecode.h \
    ../../client/tarcompressed/ZstdDecode.h \
    ../../client/qt/QtDatapackChecksum.h \
    ../../client/qt/FacilityLibClient.h \
    ../../client/qt/Api_protocol_Qt.h \
    ../../client/qt/ConnectedSocket.h \
    ../../client/libcatchchallenger/DatapackChecksum.h \
    ../../client/libcatchchallenger/DatapackClientLoader.h \
    ../../client/qt/ClientVariableAudio.h \
    ../../client/libcatchchallenger/ClientStructures.h \
    ../../client/qt/QFakeServer.h \
    ../../client/qt/QFakeSocket.h

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
