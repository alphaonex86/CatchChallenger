QMAKE_CFLAGS += -O0
QMAKE_CXXFLAGS += -O0

QT       += core gui network opengl xml

TEMPLATE = app

SOURCES += $$PWD/base/DebugClass.cpp \
    $$PWD/base/ChatParsing.cpp \
    $$PWD/base/QFakeSocket.cpp \
    $$PWD/base/QFakeServer.cpp \
    $$PWD/base/ProtocolParsing.cpp \
    $$PWD/base/MoveOnTheMap.cpp \
    $$PWD/base/Map_loader.cpp \
    $$PWD/base/Map.cpp \
    $$PWD/base/FacilityLib.cpp \
    $$PWD/base/ConnectedSocket.cpp \
    $$PWD/fight/FightEngine.cpp \
    ../general/fight/FightLoader.cpp

HEADERS  += $$PWD/base/GeneralStructures.h \
    $$PWD/base/DebugClass.h \
    $$PWD/base/ChatParsing.h \
    $$PWD/base/VariableGeneral.h \
    $$PWD/base/QFakeServer.h \
    $$PWD/base/ProtocolParsing.h \
    $$PWD/base/MoveOnTheMap.h \
    $$PWD/base/Map_loader.h \
    $$PWD/base/Map.h \
    $$PWD/base/GeneralVariable.h \
    $$PWD/base/QFakeSocket.h \
    $$PWD/base/FacilityLib.h \
    $$PWD/base/ConnectedSocket.h \
    $$PWD/fight/FightEngine.h \
    ../general/fight/FightLoader.h
