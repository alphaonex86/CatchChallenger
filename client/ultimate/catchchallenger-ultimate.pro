include(../base/client.pri)
include(../../general/general.pri)
include(../base/multi.pri)
!wasm: {
include(../base/solo.pri)
include(../../server/catchchallenger-server-qt.pri)
INCLUDEPATH += -I/usr/include/
}
else {
QT += websockets
}
include(specific.pri)

TARGET = catchchallenger-ultimate
