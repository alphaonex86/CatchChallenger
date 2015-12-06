include(../../general/general.pri)
include(../base/client.pri)
include(../base/solo.pri)
include(../base/multi.pri)
include(../../server/catchchallenger-server-qt.pri)
include(specific.pri)

TARGET = catchchallenger-ultimate

QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
QMAKE_LFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer
