include(../epoll/catchchallenger-server-cli-epoll.pro)
TEMPLATE = app
TARGET = catchchallenger-game-server-alone

DEFINES -= CATCHCHALLENGER_CLASS_ALLINONESERVER
DEFINES += CATCHCHALLENGER_CLASS_ONLYGAMESERVER
DEFINES -= CATCHCHALLENGERSERVERDROPIFCLENT
DEFINES += CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
DEFINES += EPOLLCATCHCHALLENGERSERVER

HEADERS += \
    $$PWD/LinkToMaster.h \
    $$PWD/timer/PlayerUpdaterToMaster.h \
    $$PWD/../epoll/timer/TimerPurgeTokenAuthList.h

SOURCES += \
    $$PWD/LinkToMaster.cpp \
    $$PWD/LinkToMasterProtocolParsing.cpp \
    $$PWD/LinkToMasterStaticVar.cpp \
    $$PWD/timer/PlayerUpdaterToMaster.cpp \
    $$PWD/../epoll/timer/TimerPurgeTokenAuthList.cpp
