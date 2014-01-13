include(../../client/tiled/tiled.pri)

TEMPLATE = app
TARGET = map2png

QT += xml

include(../../general/general.pri)

win32:RC_FILE += resources-windows.rc

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
         map2png.cpp \
    ../../client/base/Map_client.cpp

HEADERS += map2png.h \
    ../../client/base/ClientStructures.h \
    ../../client/base/Map_client.h

RESOURCES += \
    resources.qrc

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
