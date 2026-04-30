DEFINES += CATCHCHALLENGER_CLIENT

# internal libtiled (replaces -ltiled)
include($$PWD/libtiled.pri)

SOURCES += \
    $$PWD/Api_client_real_base.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/Api_client_real_main.cpp \
    $$PWD/Api_client_real_sub.cpp \
    $$PWD/Api_client_virtual.cpp \
    $$PWD/Api_protocol_Qt.cpp \
    $$PWD/QtDatapackClientLoader.cpp \
    $$PWD/QtDatapackClientLoaderThread.cpp \
    $$PWD/QtDatapackChecksum.cpp \
    $$PWD/QZstdDecodeThread.cpp \
    $$PWD/Language.cpp \
    $$PWD/Settings.cpp \
    $$PWD/CliClientOptions.cpp \
    $$PWD/FeedNews.cpp \
    $$PWD/InternetUpdater.cpp \
    $$PWD/Ultimate.cpp \
    $$PWD/LanBroadcastWatcher.cpp \
    $$PWD/LocalListener.cpp \
    $$PWD/ExtraSocket.cpp \
    $$PWD/ConnectedSocket.cpp

!contains(DEFINES, NOSINGLEPLAYER) {
    SOURCES += $$PWD/SoloDatabaseInit.cpp
}

DEFINES += CATCHCHALLENGERLIB

#match qtopengl/client.pri: opt out of QtMultimedia when CATCHCHALLENGER_NOAUDIO
#is set (Qt-for-Android in this workspace doesn't ship the Multimedia module).
#Audio.cpp #include <QAudioSink> unconditionally so we must also drop it from
#SOURCES when audio is off, otherwise the compile fails with
#"QAudioSink: No such file or directory".
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
    QT += multimedia
    SOURCES += $$PWD/Audio.cpp
}
# internal libogg, libopus, libopusfile (replaces -lopus -logg)
include($$PWD/libogg.pri)
include($$PWD/libopus.pri)
include($$PWD/libopusfile.pri)
SOURCES += \
    $$PWD/QInfiniteBuffer.cpp
