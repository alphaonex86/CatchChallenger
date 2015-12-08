#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math"

QT       -= gui widgets sql xml network core

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += CATCHCHALLENGER_CLASS_GATEWAY

#LIBS += -lssl -lcrypto
LIBS    += -llzma
LIBS += -lcurl

CONFIG += c++11

TARGET = catchchallenger-gateway
CONFIG   += console

TEMPLATE = app

SOURCES += \
    main-epoll-login-slave.cpp \
    EpollClientLoginSlave.cpp \
    EpollServerLoginSlave.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    EpollClientLoginSlaveStaticVar.cpp \
    ../epoll/EpollSslClient.cpp \
    EpollClientLoginSlaveProtocolParsing.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp \
    EpollClientLoginSlaveWrite.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    LinkToGameServerStaticVar.cpp \
    LinkToGameServerProtocolParsing.cpp \
    LinkToGameServer.cpp \
    TimerDdos.cpp \
    ../epoll/EpollTimer.cpp \
    DatapackDownloaderBase.cpp \
    DatapackDownloader_sub.cpp \
    DatapackDownloader_main.cpp \
    DatapackDownloaderMainSub.cpp \
    ../../client/base/DatapackChecksum.cpp \
    ../../client/base/qt-tar-xz/QTarDecode.cpp \
    ../../client/base/qt-tar-xz/QXzDecode.cpp \
    ../../client/base/qt-tar-xz/QXzDecodeThread.cpp \
    ../../client/base/qt-tar-xz/xz_crc32.c \
    ../../client/base/qt-tar-xz/xz_dec_bcj.c \
    ../../client/base/qt-tar-xz/xz_dec_lzma2.c \
    ../../client/base/qt-tar-xz/xz_dec_stream.c \
    ../../general/base/CommonSettingsCommon.cpp \
    ../../general/base/CommonSettingsServer.cpp \
    EpollClientLoginSlaveDatapack.cpp \
    ../../general/base/lz4/lz4.c \
    ../../general/base/cpp11addition.cpp

HEADERS += \
    EpollClientLoginSlave.h \
    EpollServerLoginSlave.h \
    ../epoll/Epoll.h \
    ../epoll/EpollGenericSslServer.h \
    ../epoll/EpollGenericServer.h \
    ../epoll/EpollClient.h \
    ../epoll/EpollSocket.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../epoll/EpollSslClient.h \
    ../epoll/EpollClientToServer.h \
    ../epoll/EpollSslClientToServer.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/FacilityLibGeneral.h \
    ../VariableServer.h \
    ../../general/base/GeneralVariable.h \
    LinkToGameServer.h \
    TimerDdos.h \
    ../epoll/EpollTimer.h \
    DatapackDownloaderBase.h \
    DatapackDownloaderMainSub.h \
    ../../client/base/DatapackChecksum.h \
    ../../client/base/qt-tar-xz/QTarDecode.h \
    ../../client/base/qt-tar-xz/xz.h \
    ../../client/base/qt-tar-xz/xz_config.h \
    ../../client/base/qt-tar-xz/QXzDecode.h \
    ../../client/base/qt-tar-xz/QXzDecodeThread.h \
    ../../client/base/qt-tar-xz/xz_lzma2.h \
    ../../client/base/qt-tar-xz/xz_private.h \
    ../../client/base/qt-tar-xz/xz_stream.h \
    ../../general/base/CommonSettingsCommon.h \
    ../../general/base/CommonSettingsServer.h \
    ../../general/base/lz4/lz4.h
