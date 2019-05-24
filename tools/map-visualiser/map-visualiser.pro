INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
LIBS *= -ltiled

include(../../client/base/client.pri)
include(../../general/general.pri)
include(../../client/base/solo.pri)
include(../../client/base/multi.pri)
include(../../server/catchchallenger-server-qt.pri)

TEMPLATE = app
TARGET = map-visualiser

QT += xml opengl network

win32:RC_FILE += resources-windows.rc

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
    OptionsV.cpp \
    MapControllerV.cpp

HEADERS += \
    OptionsV.h \
    MapControllerV.h

RESOURCES += \
    resources.qrc

FORMS += \
    OptionsV.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc
