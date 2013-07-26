include(../base/client.pri)
include(../../server/catchchallenger-server.pri)
include(../../general/general.pri)

QT       += sql

TARGET = catchchallenger-single-player

SOURCES += main.cpp\
        mainwindow.cpp \
        SaveGameLabel.cpp \
        InternalServer.cpp \
        NewGame.cpp \
    NewProfile.cpp

HEADERS  += mainwindow.h \
        InternalServer.h \
        NewGame.h \
        SaveGameLabel.h \
    NewProfile.h

FORMS    += mainwindow.ui \
    NewGame.ui \
    NewProfile.ui

RESOURCES += \
    resources/resources-single-player.qrc \
    ../../server/databases/resources-db-sqlite.qrc

win32:RESOURCES += ../../server/base/resources/resources-windows-qt-plugin.qrc

DEFINES += NOREMOTE
