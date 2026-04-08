DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)
include(../../client/libcatchchallenger/lib.pri)
include(../../client/libcatchchallenger/libheader.pri)
include(../../client/libqtcatchchallenger/libqt.pri)
include(../../client/libqtcatchchallenger/libqtheader.pri)
include(../../client/libqtcatchchallenger/maprender/render.pri)
include(../../client/libqtcatchchallenger/maprender/renderheader.pri)
include(../bot/simple/Simple.pri)

# maprender headers use ../libqtcatchchallenger/ and ../libcatchchallenger/ relative includes
# that need a base directory at the same level under client/
INCLUDEPATH += ../../client/libcatchchallenger

QT       += core gui xml network sql opengl websockets
QT += widgets

TARGET = bot-connection
TEMPLATE = app

DEFINES += CATCHCHALLENGER_BOT

SOURCES += main.cpp\
        MainWindow.cpp \
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    ../bot/BotInterface.cpp
HEADERS  += MainWindow.h \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h

FORMS    += MainWindow.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
