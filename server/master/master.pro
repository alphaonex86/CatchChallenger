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
    EpollClientLoginMasterStaticVar.cpp \
    EpollClientLoginMasterProtocolParsing.cpp \
    CharactersGroup.cpp \
    PlayerUpdaterToLogin.cpp \
    PurgeTheLockedAccount.cpp \
    CheckTimeoutGameServer.cpp \
    AutomaticPingSend.cpp \
    TimeRangeEvent.cpp \
    ../epoll/EpollTimer.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../epoll/EpollSslClient.cpp \
    ../epoll/db/EpollPostgresql.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/CommonDatapack.cpp \
    ../../general/base/DatapackGeneralLoader.cpp \
    ../../general/base/DatapackGeneralLoaderCrafting.cpp \
    ../../general/base/DatapackGeneralLoaderIndustry.cpp \
    ../../general/base/DatapackGeneralLoaderItem.cpp \
    ../../general/base/DatapackGeneralLoaderMap.cpp \
    ../../general/base/DatapackGeneralLoaderMonsterDrop.cpp \
    ../../general/base/DatapackGeneralLoaderPlant.cpp \
    ../../general/base/DatapackGeneralLoaderQuest.cpp \
    ../../general/base/DatapackGeneralLoaderReputation.cpp \
    ../../general/fight/CommonFightEngineBase.cpp \
    ../../general/fight/FightLoader.cpp \
    ../../general/fight/FightLoaderBuff.cpp \
    ../../general/fight/FightLoaderFight.cpp \
    ../../general/fight/FightLoaderMonster.cpp \
    ../../general/fight/FightLoaderSkill.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../base/BaseServerMasterLoadDictionary.cpp \
    ../base/DatabaseBase.cpp \
    ../base/BaseServerMasterSendDatapack.cpp \
    ../base/DatabaseFunction.cpp \
    ../base/TinyXMLSettings.cpp

HEADERS += \
    EpollClientLoginMaster.h \
    EpollServerLoginMaster.h \
    CharactersGroup.h \
    PlayerUpdaterToLogin.h \
    PurgeTheLockedAccount.h \
    CheckTimeoutGameServer.h \
    AutomaticPingSend.h \
    TimeRangeEvent.h \
    ../epoll/Epoll.h \
    ../epoll/EpollGenericSslServer.h \
    ../epoll/EpollGenericServer.h \
    ../epoll/EpollClient.h \
    ../epoll/EpollSslClient.h \
    ../epoll/EpollTimer.h \
    ../epoll/db/EpollPostgresql.h \
    ../epoll/BaseClassSwitch.h \
    ../epoll/EpollSocket.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/CommonDatapack.h \
    ../../general/base/DatapackGeneralLoader.h \
    ../../general/fight/CommonFightEngineBase.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/GeneralType.h \
    ../../general/fight/FightLoader.h \
    ../../general/base/CommonSettingsCommon.h \
    ../../general/base/cpp11addition.h \
    ../../general/base/GeneralVariable.h \
    ../base/BaseServerMasterLoadDictionary.h \
    ../base/DatabaseBase.h \
    ../base/TinyXMLSettings.h \
    ../base/DatabaseFunction.h \
    ../base/BaseServerMasterSendDatapack.h \
    ../VariableServer.h

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
