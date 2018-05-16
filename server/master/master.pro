#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations -std=c++0x -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -faggressive-loop-optimizations -funsafe-loop-optimizations -std=c++0x -fno-rtti"

QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g -fno-rtti"

QT       -= gui widgets network sql
QT       -= xml core

#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
DEFINES += CATCHCHALLENGER_CLASS_MASTER

LIBS += -lssl -lcrypto

# postgresql 9+
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL
LIBS    += -lpq
# mysql 5.5+
#LIBS    += -lmysqlclient
#DEFINES += CATCHCHALLENGER_DB_MYSQL

CONFIG += c++11

TARGET = catchchallenger-server-master
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
    ../../general/base/DatapackGeneralLoader.cpp \
    ../../general/fight/CommonFightEngineBase.cpp \
    ../base/BaseServerMasterLoadDictionary.cpp \
    ../base/BaseServerMasterSendDatapack.cpp \
    ../../general/fight/FightLoader.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    PlayerUpdaterToLogin.cpp \
    ../epoll/EpollTimer.cpp \
    PurgeTheLockedAccount.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../base/TinyXMLSettings.cpp \
    CheckTimeoutGameServer.cpp \
    AutomaticPingSend.cpp \
    ../base/DatabaseFunction.cpp \
    TimeRangeEvent.cpp

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
    ../VariableServer.h \
    ../../general/base/DatapackGeneralLoader.h \
    ../../general/fight/CommonFightEngineBase.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/GeneralType.h \
    ../base/BaseServerMasterLoadDictionary.h \
    ../base/BaseServerMasterSendDatapack.h \
    ../../general/fight/FightLoader.h \
    ../../general/base/CommonSettingsCommon.h \
    PlayerUpdaterToLogin.h \
    ../epoll/EpollTimer.h \
    PurgeTheLockedAccount.h \
    ../../general/base/cpp11addition.h \
    ../base/TinyXMLSettings.h \
    ../../general/base/GeneralVariable.h \
    CheckTimeoutGameServer.h \
    AutomaticPingSend.h \
    ../base/DatabaseFunction.h \
    TimeRangeEvent.h \
    ../epoll/BaseClassSwitch.h

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp
