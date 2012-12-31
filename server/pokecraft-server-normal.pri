include(pokecraft-server.pri)
include(../general/general.pri)

SOURCES += $$PWD/NormalServer.cpp \
    $$PWD/../client/base/Api_protocol.cpp \
    $$PWD/../client/base/Api_client_real.cpp \
    $$PWD/../client/base/Api_client_virtual.cpp

HEADERS += $$PWD/NormalServer.h \
    $$PWD/../client/base/ClientStructures.h \
    $$PWD/../client/base/Api_protocol.h \
    $$PWD/../client/base/Api_client_real.h \
    $$PWD/../client/base/Api_client_virtual.h

