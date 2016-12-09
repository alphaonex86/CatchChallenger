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
#DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML1
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

defined(CATCHCHALLENGER_XLMPARSER_TINYXML1)
{
    DEFINES += TIXML_USE_STL
    HEADERS += $$PWD/../general/base/tinyXML/tinystr.h \
        $$PWD/../general/base/tinyXML/tinyxml.h

    SOURCES += $$PWD/../general/base/tinyXML/tinystr.cpp \
        $$PWD/../general/base/tinyXML/tinyxml.cpp \
        $$PWD/../general/base/tinyXML/tinyxmlerror.cpp \
        $$PWD/../general/base/tinyXML/tinyxmlparser.cpp
}
defined(CATCHCHALLENGER_XLMPARSER_TINYXML2)
{
    HEADERS += $$PWD/../general/base/tinyXML2/tinyxml2.h
    SOURCES += $$PWD/../general/base/tinyXML2/tinyxml2.cpp
}
