DEFINES += CATCHCHALLENGER_CLIENT

SOURCES += \
    $$PWD/Api_protocol.cpp \
    $$PWD/Api_protocol_loadchar.cpp \
    $$PWD/Api_protocol_message.cpp \
    $$PWD/Api_protocol_query.cpp \
    $$PWD/Api_protocol_reply.cpp \
    $$PWD/DatapackClientLoader.cpp \
    $$PWD/DatapackChecksum.cpp \
    $$PWD/ZstdDecode.cpp \
    $$PWD/TarDecode.cpp

DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2c.cpp

include(../../general/general.pri)

DEFINES += CATCHCHALLENGERLIB
