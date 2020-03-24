include(catchchallenger-server.pri)

QT       += sql core
DEFINES += CATCHCHALLENGER_CLASS_QT

RESOURCES += $$PWD/all-server-resources.qrc \
    $$PWD/databases/resources-db-sqlite.qrc

#now in generale, only linux is C only, mac, windows, other is in Qt for compatibility
#win32:RC_FILE += $$PWD/resources-windows.rc

QT       += gui network

SOURCES += $$PWD/base/QtServer.cpp \
    $$PWD/base/QtTimerEvents.cpp \
    $$PWD/base/QtDatabase.cpp \
    $$PWD/../client/qt/ConnectedSocket.cpp \
    $$PWD/../client/qt/QFakeSocket.cpp \
    $$PWD/../client/qt/QFakeServer.cpp
HEADERS += $$PWD/base/QtServer.hpp \
    $$PWD/base/QtTimerEvents.hpp \
    $$PWD/base/QtDatabase.hpp \
    $$PWD/../client/qt/ConnectedSocket.hpp \
    $$PWD/../client/qt/QFakeServer.hpp \
    $$PWD/../client/qt/QFakeSocket.hpp


