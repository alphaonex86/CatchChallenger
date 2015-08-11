#-------------------------------------------------
#
# Project created by QtCreator 2015-08-11T16:16:12
#
#-------------------------------------------------

QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -funroll-loops -ffast-math -funsafe-loop-optimizations"
QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -funroll-loops -ffast-math -funsafe-loop-optimizations"

CONFIG += c++11

QT       -= gui core

TARGET = xxhashbenchmark
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    xxhash.c

HEADERS += \
    xxhash.h
