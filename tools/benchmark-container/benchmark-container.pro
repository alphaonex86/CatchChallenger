QT -= gui core xml network

DEFINES += TIXML_USE_STL EPOLLCATCHCHALLENGERSERVER EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

TARGET = benchmark-container
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    ../../general/base/DatapackGeneralLoader.cpp \
    ../../general/base/CommonDatapack.cpp \
    ../../general/base/tinyXML/tinystr.cpp \
    ../../general/base/tinyXML/tinyxml.cpp \
    ../../general/base/tinyXML/tinyxmlerror.cpp \
    ../../general/base/tinyXML/tinyxmlparser.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/fight/FightLoader.cpp

HEADERS += \
    ../../general/base/DatapackGeneralLoader.h \
    ../../general/base/CommonDatapack.h \
    ../../general/base/tinyXML/tinystr.h \
    ../../general/base/tinyXML/tinyxml.h \
    ../../general/base/cpp11addition.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/GeneralStructuresXml.h \
    ../../general/base/GeneralType.h \
    ../../general/base/GeneralVariable.h \
    ../../general/fight/FightLoader.h
