INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
LIBS *= -ltiled

TEMPLATE = app
TARGET = map-visualiser

QT += xml

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
    ../../general/base/MoveOnTheMap.cpp \
    ../../general/base/Map_loader.cpp \
    ../../general/base/Map.cpp \
    ../../general/base/DebugClass.cpp \
    ../../general/base/FacilityLib.cpp \
    ../../client/base/Map_client.cpp \
    ../../client/base/render/TileLayerItem.cpp \
    ../../client/base/render/ObjectGroupItem.cpp \
    ../../client/base/render/MapVisualiser.cpp \
    ../../client/base/render/MapVisualiser-move.cpp \
    ../../client/base/render/MapVisualiser-move-player.cpp \
    ../../client/base/render/MapVisualiser-map.cpp \
    ../../client/base/render/MapObjectItem.cpp \
    ../../client/base/render/MapItem.cpp \
    Options.cpp

HEADERS += \
    ../../general/base/Map_loader.h \
    ../../general/base/Map.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/MoveOnTheMap.h \
    ../../client/base/ClientStructures.h \
    ../../general/base/DebugClass.h \
    ../../general/base/FacilityLib.h \
    ../../client/base/Map_client.h \
    ../../client/base/render/TileLayerItem.h \
    ../../client/base/render/ObjectGroupItem.h \
    ../../client/base/render/MapVisualiser.h \
    ../../client/base/render/MapObjectItem.h \
    ../../client/base/render/MapItem.h \
    Options.h

RESOURCES += \
    resources.qrc

FORMS += \
    Options.ui
