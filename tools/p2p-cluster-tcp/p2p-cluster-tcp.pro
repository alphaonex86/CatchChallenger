QT       -= gui core
CONFIG += c++11
QMAKE_CFLAGS="-std=c++0x"
QMAKE_CXXFLAGS="-std=c++0x"

DEFINES += EPOLLCATCHCHALLENGERSERVER CATCHCHALLENGER_CLASS_P2PCLUSTER SERVERSSL
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL

LIBS += -lcrypto -lssl
TARGET = p2p-cluster-tcp
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    ../../server/epoll/Epoll.cpp \
    ../../server/epoll/EpollSocket.cpp \
    ../../server/base/TinyXMLSettings.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    P2PClient.cpp \
    P2PServer.cpp \
    P2PTimerConnect.cpp \
    ../../server/epoll/EpollTimer.cpp

HEADERS += \
    ../../server/epoll/Epoll.h \
    ../../server/epoll/EpollSocket.h \
    ../../server/epoll/BaseClassSwitch.h \
    ../../server/base/TinyXMLSettings.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/cpp11addition.h \
    ../../general/base/PortableEndian.h \
    P2PClient.h \
    P2PServer.h \
    P2PTimerConnect.h \
    ../../server/epoll/EpollTimer.h

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
