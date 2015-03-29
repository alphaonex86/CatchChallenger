QT       -= gui widgets network sql
QT       += xml

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION

#LIBS += -lssl -lcrypto
LIBS    += -lpq

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
    EpollClientLoginMasterStaticVar.cpp \
    ../epoll/EpollSslClient.cpp \
    ../base/DatabaseBase.cpp \
    ../epoll/db/EpollPostgresql.cpp \
    EpollClientLoginMasterProtocolParsing.cpp \
    CharactersGroup.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/CommonDatapack.cpp \
    ../base/BaseServerCommon.cpp \
    ../../general/base/DatapackGeneralLoader.cpp \
    ../../general/fight/CommonFightEngine.cpp

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
    ../epoll/EpollSslClient.h \
    ../base/DatabaseBase.h \
    ../epoll/db/EpollPostgresql.h \
    CharactersGroup.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/CommonDatapack.h \
    ../base/BaseServerCommon.h \
    ../VariableServer.h \
    ../../general/base/DatapackGeneralLoader.h \
    ../../general/fight/CommonFightEngine.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/GeneralType.h
