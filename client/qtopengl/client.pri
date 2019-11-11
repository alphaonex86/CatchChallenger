include(../libcatchchallenger/lib.pri)
include(../qt/tiled/tiled.pri)

QT       += gui network core widgets opengl xml

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
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp \
    $$PWD/LanguagesSelect.cpp \
    $$PWD/CCTitle.cpp \
    $$PWD/ConnexionManager.cpp \
    $$PWD/LoadingScreen.cpp \
    $$PWD/CCWidget.cpp \
    $$PWD/CustomButton.cpp \
    $$PWD/MainScreen.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/ScreenTransition.cpp \
    $$PWD/Multi.cpp \
    $$PWD/OptionsDialog.cpp \
    $$PWD/CCBackground.cpp \
    $$PWD/CCprogressbar.cpp \
    $$PWD/../qt/Api_client_real.cpp \
    $$PWD/../qt/Api_client_real_base.cpp \
    $$PWD/../qt/Api_client_real_main.cpp \
    $$PWD/../qt/Api_client_real_sub.cpp \
    $$PWD/../qt/Api_client_virtual.cpp \
    $$PWD/../qt/Api_protocol_Qt.cpp \
    $$PWD/../qt/QtDatapackChecksum.cpp \
    $$PWD/../qt/QtDatapackClientLoader.cpp \
    $$PWD/../qt/QZstdDecodeThread.cpp \
    $$PWD/../qt/InternetUpdater.cpp \
    $$PWD/../qt/FeedNews.cpp \
    $$PWD/../qt/ConnectedSocket.cpp \
    $$PWD/../qt/GameLoader.cpp \
    $$PWD/../tarcompressed/TarDecode.cpp \
    $$PWD/../tarcompressed/ZstdDecode.cpp \
    $$PWD/../qt/fight/interface/ClientFightEngine.cpp \
    $$PWD/../qt/fight/interface/DatapackClientLoaderFight.cpp \
    $$PWD/../qt/crafting/interface/DatapackClientLoaderCrafting.cpp \
    $$PWD/../qt/Ultimate.cpp \
    $$PWD/../qt/GameLoaderThread.cpp \
    $$PWD/../qt/ExtraSocket.cpp \
    $$PWD/../qt/Settings.cpp

HEADERS  += \
    $$PWD/../../general/base/tinyXML2/tinyxml2.h \
    $$PWD/LanguagesSelect.h \
    $$PWD/CCprogressbar.h \
    $$PWD/ClientStructures.h \
    $$PWD/CCTitle.h \
    $$PWD/ConnexionManager.h \
    $$PWD/CCWidget.h \
    $$PWD/CustomButton.h \
    $$PWD/LoadingScreen.h \
    $$PWD/DisplayStructures.h \
    $$PWD/MainScreen.h \
    $$PWD/Map_client.h \
    $$PWD/OptionsDialog.h \
    $$PWD/ScreenTransition.h \
    $$PWD/Multi.h \
    $$PWD/CCBackground.h \
    $$PWD/../qt/Api_client_real.h \
    $$PWD/../qt/Api_client_virtual.h \
    $$PWD/../qt/Api_protocol_Qt.h \
    $$PWD/../qt/QtDatapackChecksum.h \
    $$PWD/../qt/QtDatapackClientLoader.h \
    $$PWD/../qt/QZstdDecodeThread.h \
    $$PWD/../qt/InternetUpdater.h \
    $$PWD/../qt/FeedNews.h \
    $$PWD/../qt/ConnectedSocket.h \
    $$PWD/../qt/GameLoader.h \
    $$PWD/../tarcompressed/TarDecode.h \
    $$PWD/../tarcompressed/ZstdDecode.h \
    $$PWD/../qt/fight/interface/ClientFightEngine.h \
    $$PWD/../qt/Ultimate.h \
    $$PWD/../qt/GameLoaderThread.h \
    $$PWD/../qt/ExtraSocket.h \
    $$PWD/../qt/Settings.h

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

DISTFILES += \
    $$PWD/client.pri
