include(catchchallenger-server.pri)
include(../general/general.pri)

QT       -= gui widgets network sql
#QT       -= core xml

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations"

QMAKE_CFLAGS="-fstack-protector-all"
QMAKE_CXXFLAGS="-fstack-protector-all"

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL
#DEFINES += SERVERBENCHMARK

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

#LIBS += -lssl -lcrypto
# postgresql 9+
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL
LIBS    += -lpq
# mysql 5.5+
#LIBS    += -lmysqlclient
#DEFINES += CATCHCHALLENGER_DB_MYSQL

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console

TEMPLATE = app

SOURCES += main-epoll.cpp \
    epoll/EpollSocket.cpp \
    epoll/EpollClient.cpp \
    epoll/EpollServer.cpp \
    epoll/EpollUnixSocketServer.cpp \
    epoll/EpollSslClient.cpp \
    epoll/EpollSslServer.cpp \
    epoll/EpollUnixSocketClient.cpp \
    epoll/EpollUnixSocketClientFinal.cpp \
    epoll/Epoll.cpp \
    epoll/EpollTimer.cpp \
    epoll/db/EpollPostgresql.cpp \
    epoll/db/EpollMySQL.cpp \
    epoll/timer/TimerDisplayEventBySeconds.cpp \
    epoll/timer/TimerCityCapture.cpp \
    epoll/timer/TimerSendInsertMoveRemove.cpp \
    epoll/timer/TimerPositionSync.cpp \
    epoll/timer/TimerDdos.cpp \
    epoll/timer/TimerEvents.cpp \
    NormalServerGlobal.cpp \
    epoll/EpollGenericServer.cpp \
    epoll/EpollGenericSslServer.cpp

HEADERS += epoll/EpollSocket.h \
    epoll/EpollClient.h \
    epoll/EpollServer.h \
    epoll/EpollUnixSocketServer.h \
    epoll/EpollSslClient.h \
    epoll/EpollSslServer.h \
    epoll/EpollUnixSocketClient.h \
    epoll/EpollUnixSocketClientFinal.h \
    epoll/Epoll.h \
    epoll/BaseClassSwitch.h \
    epoll/EpollTimer.h \
    epoll/db/EpollPostgresql.h \
    epoll/db/EpollMySQL.h \
    epoll/timer/TimerDisplayEventBySeconds.h \
    epoll/timer/TimerCityCapture.h \
    epoll/timer/TimerPositionSync.h \
    epoll/timer/TimerSendInsertMoveRemove.h \
    epoll/timer/TimerDdos.h \
    epoll/timer/TimerEvents.h \
    NormalServerGlobal.h \
    epoll/EpollGenericServer.h \
    epoll/EpollGenericSslServer.h

