include(../base/client.pri)
include(../../server/catchchallenger-server.pri)
include(../../general/general.pri)

QT       += sql

VPATH += ../../server/

TARGET = catchchallenger-single-player

SOURCES += main.cpp\
        mainwindow.cpp \
        SaveGameLabel.cpp \
        InternalServer.cpp \
        NewGame.cpp

HEADERS  += mainwindow.h \
        InternalServer.h \
        NewGame.h \
        SaveGameLabel.h

FORMS    += mainwindow.ui \
    NewGame.ui

RESOURCES += \
    resources/resources-single-player.qrc \
    ../../server/databases/resources-db-sqlite.qrc

win32:RESOURCES += ../../server/base/resources/resources-windows-qt-plugin.qrc
