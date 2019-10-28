DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT

include(../general/general.pri)
include(catchchallenger-server.pri)
include(../general/hps/hps.pri)

QT       -= gui widgets network sql
#QT       -= core xml

#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations  -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations -std=c++0x  -fno-rtti"

QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g -fno-rtti"

#DEFINES += CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#DEFINES += SERVERSSL
#DEFINES += SERVERBENCHMARK

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

LIBS += -lssl -lcrypto

# postgresql 9+
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL
LIBS    += -lpq
# mysql 5.7+
#LIBS    += -lmysqlclient
#DEFINES += CATCHCHALLENGER_DB_MYSQL

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console

TEMPLATE = app

SOURCES += \
    $$PWD/epoll/EpollSocket.cpp \
    $$PWD/epoll/EpollClient.cpp \
    $$PWD/epoll/EpollServer.cpp \
    $$PWD/epoll/EpollSslClient.cpp \
    $$PWD/epoll/EpollSslServer.cpp \
    $$PWD/epoll/Epoll.cpp \
    $$PWD/epoll/EpollTimer.cpp \
    $$PWD/epoll/db/EpollPostgresql.cpp \
    $$PWD/epoll/db/EpollMySQL.cpp \
    $$PWD/epoll/timer/TimerCityCapture.cpp \
    $$PWD/epoll/timer/TimerSendInsertMoveRemove.cpp \
    $$PWD/epoll/timer/TimerPositionSync.cpp \
    $$PWD/epoll/timer/TimerDdos.cpp \
    $$PWD/epoll/timer/TimerEvents.cpp \
    $$PWD/epoll/EpollGenericServer.cpp \
    $$PWD/epoll/EpollGenericSslServer.cpp \
    $$PWD/all-in-one/TimeRangeEvent.cpp \
    $$PWD/NormalServerGlobal.cpp \
    $$PWD/main-epoll.cpp \
    $$PWD/main-epoll2.cpp

HEADERS += $$PWD/epoll/EpollSocket.h \
    $$PWD/epoll/EpollClient.h \
    $$PWD/epoll/EpollServer.h \
    $$PWD/epoll/EpollSslClient.h \
    $$PWD/epoll/EpollSslServer.h \
    $$PWD/epoll/Epoll.h \
    $$PWD/epoll/BaseClassSwitch.h \
    $$PWD/epoll/EpollTimer.h \
    $$PWD/epoll/db/EpollPostgresql.h \
    $$PWD/epoll/db/EpollMySQL.h \
    $$PWD/epoll/timer/TimerCityCapture.h \
    $$PWD/epoll/timer/TimerPositionSync.h \
    $$PWD/epoll/timer/TimerSendInsertMoveRemove.h \
    $$PWD/epoll/timer/TimerDdos.h \
    $$PWD/epoll/timer/TimerEvents.h \
    $$PWD/epoll/EpollGenericServer.h \
    $$PWD/epoll/EpollGenericSslServer.h \
    $$PWD/NormalServerGlobal.h \
    $$PWD/base/DdosBuffer.h \
    $$PWD/all-in-one/TimeRangeEvent.h

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../general/base/tinyXML2/tinyxml2c.cpp
