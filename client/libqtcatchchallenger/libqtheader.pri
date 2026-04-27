HEADERS  += \
    $$PWD/Api_client_real.hpp \
    $$PWD/Api_client_virtual.hpp \
    $$PWD/Api_protocol_Qt.hpp \
    $$PWD/QtDatapackClientLoader.hpp \
    $$PWD/QtDatapackClientLoaderThread.hpp \
    $$PWD/QtDatapackChecksum.hpp \
    $$PWD/QZstdDecodeThread.hpp \
    $$PWD/QInfiniteBuffer.hpp \
    $$PWD/Audio.hpp \
    $$PWD/Language.hpp \
    $$PWD/DisplayStructures.hpp \
    $$PWD/PlatformMacro.hpp \
    $$PWD/Settings.hpp \
    $$PWD/CliClientOptions.hpp \
    $$PWD/ClientVariableAudio.hpp \
    $$PWD/FeedNews.hpp \
    $$PWD/InternetUpdater.hpp \
    $$PWD/Ultimate.hpp \
    $$PWD/LanBroadcastWatcher.hpp \
    $$PWD/LocalListener.hpp \
    $$PWD/ExtraSocket.hpp \
    $$PWD/ConnectedSocket.hpp

!contains(DEFINES, NOSINGLEPLAYER) {
    HEADERS += $$PWD/SoloDatabaseInit.hpp
}

# internal libtiled include path (replaces /usr/include/tiled/)
# already included via libtiled.pri in libqt.pri

QT += multimedia

HEADERS  += \
    $$PWD/Audio.hpp \
    $$PWD/QInfiniteBuffer.hpp

# internal libopus/libopusfile include paths (replaces /usr/include/opus/ and -lopus -lopusfile)
# already included via libopus.pri and libopusfile.pri in libqt.pri
