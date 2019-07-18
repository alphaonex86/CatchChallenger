include(../base/client.pri)
include(../../general/general.pri)
include(../base/multi.pri)

!contains(DEFINES, NOSINGLEPLAYER) {
    include(../base/solo.pri)
    include(../../server/catchchallenger-server-qt.pri)
    INCLUDEPATH += -I/usr/include/
}
include(specific.pri)

TARGET = catchchallenger
