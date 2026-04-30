QT       += sql core
DEFINES += CATCHCHALLENGER_CLASS_QT CATCHCHALLENGER_DB_SQLITE

#match the client pris (qtcpu800x600/base/client.pri, qtopengl/client.pri):
#opt out of QtWebSockets when NOWEBSOCKET is set (cross-builds where the
#WebSockets module is unavailable, e.g. Qt-for-mac without QtWebSockets).
!contains(DEFINES, NOWEBSOCKET) {
    QT += websockets
}

RESOURCES += $$PWD/../all-server-resources.qrc \
    $$PWD/../databases/resources-db-sqlite.qrc

#now in generale, only linux is C only, mac, windows, other is in Qt for compatibility
#win32:RC_FILE += $$PWD/resources-windows.rc

QT       += network

SOURCES += \
    $$PWD/EventThreader.cpp \
    $$PWD/QFakeServer.cpp \
    $$PWD/QFakeSocket.cpp \
    $$PWD/QSslServer.cpp \
    $$PWD/QtClient.cpp \
    $$PWD/QtClientList.cpp \
    $$PWD/QtClientWithMap.cpp \
    $$PWD/timer/QtPlayerUpdater.cpp \
    $$PWD/timer/QtTimeRangeEventScanBase.cpp \
    $$PWD/timer/QtTimerEvents.cpp \
    $$PWD/QtServer.cpp \
    $$PWD/InternalServer.cpp \
    $$PWD/NormalServer.cpp \
    $$PWD/db/QtDatabase.cpp \
    $$PWD/db/QtDatabaseMySQL.cpp \
    $$PWD/db/QtDatabasePostgreSQL.cpp \
    $$PWD/db/QtDatabaseSQLite.cpp
