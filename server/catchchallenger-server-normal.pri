include(../general/general.pri)
include(catchchallenger-server-qt.pri)

SOURCES += $$PWD/NormalServer.cpp \
    QSslServer.cpp \
    NormalServerGlobal.cpp

HEADERS += $$PWD/NormalServer.h \
    $$PWD/../client/base/ClientStructures.h \
    QSslServer.h \
    NormalServerGlobal.h

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../general/base/tinyXML2/tinyxml2c.cpp
