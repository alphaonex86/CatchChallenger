include(../general/general.pri)
include(catchchallenger-server-qt.pri)

SOURCES += $$PWD/NormalServer.cpp \
    QSslServer.cpp \
    NormalServerGlobal.cpp

HEADERS += $$PWD/NormalServer.h \
    $$PWD/../client/base/ClientStructures.h \
    QSslServer.h \
    NormalServerGlobal.h


