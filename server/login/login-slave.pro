QT       -= gui widgets network sql

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT

#LIBS += -lssl -lcrypto
LIBS    += -lpq

CONFIG += c++11

TARGET = catchchallenger-server-login-slave
CONFIG   += console

TEMPLATE = app
