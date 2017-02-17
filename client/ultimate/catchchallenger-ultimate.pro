include(../base/client.pri)
include(../../general/general.pri)
include(../base/solo.pri)
include(../base/multi.pri)
include(../../server/catchchallenger-server-qt.pri)
include(specific.pri)

TARGET = catchchallenger-ultimate

QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra -fno-omit-frame-pointer -fsanitize=address"
QMAKE_CFLAGS+="-Wall -Wextra -fno-omit-frame-pointer -fsanitize=address"
QMAKE_LFLAGS+="-fno-omit-frame-pointer -Wl,--no-undefined -fsanitize=address -stdlib=libc++"
#/usr/lib64/qt5/bin/qmake -spec linux-clang
