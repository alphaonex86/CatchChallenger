CONFIG += c++11

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = untitled
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp

HEADERS  += MainWindow.h

FORMS    += MainWindow.ui
