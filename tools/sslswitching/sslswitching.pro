QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sslswitching
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    Server.cpp

HEADERS  += MainWindow.h \
    Server.h

FORMS    += MainWindow.ui
