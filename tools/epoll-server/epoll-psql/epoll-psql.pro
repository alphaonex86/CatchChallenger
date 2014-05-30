#-------------------------------------------------
#
# Project created by QtCreator 2014-04-22T15:27:03
#
#-------------------------------------------------

CONFIG += c++11

QT       += core

QT       -= gui

TARGET = epoll-psql
CONFIG   += console
CONFIG   -= app_bundle

LIBS+=-lpq

TEMPLATE = app

SOURCES += main.cpp
