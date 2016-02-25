include(catchchallenger-server.pri)

QT       += sql core

RESOURCES += $$PWD/all-server-resources.qrc

#now in generale, only linux is C only, mac, windows, other is in Qt for compatibility
#win32:RC_FILE += $$PWD/resources-windows.rc

QT       += gui network xml

SOURCES += $$PWD/base/QtServer.cpp \
    $$PWD/base/QtTimerEvents.cpp \
    $$PWD/base/QtDatabase.cpp
HEADERS += $$PWD/base/QtServer.h \
    $$PWD/base/QtTimerEvents.h \
    $$PWD/base/QtDatabase.h

