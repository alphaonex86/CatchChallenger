#-------------------------------------------------
#
# Project created by QtCreator 2014-04-22T15:27:03
#
#-------------------------------------------------

CONFIG += c++11

QT       += core

QT       -= gui

LIBS += -lssl -lcrypto

TARGET = epoll-ssl-server-skeleton
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    EpollSslClient.cpp \
    EpollClient.cpp \
    EpollSocket.cpp \
    EpollSslServer.cpp \
    EpollServer.cpp \
    Epoll.cpp \
    EpollTimer.cpp \
    TimerDisplayEventBySeconds.cpp

HEADERS += \
    EpollSslClient.h \
    EpollClient.h \
    EpollSocket.h \
    EpollSslServer.h \
    EpollServer.h \
    Epoll.h \
    BaseClassSwitch.h \
    EpollTimer.h \
    TimerDisplayEventBySeconds.h

