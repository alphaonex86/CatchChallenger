include(../../client/tiled/tiled.pri)

TEMPLATE = app
TARGET = map2png
DEFINES += MAXIMIZEPERFORMANCEOVERDATABASESIZE

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"

QT += xml network opengl

DEFINES += ONLYMAPRENDER NOWEBSOCKET

include(../../general/general.pri)
include(../../client/qtmaprender/render.pri)
include(../../client/qtmaprender/renderheader.pri)
include(../../client/libcatchchallenger/lib.pri)
include(../../client/libcatchchallenger/libheader.pri)
include(../../client/libqtcatchchallenger/libqt.pri)
include(../../client/libqtcatchchallenger/libqtheader.pri)

win32:RC_FILE += resources-windows.rc

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
    map2png.cpp

HEADERS += map2png.h

RESOURCES += \
    resources.qrc

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/tinyXML2/tinyxml2c.cpp
