QT       -= gui widgets network sql xml

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION

#LIBS += -lssl -lcrypto

CONFIG += c++11

TARGET = catchchallenger-server-login-master
CONFIG   += console

TEMPLATE = app

SOURCES += \
    main-epoll-login-master.cpp \
    EpollClientLoginMaster.cpp \
    EpollServerLoginMaster.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    EpollClientLoginMasterProtocolParsning.cpp \
    EpollClientLoginMasterStaticVar.cpp \
    ../epoll/EpollSslClient.cpp

HEADERS += \
    EpollClientLoginMaster.h \
    EpollServerLoginMaster.h \
    ../epoll/Epoll.h \
    ../epoll/EpollGenericSslServer.h \
    ../epoll/EpollGenericServer.h \
    ../epoll/EpollClient.h \
    ../epoll/EpollSocket.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../epoll/EpollSslClient.h
