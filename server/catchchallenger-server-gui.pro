include(catchchallenger-server-normal.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += gui

QMAKE_CFLAGS += -O0
QMAKE_CXXFLAGS += -O0

TARGET = catchchallenger-server-gui

win32:CONFIG   += console

SOURCES += MainWindow.cpp \
    main.cpp
HEADERS  += MainWindow.h

FORMS    += MainWindow.ui

RESOURCES += \
    ../client/base/resources/client-resources-multi.qrc
