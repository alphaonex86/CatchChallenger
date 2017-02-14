include(../../general/general.pri)
include(../../client/base/client.pri)
include(../../client/base/multi.pri)
include(../bot/actions/Actions.pri)

QT       += core gui xml network sql
QT += widgets

TARGET = bot-actions
TEMPLATE = app

DEFINES += BOTACTIONS

SOURCES += main.cpp\
        MainWindow.cpp \
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    BotTargetList.cpp \
    BotTargetListGUI.cpp \
    BotTargetListNear.cpp \
    MapBrowse.cpp \
    WaitScreen.cpp \
    BotTargetListPlayerInfo.cpp \
    BotTargetListUpdateStep.cpp \
    BotTargetListNextPos.cpp \
    SocialChat.cpp
HEADERS  += MainWindow.h \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    BotTargetList.h \
    MapBrowse.h \
    WaitScreen.h \
    SocialChat.h

FORMS    += MainWindow.ui \
    BotTargetList.ui \
    MapBrowse.ui \
    WaitScreen.ui \
    SocialChat.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

RESOURCES += \
    localr.qrc

QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra -fno-omit-frame-pointer -fsanitize=address"
QMAKE_CFLAGS+="-Wall -Wextra -fno-omit-frame-pointer -fsanitize=address"
QMAKE_LFLAGS+="-fno-omit-frame-pointer -Wl,--no-undefined -fsanitize=address -stdlib=libc++"
#/usr/lib64/qt5/bin/qmake -spec linux-clang
