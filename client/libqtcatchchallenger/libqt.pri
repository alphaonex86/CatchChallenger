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

QT += multimedia
# internal libogg, libopus, libopusfile (replaces -lopus -logg)
include($$PWD/libogg.pri)
include($$PWD/libopus.pri)
include($$PWD/libopusfile.pri)
SOURCES += \
    $$PWD/Audio.cpp \
    $$PWD/QInfiniteBuffer.cpp
