include(lib.pri)
include(../../general/general.pri)

QT       -= core gui
LIBS -= -lcrypto -lpthread

DEFINES += CATCHCHALLENGERLIB

TARGET = catchchallengerclient
TEMPLATE = lib
