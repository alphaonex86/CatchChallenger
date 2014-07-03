include(../base/client.pri)
include(../base/solo.pri)
include(../base/multi.pri)
include(../../server/catchchallenger-server-qt.pri)
include(../../general/general.pri)
include(specific.pri)

TARGET = catchchallenger-ultimate

#QMAKE_CFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer
#QMAKE_CXXFLAGS_DEBUG   += -fsanitize=address -fno-omit-frame-pointer
#QMAKE_LFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer
