include(../../general/general.pri)

SOURCES += $$PWD/../../client/base/Api_client_virtual.cpp \
    $$PWD/../../client/base/DatapackChecksum.cpp \
    $$PWD/../../client/base/Api_protocol.cpp \
    $$PWD/../../client/base/Bot/FakeBot.cpp \
    $$PWD/../../client/fight/interface/ClientFightEngine.cpp \
    $$PWD/../../client/base/ExtraSocket.cpp \
    $$PWD/../../client/base/LocalListener.cpp \
    $$PWD/../../client/base/Api_client_real.cpp \
    $$PWD/../../client/base/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/../../client/base/qt-tar-xz/QXzDecode.cpp \
    $$PWD/../../client/base/qt-tar-xz/QTarDecode.cpp \
    $$PWD/../../client/base/qt-tar-xz/xz_crc32.c \
    $$PWD/../../client/base/qt-tar-xz/xz_dec_stream.c \
    $$PWD/../../client/base/qt-tar-xz/xz_dec_lzma2.c \
    $$PWD/../../client/base/qt-tar-xz/xz_dec_bcj.c

HEADERS += $$PWD/../../client/base/Api_client_virtual.h \
    $$PWD/../../client/base/DatapackChecksum.h \
    $$PWD/../../client/base/Api_protocol.h \
    $$PWD/../../client/base/Bot/FakeBot.h \
    $$PWD/../../client/fight/interface/ClientFightEngine.h \
    $$PWD/../../client/base/ExtraSocket.h \
    $$PWD/../../client/base/LocalListener.h \
    $$PWD/../../client/base/Api_client_real.h \
    $$PWD/../../client/base/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/../../client/base/qt-tar-xz/QXzDecode.h \
    $$PWD/../../client/base/qt-tar-xz/QTarDecode.h

DEFINES += CATCHCHALLENGER_MULTI

include(../bot/simple/Simple.pri)

QT       += core xml network sql
QT       -= script opengl qml quick gui

TARGET = benchmark-cli
TEMPLATE = app

DEFINES += BENCHMARKMUTIPLECLIENT

SOURCES += main.cpp\
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplFoprGui.cpp \
    MainBenchmark.cpp

HEADERS  += \
    ../bot/BotInterface.h \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplFoprGui.h \
    MainBenchmark.h

