include(catchchallenger-server-cli-epoll.pro)

TARGET = catchchallenger-game-server-alone

DEFINES -= CATCHCHALLENGER_CLASS_ALLINONESERVER
DEFINES += CATCHCHALLENGER_CLASS_ONLYGAMESERVER
DEFINES -= CATCHCHALLENGERSERVERDROPIFCLENT

HEADERS += \
    game-server-alone/LoginLinkToMaster.h

SOURCES += \
    game-server-alone/LoginLinkToMaster.cpp \
    game-server-alone/LoginLinkToMasterProtocolParsing.cpp \
    game-server-alone/LoginLinkToMasterStaticVar.cpp
