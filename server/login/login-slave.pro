QT       -= gui widgets network sql xml

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION

#LIBS += -lssl -lcrypto
LIBS    += -lpq

CONFIG += c++11

TARGET = catchchallenger-server-login-slave
CONFIG   += console

TEMPLATE = app

SOURCES += \
    main-epoll-login-slave.cpp \
    EpollClientLoginSlave.cpp \
    EpollServerLoginSlave.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    EpollClientLoginSlaveStaticVar.cpp \
    ../epoll/EpollSslClient.cpp \
    EpollClientLoginSlaveHeavyLoad.cpp \
    ../epoll/db/EpollPostgresql.cpp \
    ../base/DatabaseBase.cpp \
    ../base/PreparedDBQuery.cpp \
    LoginLinkToMaster.cpp \
    LoginLinkToMasterStaticVar.cpp \
    LoginLinkToMasterProtocolParsing.cpp \
    EpollClientLoginSlaveProtocolParsing.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp

HEADERS += \
    EpollClientLoginSlave.h \
    EpollServerLoginSlave.h \
    ../epoll/Epoll.h \
    ../epoll/EpollGenericSslServer.h \
    ../epoll/EpollGenericServer.h \
    ../epoll/EpollClient.h \
    ../epoll/EpollSocket.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../epoll/EpollSslClient.h \
    ../epoll/db/EpollPostgresql.h \
    ../base/DatabaseBase.h \
    ../base/PreparedDBQuery.h \
    LoginLinkToMaster.h \
    ../epoll/EpollClientToServer.h \
    ../epoll/EpollSslClientToServer.h
