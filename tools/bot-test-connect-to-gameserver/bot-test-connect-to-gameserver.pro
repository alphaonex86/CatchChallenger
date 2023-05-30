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
    ../bot/BotInterface.cpp \
    ../../client/libqtcatchchallenger/Api_protocol_Qt.cpp \
    ../../client/libqtcatchchallenger/ConnectedSocket.cpp \
    ../../client/libcatchchallenger/DatapackChecksum.cpp \
    ../../client/libcatchchallenger/DatapackClientLoader.cpp
HEADERS  += \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    GlobalControler.h \
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

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/tinyXML2/tinyxml2c.cpp

#linux:LIBS += -fuse-ld=mold
#precompile_header:!isEmpty(PRECOMPILED_HEADER) {
#DEFINES += USING_PCH
#
