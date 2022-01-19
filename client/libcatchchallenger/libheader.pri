HEADERS  += \
    $$PWD/Api_protocol.hpp \
    $$PWD/ClientStructures.hpp \
    $$PWD/ClientVariable.hpp \
    $$PWD/DatapackClientLoader.hpp \
    $$PWD/DatapackChecksum.hpp \
    $$PWD/ZstdDecode.hpp  \
    $$PWD/TarDecode.hpp
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2
HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.h
#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-serverheader.pri)
}
else
{
}
