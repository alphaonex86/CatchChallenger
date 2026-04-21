DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../libbot/simple/Simple.pri)

QMAKE_CXXFLAGS+="-fstack-protector-all -g"

QT       += core network
QT -= gui widgets opengl qml quick sql
DEFINES += BOTTESTCONNECT CATCHCHALLENGER_BOT NOWEBSOCKET

TARGET = bot-test-connect-to-gameserver
TEMPLATE = app


SOURCES += main.cpp\
    ../libbot/MultipleBotConnection.cpp \
    ../libbot/MultipleBotConnectionImplForGui.cpp \
    GlobalController.cpp \
    ../../client/libqtcatchchallenger/Api_client_real_base.cpp \
    ../../client/libqtcatchchallenger/Api_client_real_main.cpp \
    ../../client/libqtcatchchallenger/Api_client_real_sub.cpp \
    ../../client/libqtcatchchallenger/Api_client_real.cpp \
    ../../client/libqtcatchchallenger/Api_client_virtual.cpp \
    ../../client/libcatchchallenger/Api_protocol_loadchar.cpp \
    ../../client/libcatchchallenger/Api_protocol_message.cpp \
    ../../client/libcatchchallenger/Api_protocol_query.cpp \
    ../../client/libcatchchallenger/Api_protocol_reply.cpp \
    ../../client/libcatchchallenger/Api_protocol.cpp \
    ../../client/libqtcatchchallenger/QZstdDecodeThread.cpp \
    ../../client/libcatchchallenger/TarDecode.cpp \
    ../../client/libcatchchallenger/ZstdDecode.cpp \
    ../../client/libqtcatchchallenger/QtDatapackChecksum.cpp \
    ../libbot/BotInterface.cpp \
    ../../client/libqtcatchchallenger/Api_protocol_Qt.cpp \
    ../../client/libqtcatchchallenger/ConnectedSocket.cpp \
    ../../client/libcatchchallenger/DatapackChecksum.cpp \
    ../../client/libcatchchallenger/DatapackClientLoader.cpp \
    ../../client/libcatchchallenger/ClientStructures.cpp
HEADERS  += \
    ../libbot/MultipleBotConnection.h \
    ../libbot/MultipleBotConnectionImplForGui.h \
    ../libbot/BotInterface.h \
    GlobalController.h \
    ../../client/libqtcatchchallenger/Api_client_real.hpp \
    ../../client/libqtcatchchallenger/Api_client_virtual.hpp \
    ../../client/libcatchchallenger/Api_protocol.hpp \
    ../../client/libcatchchallenger/ClientVariable.hpp \
    ../../client/libqtcatchchallenger/QZstdDecodeThread.hpp \
    ../../client/libcatchchallenger/TarDecode.hpp \
    ../../client/libcatchchallenger/ZstdDecode.hpp \
    ../../client/libqtcatchchallenger/QtDatapackChecksum.hpp \
    ../../client/libqtcatchchallenger/Api_protocol_Qt.hpp \
    ../../client/libqtcatchchallenger/ConnectedSocket.hpp \
    ../../client/libcatchchallenger/DatapackChecksum.hpp \
    ../../client/libcatchchallenger/DatapackClientLoader.hpp \
    ../../client/libqtcatchchallenger/ClientVariableAudio.hpp \
    ../../client/libcatchchallenger/ClientStructures.hpp

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/tinyXML2/tinyxml2c.cpp
