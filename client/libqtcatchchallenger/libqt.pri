DEFINES += CATCHCHALLENGER_CLIENT

LIBS += -ltiled

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

DEFINES += CATCHCHALLENGERLIB

QT += multimedia
LIBS += -lopus -logg
SOURCES += \
    $$PWD/Audio.cpp \
    $$PWD/QInfiniteBuffer.cpp
