#DEFINES += EPOLLCATCHCHALLENGERSERVER

include(../../general/general.pri)

QT       += core network xml
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
    TestUnitCpp.cpp \
    TestString.cpp \
    TestUnitMessageParsing.cpp \
    ../../server/base/StringWithReplacement.cpp \
    TestUnitCompression.cpp

HEADERS += \
    reputation/TestUnitReputation.h \
    TestUnitCpp.h \
    TestString.h \
    TestUnitMessageParsing.h \
    ../../server/base/StringWithReplacement.h \
    TestUnitCompression.h
