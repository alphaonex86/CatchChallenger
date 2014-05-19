include(catchchallenger-server-normal.pri)

QT       -= gui widgets

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += epoll/main-epoll.cpp \
    epoll/EpollClient.cpp \
    epoll/EpollSocket.cpp \
    epoll/EpollServer.cpp \
    epoll/Epoll.cpp \
    epoll/EpollTimer.cpp \
    epoll/TimerDisplayEventBySeconds.cpp

HEADERS += epoll/EpollClient.h \
    epoll/EpollSocket.h \
    epoll/EpollServer.h \
    epoll/Epoll.h \
    epoll/BaseClassSwitch.h \
    epoll/EpollTimer.h \
    epoll/TimerDisplayEventBySeconds.h

