TEMPLATE = app
TARGET = filedb-converter
CONFIG += console c++17
CONFIG -= app_bundle
QT -= core gui widgets network sql xml

DEFINES += CATCHCHALLENGER_DB_FILE
DEFINES += CATCHCHALLENGER_CACHE_HPS

linux:QMAKE_CFLAGS   += -fstack-protector-all -g -fno-rtti -fno-exceptions
linux:QMAKE_CXXFLAGS += -fstack-protector-all -g -fno-rtti -fno-exceptions
linux:QMAKE_CXXFLAGS += -Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra
linux:QMAKE_CFLAGS   += -Wno-missing-braces -Wall -Wextra

# Reuse HPS (same library catchchallenger-server-filedb.pro uses)
include(../../../general/hps/hps.pri)

# Reuse the same tinyxml2 sources the server uses
include(../../../general/tinyXML2/tinyXML2.pri)
HEADERS += $$PWD/../../../general/tinyXML2/tinyxml2.hpp

# Reuse the same in-memory structures used by the filedb server (HPS serialize/parse
# methods are templated inline in the header - no .cpp needed for these structs)
HEADERS += $$PWD/../../../general/base/GeneralStructures.hpp \
           $$PWD/../../../general/base/GeneralType.hpp

INCLUDEPATH += $$PWD/../../../general

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/FiledbConverter.cpp

HEADERS += \
    $$PWD/FiledbConverter.hpp
