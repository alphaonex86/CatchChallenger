QT       -= gui core
CONFIG += c++11
QMAKE_CFLAGS="-std=c++0x"
QMAKE_CXXFLAGS="-std=c++0x"

LIBS += -lcrypto
TARGET = stats
CONFIG   += console
CONFIG   -= app_bundle

DEFINES += EPOLLCATCHCHALLENGERSERVER EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION SERVERNOBUFFER
DEFINES += TIXML_USE_STL

TEMPLATE = app

SOURCES += main.cpp \
    LinkToLogin.cpp \
    LinkToLoginProtocolParsing.cpp \
    LinkToLoginStaticVar.cpp \
    ../../server/epoll/Epoll.cpp \
    ../../server/epoll/EpollSocket.cpp \
    ../../server/base/TinyXMLSettings.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/tinyXML/tinystr.cpp \
    ../../general/base/tinyXML/tinyxml.cpp \
    ../../general/base/tinyXML/tinyxmlerror.cpp \
    ../../general/base/tinyXML/tinyxmlparser.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    ../../server/epoll/EpollClient.cpp

HEADERS += \
    LinkToLogin.h \
    ../../server/epoll/Epoll.h \
    ../../server/epoll/EpollSocket.h \
    ../../server/epoll/BaseClassSwitch.h \
    ../../server/base/TinyXMLSettings.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/tinyXML/tinystr.h \
    ../../general/base/tinyXML/tinyxml.h \
    ../../general/base/cpp11addition.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../../general/base/PortableEndian.h \
    ../../server/epoll/EpollClient.h
