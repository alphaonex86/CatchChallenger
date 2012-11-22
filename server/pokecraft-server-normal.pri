include(pokecraft-server.pri)

SOURCES += NormalServer.cpp \
    ../general/base/DebugClass.cpp \
    ../general/base/MoveOnTheMap.cpp \
    ../general/base/ChatParsing.cpp \
    ../general/base/ProtocolParsing.cpp \
    ../general/base/QFakeServer.cpp \
    ../general/base/QFakeSocket.cpp \
    ../general/base/Map_loader.cpp \
    ../general/base/Map.cpp \
    ../general/base/ConnectedSocket.cpp \
    ../general/base/FacilityLib.cpp \
    ../client/base/Api_protocol.cpp \
    ../client/base/Api_client_real.cpp \
    ../client/base/Api_client_virtual.cpp

HEADERS += NormalServer.h \
    ../general/base/ConnectedSocket.h \
    ../general/base/FacilityLib.h \
    ../general/base/MoveOnTheMap.h \
    ../general/base/DebugClass.h \
    ../general/base/GeneralStructures.h \
    ../general/base/ChatParsing.h \
    ../general/base/GeneralVariable.h \
    ../general/base/ProtocolParsing.h \
    ../general/base/QFakeServer.h \
    ../general/base/QFakeSocket.h \
    ../general/base/Map_loader.h \
    ../general/base/Map.h \
    ../client/base/ClientStructures.h \
    ../client/base/Api_protocol.h \
    ../client/base/Api_client_real.h \
    ../client/base/Api_client_virtual.h

