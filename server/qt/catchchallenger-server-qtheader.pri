QT       += sql core
DEFINES += CATCHCHALLENGER_CLASS_QT

QT       += network

HEADERS += $$PWD/QtServer.hpp \
    $$PWD/EventThreader.hpp \
    $$PWD/QFakeServer.hpp \
    $$PWD/QFakeSocket.hpp \
    $$PWD/QSslServer.hpp \
    $$PWD/QtClient.hpp \
    $$PWD/QtClientMapManagement.hpp \
    $$PWD/QtPlayerUpdater.hpp \
    $$PWD/QtServerStructures.hpp \
    $$PWD/QtTimeRangeEventScanBase.hpp \
    $$PWD/QtTimerEvents.hpp \
    $$PWD/QtDatabase.hpp \
    $$PWD/InternalServer.hpp \
    $$PWD/NormalServer.hpp
