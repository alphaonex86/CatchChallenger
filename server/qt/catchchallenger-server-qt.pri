QT       += sql core
DEFINES += CATCHCHALLENGER_CLASS_QT

QT += websockets
#DEFINES+=NOWEBSOCKET

RESOURCES += $$PWD/../all-server-resources.qrc \
    $$PWD/../databases/resources-db-sqlite.qrc

#now in generale, only linux is C only, mac, windows, other is in Qt for compatibility
#win32:RC_FILE += $$PWD/resources-windows.rc

QT       += network

SOURCES += $$PWD/QtServer.cpp \
    $$PWD/EventThreader.cpp \
    $$PWD/QFakeServer.cpp \
    $$PWD/QFakeSocket.cpp \
    $$PWD/QSslServer.cpp \
    $$PWD/QtClient.cpp \
    $$PWD/QtClientMapManagement.cpp \
    $$PWD/QtPlayerUpdater.cpp \
    $$PWD/QtTimeRangeEventScanBase.cpp \
    $$PWD/QtTimerEvents.cpp \
    $$PWD/QtDatabase.cpp \
    $$PWD/InternalServer.cpp \
    $$PWD/NormalServer.cpp
