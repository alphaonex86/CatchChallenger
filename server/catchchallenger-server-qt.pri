include(catchchallenger-server.pri)

QT       += sql core

RESOURCES += $$PWD/all-server-resources.qrc

win32:RC_FILE += $$PWD/resources-windows.rc


win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
QT       += gui network xml

SOURCES += $$PWD/base/QtServer.cpp \
    $$PWD/base/QtTimerEvents.cpp \
    $$PWD/base/QtDatabase.cpp
HEADERS += $$PWD/base/QtServer.h \
    $$PWD/base/QtTimerEvents.h \
    $$PWD/base/QtDatabase.h

