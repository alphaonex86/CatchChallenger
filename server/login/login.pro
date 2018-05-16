#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -std=c++0x -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -std=c++0x -fno-rtti"

QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g -fno-rtti"

QT       -= gui widgets network sql xml
QT       -= xml core

DEFINES += TIXML_USE_STL
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += CATCHCHALLENGER_CLASS_LOGIN

# postgresql 9+
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL
LIBS    += -lpq
# mysql 5.5+
#LIBS    += -lmysqlclient
#DEFINES += CATCHCHALLENGER_DB_MYSQL

LIBS += -lssl -lcrypto
LIBS    += -lzstd

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
    ../base/PreparedStatementUnit.cpp \
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
    LinkToGameServer.cpp \
    TimerDdos.cpp \
    ../epoll/EpollTimer.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../base/TinyXMLSettings.cpp \
    ../base/DatabaseFunction.cpp \
    ../base/StringWithReplacement.cpp

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
    ../base/PreparedStatementUnit.h \
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
    LinkToGameServer.h \
    TimerDdos.h \
    ../epoll/EpollTimer.h \
    ../../general/base/cpp11addition.h \
    ../base/TinyXMLSettings.h \
    ../base/DatabaseFunction.h \
    ../base/StringWithReplacement.h \
    VariableLoginServer.h \
    ../epoll/BaseClassSwitch.h

DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp
