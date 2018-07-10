QT       -= gui core
CONFIG += c++11
QMAKE_CFLAGS="-std=c++0x"
QMAKE_CXXFLAGS="-std=c++0x"

LIBS += -lcrypto
TARGET = stats
CONFIG   += console
CONFIG   -= app_bundle

DEFINES += EPOLLCATCHCHALLENGERSERVER EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION SERVERNOBUFFER
DEFINES += CATCHCHALLENGER_CLASS_STATS

TEMPLATE = app

SOURCES += main.cpp \
    LinkToLogin.cpp \
    LinkToLoginProtocolParsing.cpp \
    LinkToLoginStaticVar.cpp \
    ../../server/epoll/Epoll.cpp \
    ../../server/epoll/EpollSocket.cpp \
    ../../server/base/TinyXMLSettings.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    ../../server/epoll/EpollClient.cpp \
    ../../server/epoll/EpollUnixSocketServer.cpp \
    EpollServerStats.cpp

HEADERS += \
    LinkToLogin.h \
    ../../server/epoll/Epoll.h \
    ../../server/epoll/EpollSocket.h \
    ../../server/epoll/BaseClassSwitch.h \
    ../../server/base/TinyXMLSettings.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/cpp11addition.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../../general/base/PortableEndian.h \
    ../../server/epoll/EpollClient.h \
    ../../server/epoll/EpollUnixSocketServer.h \
    EpollServerStats.h

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2c.cpp

DEFINES += DEBUG_PROTOCOLPARSING_RAW_NETWORK PROTOCOLPARSINGDEBUG PROTOCOLPARSINGINPUTOUTPUTDEBUG PROTOCOLPARSINGINPUTDEBUG
