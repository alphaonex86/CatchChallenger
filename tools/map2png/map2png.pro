include(../../client/tiled/tiled.pri)

TEMPLATE = app
TARGET = map2png

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"

QT += xml

DEFINES += ONLYMAPRENDER

include(../../general/general.pri)

win32:RC_FILE += resources-windows.rc

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
         map2png.cpp \
    ../../client/base/Map_client.cpp \
    ../../client/base/render/MapVisualiserOrder.cpp \
    ../../client/base/render/MapDoor.cpp

HEADERS += map2png.h \
    ../../client/base/ClientStructures.h \
    ../../client/base/Map_client.h \
    ../../client/base/render/MapVisualiserOrder.h \
    ../../client/base/render/MapDoor.h

RESOURCES += \
    resources.qrc

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
