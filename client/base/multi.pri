SOURCES += $$PWD/RssNews.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/qt-tar-xz/QXzDecode.cpp \
    $$PWD/qt-tar-xz/QTarDecode.cpp \
    $$PWD/qt-tar-xz/xz_crc32.c \
    $$PWD/qt-tar-xz/xz_dec_stream.c \
    $$PWD/qt-tar-xz/xz_dec_lzma2.c \
    $$PWD/qt-tar-xz/xz_dec_bcj.c \
    $$PWD/SslCert.cpp

HEADERS  += $$PWD/RssNews.h \
    $$PWD/Api_client_real.h \
    $$PWD/qt-tar-xz/xz.h \
    $$PWD/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/qt-tar-xz/QXzDecode.h \
    $$PWD/qt-tar-xz/QTarDecode.h \
    $$PWD/SslCert.h

RESOURCES += $$PWD/resources/client-resources-multi.qrc

DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

FORMS += \
    $$PWD/SslCert.ui
