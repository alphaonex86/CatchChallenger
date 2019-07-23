DEFINES += CATCHCHALLENGER_CLIENT

SOURCES += \
    $$PWD/Api_protocol.cpp \
    $$PWD/Api_protocol_loadchar.cpp \
    $$PWD/Api_protocol_message.cpp \
    $$PWD/Api_protocol_query.cpp \
    $$PWD/Api_protocol_reply.cpp \
    $$PWD/DatapackClientLoader.cpp

HEADERS  += \
    $$PWD/Api_protocol.h \
    $$PWD/ClientStructures.h \
    $$PWD/DatapackClientLoader.h \
    $$PWD/ClientVariableAudio.h
