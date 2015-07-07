#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math"

QT       -= gui widgets network sql xml

DEFINES += SERVERNOBUFFER
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += CATCHCHALLENGER_CLASS_LOGIN

#LIBS += -lssl -lcrypto
LIBS    += -lpq -llzma

CONFIG += c++11

TARGET = catchchallenger-server-login
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
    LinkToMaster.cpp \
    LinkToMasterStaticVar.cpp \
    LinkToMasterProtocolParsing.cpp \
    EpollClientLoginSlaveProtocolParsing.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp \
    EpollClientLoginSlaveWrite.cpp \
    CharactersGroupForLogin.cpp \
    CharactersGroupClient.cpp \
    ../base/BaseServerLogin.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../base/SqlFunction.cpp \
    ../base/DictionaryLogin.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    LinkToGameServerStaticVar.cpp \
    LinkToGameServerProtocolParsing.cpp \
    LinkToGameServer.cpp

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
    LinkToMaster.h \
    ../epoll/EpollClientToServer.h \
    ../epoll/EpollSslClientToServer.h \
    CharactersGroupForLogin.h \
    ../base/BaseServerLogin.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/FacilityLibGeneral.h \
    ../base/SqlFunction.h \
    ../base/DictionaryLogin.h \
    ../VariableServer.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/CommonSettingsCommon.h \
    LinkToGameServer.h
