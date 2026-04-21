QT       -= gui core
CONFIG += c++20

QMAKE_CXXFLAGS += -Os -fno-exceptions
QMAKE_CFLAGS += -Os -fno-exceptions
QMAKE_LFLAGS += -Os -s

TARGET = stats
CONFIG   += console
CONFIG   -= app_bundle

DEFINES += EPOLLCATCHCHALLENGERSERVER EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION SERVERNOBUFFER
DEFINES += CATCHCHALLENGER_CLASS_STATS NOWEBSOCKET

DEFINES += BLAKE3_NO_SSE2 BLAKE3_NO_SSE41 BLAKE3_NO_AVX2 BLAKE3_NO_AVX512

TEMPLATE = app

SOURCES += main.cpp \
    ../../general/base/CatchChallenger_Hash.cpp \
    ../../general/blake3/blake3.c \
    ../../general/blake3/blake3_dispatch.c \
    ../../general/blake3/blake3_portable.c \
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
    ../../general/base/CatchChallenger_Hash.hpp \
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

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/tinyXML2/tinyxml2c.cpp

#precompile_header:!isEmpty(PRECOMPILED_HEADER) {
#DEFINES += USING_PCH
#}

#DEFINES += DEBUG_PROTOCOLPARSING_RAW_NETWORK PROTOCOLPARSINGDEBUG PROTOCOLPARSINGINPUTOUTPUTDEBUG PROTOCOLPARSINGINPUTDEBUG
