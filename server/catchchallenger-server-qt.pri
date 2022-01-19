include(catchchallenger-server.pri)

QT       += sql core
DEFINES += CATCHCHALLENGER_CLASS_QT

RESOURCES += $$PWD/all-server-resources.qrc \
    $$PWD/databases/resources-db-sqlite.qrc

#now in generale, only linux is C only, mac, windows, other is in Qt for compatibility
#win32:RC_FILE += $$PWD/resources-windows.rc

QT       += gui network

SOURCES += $$PWD/qt/QtServer.cpp \
    $$PWD/qt/EventThreader.cpp \
    $$PWD/qt/QFakeServer.cpp \
    $$PWD/qt/QFakeSocket.cpp \
    $$PWD/qt/QSslServer.cpp \
    $$PWD/qt/QtClient.cpp \
    $$PWD/qt/QtClientMapManagement.cpp \
    $$PWD/qt/QtPlayerUpdater.cpp \
    $$PWD/qt/QtTimeRangeEventScanBase.cpp \
    $$PWD/qt/QtTimerEvents.cpp \
    $$PWD/qt/QtDatabase.cpp \
    $$PWD/qt/InternalServer.cpp \
    $$PWD/qt/NormalServer.cpp \
    $$PWD/base/NormalServerGlobal.cpp
HEADERS += $$PWD/qt/QtServer.hpp \
    $$PWD/qt/EventThreader.hpp \
    $$PWD/qt/QFakeServer.hpp \
    $$PWD/qt/QFakeSocket.hpp \
    $$PWD/qt/QSslServer.hpp \
    $$PWD/qt/QtClient.hpp \
    $$PWD/qt/QtClientMapManagement.hpp \
    $$PWD/qt/QtPlayerUpdater.hpp \
    $$PWD/qt/QtServerStructures.hpp \
    $$PWD/qt/QtTimeRangeEventScanBase.hpp \
    $$PWD/qt/QtTimerEvents.hpp \
    $$PWD/qt/QtDatabase.hpp \
    $$PWD/qt/InternalServer.hpp \
    $$PWD/qt/NormalServer.hpp \
    $$PWD/base/NormalServerGlobal.hpp
