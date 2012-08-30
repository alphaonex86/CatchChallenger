INCLUDEPATH += libtiled/
DEPENDPATH += libtiled/
LIBS *= -ltiled

TEMPLATE = app
TARGET = map-visualiser-qt
INSTALLS += target
TEMPLATE = app

QT += xml

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

#macx {
#    QMAKE_LIBDIR += $$OUT_PWD/../../bin/Tiled.app/Contents/Frameworks
#} else:win32 {
#    LIBS += -L$$OUT_PWD/../../lib
#} else {
#    QMAKE_LIBDIR += $$OUT_PWD/../../lib
#}

# Make sure the executable can find libtiled
#!win32:!macx {
#    QMAKE_RPATHDIR += \$\$ORIGIN/../lib
#
#    # It is not possible to use ORIGIN in QMAKE_RPATHDIR, so a bit manually
#    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$$join(QMAKE_RPATHDIR, ":")\'
#    QMAKE_RPATHDIR =
#}

SOURCES += main.cpp \
	 map-visualiser-qt.cpp \
    general/base/MoveOnTheMap.cpp \
    general/base/Map_loader.cpp \
    general/base/Map.cpp \
    client/base/MoveOnTheMap_Client.cpp \
    general/base/DebugClass.cpp

HEADERS += map-visualiser-qt.h \
    general/base/Map_loader.h \
    general/base/Map.h \
    general/base/GeneralVariable.h \
    general/base/GeneralStructures.h \
    general/base/MoveOnTheMap.h \
    client/base/MoveOnTheMap_Client.h \
    client/base/ClientStructures.h \
    general/base/DebugClass.h

RESOURCES += \
    resources.qrc
