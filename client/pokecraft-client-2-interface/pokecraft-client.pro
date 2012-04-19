#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

include(libtiled/libtiled.pri)

QT       += core gui network opengl

TARGET = pokecraft-client
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pokecraft-client/Pokecraft_client.cpp \
    pokecraft-clients/player.cpp \
    pokecraft-clients/MultiMap.cpp \
    pokecraft-clients/craft-clients.cpp \
    pokecraft-clients/objectgroupitem.cpp \
    pokecraft-clients/objectitem.cpp \
    pokecraft-clients/spriteimage.cpp \
    pokecraft-clients/tilelayeritem.cpp \
    pokecraft-clients/gamedata.cpp \
    pokecraft-general/MoveClient.cpp \
    pokecraft-client/pokecraft-client-chat.cpp \
    pokecraft-general/DebugClass.cpp \
    pokecraft-clients/MoveClientEmiter.cpp \
    pokecraft-clients/graphicsviewkeyinput.cpp

HEADERS  += mainwindow.h \
    pokecraft-client/Pokecraft_client.h \
    pokecraft-clients/player.h \
    pokecraft-clients/graphicsviewkeyinput.h \
    pokecraft-clients/MultiMap.h \
    pokecraft-clients/craft-clients.h \
    pokecraft-clients/objectgroupitem.h \
    pokecraft-clients/objectitem.h \
    pokecraft-clients/spriteimage.h \
    pokecraft-clients/tilelayeritem.h \
    pokecraft-clients/gamedata.h \
    pokecraft-general/GeneralStructures.h \
    pokecraft-general/MoveClient.h \
    pokecraft-client/pokecraft-client-chat.h \
    pokecraft-general/DebugClass.h \
    pokecraft-clients/MoveClientEmiter.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    Pokecraft_client

win32:RC_FILE += resources-windows.rc

