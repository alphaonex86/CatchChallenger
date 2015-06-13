include(catchchallenger-server-cli-epoll.pro)

TARGET = catchchallenger-game-server-alone

DEFINES -= CATCHCHALLENGER_CLASS_ALLINONESERVER
DEFINES += CATCHCHALLENGER_CLASS_ONLYGAMESERVER
DEFINES -= CATCHCHALLENGERSERVERDROPIFCLENT

HEADERS += \
    $$PWD/game-server-alone/LoginLinkToMaster.h \
    $$PWD/base/PlayerUpdaterToMaster.h

SOURCES += \
    $$PWD/game-server-alone/LoginLinkToMaster.cpp \
    $$PWD/game-server-alone/LoginLinkToMasterProtocolParsing.cpp \
    $$PWD/game-server-alone/LoginLinkToMasterStaticVar.cpp \
    $$PWD/base/PlayerUpdaterToMaster.cpp
