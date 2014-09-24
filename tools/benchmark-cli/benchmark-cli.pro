include(../../general/general.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)
include(../bot/simple/Simple.pri)

QT       += core xml network sql

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

