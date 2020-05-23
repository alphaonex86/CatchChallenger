DEFINES += CATCHCHALLENGER_NOAUDIO

LIBS += -lzstd
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
    ../../client/qt/Api_client_real.hpp \
    ../../client/qt/Api_client_virtual.hpp \
    ../../client/libcatchchallenger/Api_protocol.hpp \
    ../../client/qt/ClientVariable.hpp \
    ../../client/qt/QZstdDecodeThread.hpp \
    ../../client/tarcompressed/TarDecode.hpp \
    ../../client/tarcompressed/ZstdDecode.hpp \
    ../../client/qt/QtDatapackChecksum.hpp \
    ../../client/qt/FacilityLibClient.hpp \
    ../../client/qt/Api_protocol_Qt.hpp \
    ../../client/qt/ConnectedSocket.hpp \
    ../../client/libcatchchallenger/DatapackChecksum.hpp \
    ../../client/libcatchchallenger/DatapackClientLoader.hpp \
    ../../client/qt/ClientVariableAudio.hpp \
    ../../client/libcatchchallenger/ClientStructures.hpp \
    ../../client/qt/QFakeServer.hpp \
    ../../client/qt/QFakeSocket.hpp

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
