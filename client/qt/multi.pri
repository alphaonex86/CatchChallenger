SOURCES += $$PWD/FeedNews.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/Api_client_real_base.cpp \
    $$PWD/Api_client_real_main.cpp \
    $$PWD/Api_client_real_sub.cpp \
    $$PWD/QZstdDecodeThread.cpp \
    $$PWD/BlacklistPassword.cpp \
    $$PWD/SslCert.cpp \
    $$PWD/../tarcompressed/TarDecode.cpp \
    $$PWD/../tarcompressed/ZstdDecode.cpp

HEADERS  += $$PWD/FeedNews.h \
    $$PWD/Api_client_real.h \
    $$PWD/QZstdDecodeThread.h \
    $$PWD/BlacklistPassword.h \
    $$PWD/SslCert.h \
    $$PWD/../tarcompressed/TarDecode.h \
    $$PWD/../tarcompressed/ZstdDecode.h

RESOURCES += $$PWD/../resources/client-resources-multi.qrc

DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

FORMS += \
    $$PWD/SslCert.ui
