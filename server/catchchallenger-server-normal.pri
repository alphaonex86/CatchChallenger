include(catchchallenger-server.pri)
include(../general/general.pri)

SOURCES += $$PWD/NormalServer.cpp \
    QSslServer.cpp \
    NormalServerGlobal.cpp

HEADERS += $$PWD/NormalServer.h \
    $$PWD/../client/base/ClientStructures.h \
    QSslServer.h \
    NormalServerGlobal.h


