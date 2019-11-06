include(../libcatchchallenger/lib.pri)
include(../qt/tiled/tiled.pri)

QT       += gui network core widgets opengl

DEFINES += CATCHCHALLENGER_CLIENT

wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
#android: DEFINES += CATCHCHALLENGER_NOAUDIO
# see the file ClientVariableAudio.h
#DEFINES += CATCHCHALLENGER_NOAUDIO
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
linux:LIBS += -lopus
macx:LIBS += -lopus
win32:LIBS += -lopus
wasm:LIBS += -Lopus
SOURCES += \
    $$PWD/../libogg/bitwise.c \
    $$PWD/../libogg/framing.c \
    $$PWD/../opusfile/info.c \
    $$PWD/../opusfile/internal.c \
    $$PWD/../opusfile/opusfile.c \
    $$PWD/../opusfile/stream.c \
    $$PWD/../qt/Audio.cpp \
    $$PWD/../qt/QInfiniteBuffer.cpp

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h \
    $$PWD/../qt/Audio.h \
    $$PWD/../qt/QInfiniteBuffer.h
}
wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
}
else
{
#    DEFINES += NOWEBSOCKET
}
android: {
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm64/usr/include/
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm/usr/include/
    LIBS += -L/opt/qt/5.12.4/android_arm64_v8a/lib/
    LIBS += -L/opt/qt/5.12.4/android_armv7/lib/
    QT += androidextras
    QMAKE_CFLAGS += -g
    QMAKE_CXXFLAGS += -g
}
!contains(DEFINES, NOWEBSOCKET) {
    QT += websockets
}

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp

HEADERS  += \
    $$PWD/../../general/base/tinyXML2/tinyxml2.h

RESOURCES += $$PWD/../resources/client-resources-multi.qrc
DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/../resources/resources-windows.rc
ICON = $$PWD/../resources/client.icns
macx:INCLUDEPATH += /Users/user/Desktop/VLC.app/Contents/MacOS/include/
macx:LIBS += -L/Users/user/Desktop/VLC.app/Contents/MacOS/lib/

RESOURCES += $$PWD/../resources/client-resources.qrc \
    $$PWD/../qt/crafting/resources/client-resources-plant.qrc \
    $$PWD/../qt/fight/resources/client-resources-fight.qrc

TRANSLATIONS    = $$PWD/../resources/languages/en/translation.ts \
    $$PWD/../languages/fr/translation.ts

#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-server-qt.pri)
QT       += sql
SOURCES += $$PWD/../qt/solo/InternalServer.cpp \
    $$PWD/../qt/solo/SoloWindow.cpp
HEADERS  += $$PWD/../qt/solo/InternalServer.h \
    $$PWD/../qt/solo/SoloWindow.h
}
