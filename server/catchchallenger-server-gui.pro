include(catchchallenger-server-normal.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += gui network

TARGET = catchchallenger-server-gui

win32:CONFIG   += console

SOURCES += MainWindow.cpp \
    main-gui.cpp
HEADERS  += MainWindow.h

FORMS    += MainWindow.ui
TEMPLATE = app
RESOURCES += \
    ../client/resources/client-resources-multi.qrc
