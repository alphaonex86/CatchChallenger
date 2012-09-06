INCLUDEPATH += ../../general/libtiled/
DEPENDPATH += ../../general/libtiled/
LIBS *= -ltiled

TEMPLATE = app
TARGET = map-visualiser-qt
INSTALLS += target
TEMPLATE = app

QT += xml

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
	 map-visualiser-qt.cpp \
    ../../general/base/MoveOnTheMap.cpp \
    ../../general/base/Map_loader.cpp \
    ../../general/base/Map.cpp \
    ../../general/base/DebugClass.cpp \
    ../../general/base/FacilityLib.cpp \
    ../../client/base/Map_client.cpp

HEADERS += map-visualiser-qt.h \
    ../../general/base/Map_loader.h \
    ../../general/base/Map.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/MoveOnTheMap.h \
    ../../client/base/ClientStructures.h \
    ../../general/base/DebugClass.h \
    ../../general/base/FacilityLib.h \
    ../../client/base/Map_client.h

RESOURCES += \
    resources.qrc
