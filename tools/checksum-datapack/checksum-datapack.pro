#-------------------------------------------------
#
# Project created by QtCreator 2016-01-07T18:15:49
#
#-------------------------------------------------

CONFIG += c++11
QT       -= gui core

TARGET = checksum-datapack
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

LIBS += -lcrypto

SOURCES += main.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/cpp11addition.cpp

HEADERS += \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/cpp11addition.h
