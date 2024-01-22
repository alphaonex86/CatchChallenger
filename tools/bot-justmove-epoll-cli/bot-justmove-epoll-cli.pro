DEFINES += CATCHCHALLENGER_NOAUDIO

QT       -= core gui xml network sql
QT -= widgets websockets
LIBS += -lcurl

TARGET = bot-actions-cli
TEMPLATE = app

DEFINES += BOTACTIONS CATCHCHALLENGER_BOT CATCHCHALLENGER_CLASS_BOT CATCHCHALLENGER_ABORTIFERROR CATCHCHALLENGER_DB_BLACKHOLE
DEFINES += LIBIMPORT
linux:QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"
precompile_header:!isEmpty(PRECOMPILED_HEADER) {
DEFINES += USING_PCH
}
#linux:LIBS += -fuse-ld=mold
#LIBS += -L../../client/build-catchchallengerclient-Desktop-Debug/ -lcatchchallengerclient

include(../../client/libcatchchallenger/libheader.pri)
include(../../client/libcatchchallenger/lib.pri)
include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)

HEADERS += \
    ../../server/epoll/BaseClassSwitch.hpp \
    ../../server/epoll/Epoll.hpp \
    ../../server/epoll/EpollClient.hpp \
    ../../server/epoll/EpollSocket.hpp \
    ../../server/epoll/EpollTimer.hpp \
    ../../server/epoll/EpollUnixSocketServer.hpp \
    Bot.hpp \
    DatapackDownloaderBase.hpp \
    DatapackDownloaderMainSub.hpp \
    TimerMove.hpp

SOURCES += \
    ../../server/epoll/Epoll.cpp \
    ../../server/epoll/EpollClient.cpp \
    ../../server/epoll/EpollSocket.cpp \
    ../../server/epoll/EpollTimer.cpp \
    ../../server/epoll/EpollUnixSocketServer.cpp \
    Bot.cpp \
    DatapackDownloaderBase.cpp \
    DatapackDownloaderMainSub.cpp \
    DatapackDownloader_main.cpp \
    DatapackDownloader_sub.cpp \
    TimerMove.cpp \
    main-epoll-bot.cpp
