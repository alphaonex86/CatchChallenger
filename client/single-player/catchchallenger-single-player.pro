include(../base/client.pri)
include(../../server/catchchallenger-server.pri)
include(../../general/general.pri)

QT       += sql

TARGET = catchchallenger-single-player

SOURCES += main.cpp\
        mainwindow.cpp \
        InternalServer.cpp \
        NewGame.cpp \
    NewProfile.cpp

HEADERS  += mainwindow.h \
        InternalServer.h \
        NewGame.h \
    NewProfile.h

FORMS    += mainwindow.ui \
    NewGame.ui \
    NewProfile.ui

RESOURCES += \
    ../base/resources/resources-single-player.qrc \
    ../../server/databases/resources-db-sqlite.qrc

DEFINES += NOREMOTE

