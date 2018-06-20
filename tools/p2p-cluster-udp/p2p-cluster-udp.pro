QT       -= gui core
CONFIG += c++11
QMAKE_CFLAGS="-std=c++0x"
QMAKE_CXXFLAGS="-std=c++0x"

DEFINES += EPOLLCATCHCHALLENGERSERVER CATCHCHALLENGER_CLASS_P2PCLUSTER SERVERSSL
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL

LIBS += -lhogweed
TARGET = p2p-cluster-udp
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
    P2PServerUDP.cpp \
    P2PTimerConnect.cpp \
    ../../server/epoll/EpollTimer.cpp \
    P2PTimerHandshake2.cpp \
    P2PTimerHandshake3.cpp \
    HostConnected.cpp \
    P2PPeer.cpp

HEADERS += \
    ../../server/epoll/Epoll.h \
    ../../server/epoll/EpollSocket.h \
    ../../server/epoll/BaseClassSwitch.h \
    ../../server/base/TinyXMLSettings.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/cpp11addition.h \
    ../../general/base/PortableEndian.h \
    P2PServerUDP.h \
    P2PTimerConnect.h \
    ../../server/epoll/EpollTimer.h \
    P2PTimerHandshake2.h \
    P2PTimerHandshake3.h \
    HostConnected.h \
    P2PPeer.h

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp
