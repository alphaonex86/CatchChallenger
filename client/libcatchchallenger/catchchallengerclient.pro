include(lib.pri)
include(../../general/general.pri)

#DEFINES += CATCHCHALLENGER_SOLO
#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
}
else
{
include(../../server/catchchallenger-server.pri)
}

QT       -= core gui
LIBS -= -lpthread

DEFINES += CATCHCHALLENGERLIB

TARGET = catchchallengerclient
TEMPLATE = lib
