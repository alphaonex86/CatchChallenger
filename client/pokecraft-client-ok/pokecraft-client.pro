#-------------------------------------------------
#
# Project created by QtCreator 2011-08-05T13:39:24
#
#-------------------------------------------------

QT       += core gui network

TARGET = pokecraft-client
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Pokecraft_client.cpp \
    DebugClass.cpp

HEADERS  += mainwindow.h \
    Pokecraft_client.h \
    DebugClass.h \
    Structures.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    Pokecraft_client
