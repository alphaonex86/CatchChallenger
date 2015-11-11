include(../../general/general.pri)

QT       += core

QT       -= gui

TARGET = test-unit
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    reputation/just-append-at-level-1.cpp \
    reputation/just-append-at-level-max.cpp \
    reputation/just-append.cpp \
    reputation/level-down.cpp \
    reputation/level-up.cpp \
    TestUnitCpp.cpp

HEADERS += \
    reputation/TestUnitReputation.h \
    TestUnitCpp.h
