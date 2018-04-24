#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -fno-rtti"

#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion -Wno-disabled-macro-expansion"
#QMAKE_CXXFLAGS+="-Wno-weak-vtables -Wno-non-virtual-dtor -Wno-gnu-statement-expression -Wno-implicit-fallthrough -Wno-float-equal -Wno-unreachable-code -Wno-missing-noreturn -Wno-unreachable-code-return -Wno-vla-extension -Wno-format-nonliteral -Wno-vla -Wno-embedded-directive -Wno-missing-variable-declarations -Wno-missing-braces -Wno-disabled-macro-expansion"

#QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
#QMAKE_CXXFLAGS+="-fstack-protector-all -g -fno-rtti"

QT       -= gui widgets sql xml network core

#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += CATCHCHALLENGER_CLASS_GATEWAY

LIBS += -lssl -lcrypto
LIBS    += -lzstd
LIBS += -lcurl

CONFIG += c++11

TARGET = catchchallenger-gateway
CONFIG   += console

TEMPLATE = app

SOURCES += \
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
    EpollClientLoginSlaveProtocolParsing.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp \
    EpollClientLoginSlaveWrite.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    LinkToGameServerStaticVar.cpp \
    LinkToGameServerProtocolParsing.cpp \
    LinkToGameServer.cpp \
    TimerDdos.cpp \
    ../epoll/EpollTimer.cpp \
    DatapackDownloaderBase.cpp \
    DatapackDownloader_sub.cpp \
    DatapackDownloader_main.cpp \
    DatapackDownloaderMainSub.cpp \
    ../../client/base/DatapackChecksum.cpp \
    ../../client/base/qt-tar-compressed/QTarDecode.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    ../../general/base/CommonSettingsServer.cpp \
    EpollClientLoginSlaveDatapack.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    FacilityLibGateway.cpp \
    ../base/TinyXMLSettings.cpp \
    main-epoll-gateway.cpp

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
    ../epoll/EpollClientToServer.h \
    ../epoll/EpollSslClientToServer.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/FacilityLibGeneral.h \
    ../VariableServer.h \
    ../../general/base/GeneralVariable.h \
    LinkToGameServer.h \
    TimerDdos.h \
    ../epoll/EpollTimer.h \
    DatapackDownloaderBase.h \
    DatapackDownloaderMainSub.h \
    ../../client/base/DatapackChecksum.h \
    ../../client/base/qt-tar-compressed/QTarDecode.h \
    ../../general/base/CommonSettingsCommon.h \
    ../../general/base/CommonSettingsServer.h \
    FacilityLibGateway.h \
    ../base/TinyXMLSettings.h \
    ../epoll/BaseClassSwitch.h

#choose one of:
#DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML1
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

defined(CATCHCHALLENGER_XLMPARSER_TINYXML1)
{
    DEFINES += TIXML_USE_STL
    HEADERS += $$PWD/../../general/base/tinyXML/tinystr.h \
        $$PWD/../../general/base/tinyXML/tinyxml.h

    SOURCES += $$PWD/../../general/base/tinyXML/tinystr.cpp \
        $$PWD/../../general/base/tinyXML/tinyxml.cpp \
        $$PWD/../../general/base/tinyXML/tinyxmlerror.cpp \
        $$PWD/../../general/base/tinyXML/tinyxmlparser.cpp
}
defined(CATCHCHALLENGER_XLMPARSER_TINYXML2)
{
    HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
    SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp
}
