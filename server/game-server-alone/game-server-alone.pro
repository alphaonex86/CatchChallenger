include(../catchchallenger-server-cli-epoll.pro)

TARGET = catchchallenger-game-server-alone

DEFINES -= CATCHCHALLENGER_CLASS_ALLINONESERVER
DEFINES += CATCHCHALLENGER_CLASS_ONLYGAMESERVER
DEFINES -= CATCHCHALLENGERSERVERDROPIFCLENT
DEFINES += CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR

HEADERS += \
    $$PWD/LinkToMaster.h \
    $$PWD/../base/PlayerUpdaterToMaster.h \
    $$PWD/../epoll/timer/TimerPurgeTokenAuthList.h

SOURCES += \
    $$PWD/LinkToMaster.cpp \
    $$PWD/LinkToMasterProtocolParsing.cpp \
    $$PWD/LinkToMasterStaticVar.cpp \
    $$PWD/../base/PlayerUpdaterToMaster.cpp \
    $$PWD/../epoll/timer/TimerPurgeTokenAuthList.cpp
