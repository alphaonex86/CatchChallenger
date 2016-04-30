SOURCES += $$PWD/FeedNews.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/Api_client_real_base.cpp \
    $$PWD/Api_client_real_main.cpp \
    $$PWD/Api_client_real_sub.cpp \
    $$PWD/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/qt-tar-xz/QXzDecode.cpp \
    $$PWD/qt-tar-xz/QTarDecode.cpp \
    $$PWD/BlacklistPassword.cpp \
    $$PWD/SslCert.cpp

HEADERS  += $$PWD/FeedNews.h \
    $$PWD/Api_client_real.h \
    $$PWD/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/qt-tar-xz/QXzDecode.h \
    $$PWD/qt-tar-xz/QTarDecode.h \
    $$PWD/BlacklistPassword.h \
    $$PWD/SslCert.h

RESOURCES += $$PWD/resources/client-resources-multi.qrc

DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

FORMS += \
    $$PWD/SslCert.ui
