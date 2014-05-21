#-------------------------------------------------
#
# Project created by QtCreator 2014-04-22T15:27:03
#
#-------------------------------------------------

CONFIG += c++11

QT       += core

QT       -= gui

TARGET = epoll-server-with-buffer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    Client.cpp \
    Socket.cpp \
    Server.cpp \
    Epoll.cpp \
    Timer.cpp \
    TimerDisplayEventBySeconds.cpp

HEADERS += \
    Client.h \
    Socket.h \
    Server.h \
    Epoll.h \
    BaseClassSwitch.h \
    Timer.h \
    TimerDisplayEventBySeconds.h

