include(qt/client.pri)
include(../general/general.pri)
include(qt/multi.pri)

!contains(DEFINES, NOSINGLEPLAYER) {
    include(qt/solo.pri)
    include(../server/catchchallenger-server-qt.pri)
    INCLUDEPATH += -I/usr/include/
}

TARGET = catchchallenger

SOURCES += main.cpp
