include(../base/client.pri)
include(../../server/catchchallenger-server.pri)
include(../../general/general.pri)

QT       += sql

TARGET = catchchallenger-single-player

SOURCES += main.cpp \
    ../base/solo/NewProfile.cpp \
    ../base/solo/NewGame.cpp \
    ../base/solo/InternalServer.cpp \
    SimpleSoloServer.cpp \
    ../base/solo/SoloWindow.cpp

HEADERS  += ../base/solo/NewProfile.h \
    ../base/solo/NewGame.h \
    ../base/solo/InternalServer.h \
    SimpleSoloServer.h \
    ../base/solo/SoloWindow.h

FORMS    += ../base/solo/solowindow.ui \
    ../base/solo/NewProfile.ui \
    ../base/solo/NewGame.ui \
    SimpleSoloServer.ui

RESOURCES += \
    ../base/resources/resources-single-player.qrc \
    ../../server/databases/resources-db-sqlite.qrc

DEFINES += NOREMOTE

