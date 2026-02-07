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
    $$PWD/ClientVariableAudio.hpp \
    $$PWD/FeedNews.hpp \
    $$PWD/InternetUpdater.hpp \
    $$PWD/Ultimate.hpp \
    $$PWD/LanBroadcastWatcher.hpp \
    $$PWD/LocalListener.hpp \
    $$PWD/ExtraSocket.hpp \
    $$PWD/ConnectedSocket.hpp

INCLUDEPATH += /usr/include/tiled/

QT += multimedia

HEADERS  += \
    $$PWD/Audio.hpp \
    $$PWD/QInfiniteBuffer.hpp

    INCLUDEPATH += /usr/include/opus/
    #Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode.
    #DEFINES += USE_ALLOCA OPUS_BUILD
    LIBS += -lopus -lopusfile

equals(QT_MAJOR_VERSION, 6):QT += core5compat
