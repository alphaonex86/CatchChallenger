#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -fno-rtti"
TEMPLATE = app
QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
QMAKE_CXXFLAGS+="-fstack-protector-all -g -fno-rtti"
linux:QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
linux:QMAKE_CFLAGS+="-Wno-missing-braces -Wall -Wextra"

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

DEFINES += CATCHCHALLENGER_DB_PREPAREDSTATEMENT

CONFIG += c++20

TARGET = catchchallenger-server-login
CONFIG   += console

TEMPLATE = app

SOURCES += \
    ../../general/base/CompressionProtocol.cpp \
    ../../general/base/CatchChallenger_Hash.cpp \
    ../../general/blake3/blake3.c \
    ../../general/blake3/blake3_dispatch.c \
    ../../general/blake3/blake3_portable.c \
    main-epoll-login-slave.cpp \
    EpollClientLoginSlave.cpp \
    EpollServerLoginSlave.cpp \
    EpollClientLoginSlaveStaticVar.cpp \
    EpollClientLoginSlaveHeavyLoad.cpp \
    LinkToMaster.cpp \
    LinkToMasterStaticVar.cpp \
    LinkToMasterProtocolParsing.cpp \
    LinkToMasterProtocolParsingMessage.cpp \
    LinkToMasterProtocolParsingReply.cpp \
    EpollClientLoginSlaveProtocolParsing.cpp \
    EpollClientLoginSlaveWrite.cpp \
    CharactersGroupForLogin.cpp \
    CharactersGroupClient.cpp \
    LinkToGameServerStaticVar.cpp \
    LinkToGameServerProtocolParsing.cpp \
    LinkToGameServer.cpp \
    TimerDdos.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../epoll/EpollSslClient.cpp \
    ../epoll/db/EpollPostgresql.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp \
    ../epoll/EpollTimer.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../base/GlobalServerData.cpp \
    ../base/DatabaseBase.cpp \
    ../base/PreparedDBQueryLogin.cpp \
    ../base/PreparedDBQueryCommon.cpp \
    ../base/PreparedDBQueryServer.cpp \
    ../base/PreparedStatementUnit.cpp \
    ../base/BaseServer/BaseServerLogin.cpp \
    ../base/SqlFunction.cpp \
    ../base/DictionaryLogin.cpp \
    ../base/TinyXMLSettings.cpp \
    ../base/DatabaseFunction.cpp \
    ../base/StringWithReplacement.cpp \
    TimerDetectTimeout.cpp \
    ../../general/base/Version.cpp

HEADERS += \
    ../../general/base/CompressionProtocol.hpp \
    ../../general/base/CatchChallenger_Hash.hpp \
    EpollClientLoginSlave.hpp \
    EpollServerLoginSlave.hpp \
    LinkToMaster.hpp \
    CharactersGroupForLogin.hpp \
    LinkToGameServer.hpp \
    TimerDdos.hpp \
    VariableLoginServer.hpp \
    ../epoll/Epoll.hpp \
    ../epoll/EpollGenericSslServer.hpp \
    ../epoll/EpollGenericServer.hpp \
    ../epoll/EpollClient.hpp \
    ../epoll/EpollSocket.hpp \
    ../epoll/EpollSslClient.hpp \
    ../epoll/db/EpollPostgresql.hpp \
    ../epoll/EpollClientToServer.hpp \
    ../epoll/EpollTimer.hpp \
    ../epoll/EpollSslClientToServer.hpp \
    ../epoll/BaseClassSwitch.hpp \
    ../../general/base/ProtocolParsing.hpp \
    ../../general/base/ProtocolParsingCheck.hpp \
    ../../general/base/GeneralStructures.hpp \
    ../../general/base/FacilityLibGeneral.hpp \
    ../../general/base/GeneralVariable.hpp \
    ../../general/base/CommonSettingsCommon.hpp \
    ../../general/base/cpp11addition.hpp \
    ../base/DatabaseBase.hpp \
    ../base/PreparedDBQuery.hpp \
    ../base/PreparedStatementUnit.hpp \
    ../base/BaseServer/BaseServerLogin.hpp \
    ../base/SqlFunction.hpp \
    ../base/DictionaryLogin.hpp \
    ../base/TinyXMLSettings.hpp \
    ../base/DatabaseFunction.hpp \
    ../base/StringWithReplacement.hpp \
    ../base/VariableServer.hpp \
    TimerDetectTimeout.hpp

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2c.cpp

LIBS += -lzstd

#precompile_header:!isEmpty(PRECOMPILED_HEADER) {
#DEFINES += USING_PCH
#
