HEADERS  += \
    $$PWD/Api_client_real.hpp \
    $$PWD/Api_client_virtual.hpp \
    $$PWD/Api_protocol_Qt.hpp \
    $$PWD/ClientFightEngine.hpp \
    $$PWD/QtDatapackClientLoader.hpp \
    $$PWD/QtDatapackClientLoaderThread.hpp \
    $$PWD/QtDatapackChecksum.hpp \
    $$PWD/QZstdDecodeThread.hpp \
    $$PWD/QInfiniteBuffer.hpp \
    $$PWD/Audio.hpp \
    $$PWD/Language.hpp \
    $$PWD/DisplayStructures.hpp \
    $$PWD/PlatformMacro.hpp \
    $$PWD/Settings.hpp

!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h

    #Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode.
    DEFINES += USE_ALLOCA OPUS_BUILD
}


