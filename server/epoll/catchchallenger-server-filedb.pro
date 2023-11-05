DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT

DEFINES += EXTERNALLIBZSTD
TEMPLATE = app
include(../../general/general.pri)
include(../catchchallenger-server.pri)
include(../catchchallenger-serverheader.pri)
include(../../general/hps/hps.pri)

QT       -= gui widgets network sql
QT       -= core xml

#linux:QMAKE_LFLAGS += -fuse-ld=mold
#linux:LIBS += -fuse-ld=mold

#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations  -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations -std=c++0x  -fno-rtti"

linux:QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti -fno-exceptions"
linux:QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g -fno-rtti -fno-exceptions"
linux:QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
linux:QMAKE_CFLAGS+="-Wno-missing-braces -Wall -Wextra"

DEFINES += CATCHCHALLENGER_EXTRA_CHECK
#DEFINES += CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#DEFINES += SERVERSSL
#DEFINES += SERVERBENCHMARK

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

# store as file to reduce RAM need
DEFINES += CATCHCHALLENGER_DB_FILE

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console

TEMPLATE = app

SOURCES += \
    $$PWD/EpollSocket.cpp \
    $$PWD/EpollClient.cpp \
    $$PWD/EpollServer.cpp \
    $$PWD/EpollSslClient.cpp \
    $$PWD/EpollSslServer.cpp \
    $$PWD/Epoll.cpp \
    $$PWD/EpollTimer.cpp \
    $$PWD/timer/TimerCityCapture.cpp \
    $$PWD/timer/TimerSendInsertMoveRemove.cpp \
    $$PWD/timer/TimerPositionSync.cpp \
    $$PWD/timer/TimerDdos.cpp \
    $$PWD/timer/TimerEvents.cpp \
    $$PWD/EpollGenericServer.cpp \
    $$PWD/EpollGenericSslServer.cpp \
    $$PWD/timer/TimeRangeEvent.cpp \
    $$PWD/../base/NormalServerGlobal.cpp \
    $$PWD/main-epoll.cpp \
    $$PWD/main-epoll2.cpp \
    $$PWD/BaseServerEpoll.cpp \
    $$PWD/ClientMapManagementEpoll.cpp \
    $$PWD/ServerPrivateVariablesEpoll.cpp \
    $$PWD/timer/PlayerUpdaterEpoll.cpp \
    $$PWD/timer/TimeRangeEventScan.cpp

HEADERS += $$PWD/EpollSocket.h \
    $$PWD/EpollClient.h \
    $$PWD/EpollServer.h \
    $$PWD/EpollSslClient.h \
    $$PWD/EpollSslServer.h \
    $$PWD/Epoll.h \
    $$PWD/BaseClassSwitch.h \
    $$PWD/EpollTimer.h \
    $$PWD/timer/TimerCityCapture.h \
    $$PWD/timer/TimerPositionSync.h \
    $$PWD/timer/TimerSendInsertMoveRemove.h \
    $$PWD/timer/TimerDdos.h \
    $$PWD/timer/TimerEvents.h \
    $$PWD/EpollGenericServer.h \
    $$PWD/EpollGenericSslServer.h \
    $$PWD/../NormalServerGlobal.h \
    $$PWD/timer/base/DdosBuffer.h \
    $$PWD/timer/TimeRangeEvent.h \
    $$PWD/base/BaseServerEpoll.hpp \
    $$PWD/ServerPrivateVariablesEpoll.hpp \
    $$PWD/timer/PlayerUpdaterEpoll.hpp \
    $$PWD/timer/TimeRangeEventScan.hpp

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/tinyXML2/tinyxml2c.cpp

SOURCES += $$PWD/../../general/base/GeneralStructures.hpp

#linux:LIBS += -fuse-ld=mold
#precompile_header:!isEmpty(PRECOMPILED_HEADER) {
#DEFINES += USING_PCH
#
