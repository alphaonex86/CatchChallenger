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
