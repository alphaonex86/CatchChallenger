include(../../client/tiled/tiled.pri)

TEMPLATE = app
TARGET = map2png

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
    ../../client/base/interface/MapDoor.cpp

HEADERS += map2png.h \
    ../../client/base/ClientStructures.h \
    ../../client/base/Map_client.h \
    ../../client/base/render/MapVisualiserOrder.h \
    ../../client/base/interface/MapDoor.h

RESOURCES += \
    resources.qrc

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
