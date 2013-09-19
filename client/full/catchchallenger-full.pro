include(../base/client.pri)
include(../../server/catchchallenger-server.pri)
include(../../general/general.pri)

TARGET = catchchallenger-full

QT       += sql

SOURCES += main.cpp\
        mainwindow.cpp \
    AddServer.cpp \
    ../base/solo/NewProfile.cpp \
    ../base/solo/NewGame.cpp \
    ../base/solo/InternalServer.cpp \
    ../base/solo/Solowindow.cpp

HEADERS  += mainwindow.h \
    AddServer.h \
    ../base/solo/NewProfile.h \
    ../base/solo/NewGame.h \
    ../base/solo/InternalServer.h \
    ../base/solo/Solowindow.h

FORMS    += mainwindow.ui \
    AddServer.ui \
    ../base/solo/solowindow.ui \
    ../base/solo/NewProfile.ui \
    ../base/solo/NewGame.ui

RESOURCES += ../base/resources/client-resources-multi.qrc \
    ../base/resources/resources-single-player.qrc \
    ../../server/databases/resources-db-sqlite.qrc

