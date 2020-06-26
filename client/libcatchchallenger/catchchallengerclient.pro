include(lib.pri)
include(../../general/general.pri)

QT       -= core gui
LIBS -= -lpthread

DEFINES += CATCHCHALLENGERLIB

TARGET = catchchallengerclient
TEMPLATE = lib
