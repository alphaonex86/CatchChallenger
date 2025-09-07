QT       += sql core
DEFINES += CATCHCHALLENGER_CLASS_QT

QT       += network

HEADERS += \
    $$PWD/EventThreader.hpp \
    $$PWD/QFakeServer.hpp \
    $$PWD/QFakeSocket.hpp \
    $$PWD/QSslServer.hpp \
    $$PWD/QtClient.hpp \
    $$PWD/QtClientList.hpp \
    $$PWD/QtClientMapManagement.hpp \
    $$PWD/timer/QtPlayerUpdater.hpp \
    $$PWD/QtServerStructures.hpp \
    $$PWD/timer/QtTimeRangeEventScanBase.hpp \
    $$PWD/timer/QtTimerEvents.hpp \
    $$PWD/QtServer.hpp \
    $$PWD/InternalServer.hpp \
    $$PWD/NormalServer.hpp \
    $$PWD/db/QtDatabase.hpp \
    $$PWD/db/QtDatabaseMySQL.hpp \
    $$PWD/db/QtDatabasePostgreSQL.hpp \
    $$PWD/db/QtDatabaseSQLite.hpp
