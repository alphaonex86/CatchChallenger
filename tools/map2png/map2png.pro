include(../../client/qt/tiled/tiled.pri)

TEMPLATE = app
TARGET = map2png

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"
LIBS += -lzstd

QT += xml

DEFINES += ONLYMAPRENDER NOWEBSOCKET

include(../../general/general.pri)

win32:RC_FILE += resources-windows.rc

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
         map2png.cpp \
    ../../client/qt/Map_client.cpp \
    ../../client/qt/render/MapVisualiserOrder.cpp \
    ../../client/qt/render/MapDoor.cpp

HEADERS += map2png.h \
    ../../client/qt/ClientStructures.hpp \
    ../../client/qt/Map_client.hpp \
    ../../client/qt/render/MapVisualiserOrder.hpp \
    ../../client/qt/render/MapDoor.hpp

RESOURCES += \
    resources.qrc

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
