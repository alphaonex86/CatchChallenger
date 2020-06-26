#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -std=c++0x -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -std=c++0x -fno-rtti"

QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g -fno-rtti"

QT       -= gui widgets network sql xml
QT       -= xml core

DEFINES += TIXML_USE_STL
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += CATCHCHALLENGER_CLASS_LOGIN

# postgresql 9+
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL
LIBS    += -lpq
# mysql 5.5+
#LIBS    += -lmysqlclient
#DEFINES += CATCHCHALLENGER_DB_MYSQL

CONFIG += c++11

TARGET = catchchallenger-server-login
CONFIG   += console

TEMPLATE = app

SOURCES += \
    ../../general/libzstd/lib/common/debug.c \
    ../../general/libzstd/lib/common/entropy_common.c \
    ../../general/libzstd/lib/common/error_private.c \
    ../../general/libzstd/lib/common/fse_decompress.c \
    ../../general/libzstd/lib/common/pool.c \
    ../../general/libzstd/lib/common/threading.c \
    ../../general/libzstd/lib/common/xxbhash.c \
    ../../general/libzstd/lib/common/zstd_common.c \
    ../../general/libzstd/lib/compress/fse_compress.c \
    ../../general/libzstd/lib/compress/hist.c \
    ../../general/libzstd/lib/compress/huf_compress.c \
    ../../general/libzstd/lib/compress/zstd_compress.c \
    ../../general/libzstd/lib/compress/zstd_compress_literals.c \
    ../../general/libzstd/lib/compress/zstd_compress_sequences.c \
    ../../general/libzstd/lib/compress/zstd_compress_superblock.c \
    ../../general/libzstd/lib/compress/zstd_double_fast.c \
    ../../general/libzstd/lib/compress/zstd_fast.c \
    ../../general/libzstd/lib/compress/zstd_lazy.c \
    ../../general/libzstd/lib/compress/zstd_ldm.c \
    ../../general/libzstd/lib/compress/zstd_opt.c \
    ../../general/libzstd/lib/compress/zstdmt_compress.c \
    ../../general/libzstd/lib/decompress/huf_decompress.c \
    ../../general/libzstd/lib/decompress/zstd_ddict.c \
    ../../general/libzstd/lib/decompress/zstd_decompress.c \
    ../../general/libzstd/lib/decompress/zstd_decompress_block.c \
    ../../general/sha224/sha224.cpp \
    main-epoll-login-slave.cpp \
    EpollClientLoginSlave.cpp \
    EpollServerLoginSlave.cpp \
    EpollClientLoginSlaveStaticVar.cpp \
    EpollClientLoginSlaveHeavyLoad.cpp \
    LinkToMaster.cpp \
    LinkToMasterStaticVar.cpp \
    LinkToMasterProtocolParsing.cpp \
    LinkToMasterProtocolParsingMessage.cpp \
    LinkToMasterProtocolParsingReply.cpp \
    EpollClientLoginSlaveProtocolParsing.cpp \
    EpollClientLoginSlaveWrite.cpp \
    CharactersGroupForLogin.cpp \
    CharactersGroupClient.cpp \
    LinkToGameServerStaticVar.cpp \
    LinkToGameServerProtocolParsing.cpp \
    LinkToGameServer.cpp \
    TimerDdos.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../epoll/EpollSslClient.cpp \
    ../epoll/db/EpollPostgresql.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp \
    ../epoll/EpollTimer.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../base/DatabaseBase.cpp \
    ../base/PreparedDBQueryLogin.cpp \
    ../base/PreparedDBQueryCommon.cpp \
    ../base/PreparedDBQueryServer.cpp \
    ../base/PreparedStatementUnit.cpp \
    ../base/BaseServerLogin.cpp \
    ../base/SqlFunction.cpp \
    ../base/DictionaryLogin.cpp \
    ../base/TinyXMLSettings.cpp \
    ../base/DatabaseFunction.cpp \
    ../base/StringWithReplacement.cpp \
    TimerDetectTimeout.cpp \
    ../../general/base/Version.cpp

HEADERS += \
    ../../general/libzstd/lib/common/bitstream.h \
    ../../general/libzstd/lib/common/compiler.h \
    ../../general/libzstd/lib/common/cpu.h \
    ../../general/libzstd/lib/common/debug.h \
    ../../general/libzstd/lib/common/error_private.h \
    ../../general/libzstd/lib/common/fse.h \
    ../../general/libzstd/lib/common/huf.h \
    ../../general/libzstd/lib/common/mem.h \
    ../../general/libzstd/lib/common/pool.h \
    ../../general/libzstd/lib/common/threading.h \
    ../../general/libzstd/lib/common/xxhash.h \
    ../../general/libzstd/lib/common/zstd_errors.h \
    ../../general/libzstd/lib/common/zstd_internal.h \
    ../../general/libzstd/lib/compress/hist.h \
    ../../general/libzstd/lib/compress/zstd_compress_internal.h \
    ../../general/libzstd/lib/compress/zstd_compress_literals.h \
    ../../general/libzstd/lib/compress/zstd_compress_sequences.h \
    ../../general/libzstd/lib/compress/zstd_compress_superblock.h \
    ../../general/libzstd/lib/compress/zstd_cwksp.h \
    ../../general/libzstd/lib/compress/zstd_double_fast.h \
    ../../general/libzstd/lib/compress/zstd_fast.h \
    ../../general/libzstd/lib/compress/zstd_lazy.h \
    ../../general/libzstd/lib/compress/zstd_ldm.h \
    ../../general/libzstd/lib/compress/zstd_opt.h \
    ../../general/libzstd/lib/compress/zstdmt_compress.h \
    ../../general/libzstd/lib/decompress/zstd_ddict.h \
    ../../general/libzstd/lib/decompress/zstd_decompress_block.h \
    ../../general/libzstd/lib/decompress/zstd_decompress_internal.h \
    ../../general/sha224/sha224.hpp \
    EpollClientLoginSlave.h \
    EpollServerLoginSlave.h \
    LinkToMaster.h \
    CharactersGroupForLogin.h \
    LinkToGameServer.h \
    TimerDdos.h \
    VariableLoginServer.h \
    ../epoll/Epoll.h \
    ../epoll/EpollGenericSslServer.h \
    ../epoll/EpollGenericServer.h \
    ../epoll/EpollClient.h \
    ../epoll/EpollSocket.h \
    ../epoll/EpollSslClient.h \
    ../epoll/db/EpollPostgresql.h \
    ../epoll/EpollClientToServer.h \
    ../epoll/EpollTimer.h \
    ../epoll/EpollSslClientToServer.h \
    ../epoll/BaseClassSwitch.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/GeneralVariable.h \
    ../../general/base/CommonSettingsCommon.h \
    ../../general/base/cpp11addition.h \
    ../base/DatabaseBase.h \
    ../base/PreparedDBQuery.h \
    ../base/PreparedStatementUnit.h \
    ../base/BaseServerLogin.h \
    ../base/SqlFunction.h \
    ../base/DictionaryLogin.h \
    ../base/TinyXMLSettings.h \
    ../base/DatabaseFunction.h \
    ../base/StringWithReplacement.h \
    ../VariableServer.h \
    TimerDetectTimeout.h

DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/base/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp
