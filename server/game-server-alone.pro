include(catchchallenger-server-cli-epoll.pro)

TARGET = catchchallenger-game-server-alone

DEFINES -= CATCHCHALLENGER_CLASS_ALLINONESERVER
DEFINES += CATCHCHALLENGER_CLASS_ONLYGAMESERVER
DEFINES -= CATCHCHALLENGERSERVERDROPIFCLENT

HEADERS += \
    $$PWD/game-server-alone/LinkToMaster.h \
    $$PWD/base/PlayerUpdaterToMaster.h \
    $$PWD/epoll/timer/TimerPurgeTokenAuthList.h

SOURCES += \
    $$PWD/game-server-alone/LinkToMaster.cpp \
    $$PWD/game-server-alone/LinkToMasterProtocolParsing.cpp \
    $$PWD/game-server-alone/LinkToMasterStaticVar.cpp \
    $$PWD/base/PlayerUpdaterToMaster.cpp \
    $$PWD/epoll/timer/TimerPurgeTokenAuthList.cpp
