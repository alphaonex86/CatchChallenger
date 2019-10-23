#-------------------------------------------------
#
# Project created by QtCreator 2019-06-18T10:17:59
#
#-------------------------------------------------

QT       += core gui
QT += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = opusdecoder
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        MainWindow.cpp \
    qopusfile/internal.c \
    qopusfile/opusfile.c \
    qopusfile/info.c \
    qopusfile/stream.cpp \
    libogg/bitwise.c \
    libogg/framing.c

HEADERS += \
        MainWindow.h \
    qopusfile/internal.h \
    qopusfile/opusfile.h \
    libogg/ogg.h \
    libogg/os_types.h

android: {
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm64/usr/include/
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm/usr/include/
    LIBS += -L/opt/qt/5.12.4/android_arm64_v8a/lib/
    LIBS += -L/opt/qt/5.12.4/android_armv7/lib/
}

FORMS += \
        MainWindow.ui

linux:LIBS += -lopus
macx:LIBS += -lopus
win32:LIBS += -lopus
wasm:LIBS += -Lopus

RESOURCES += \
    res.qrc
