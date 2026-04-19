DEFINES += EXTERNALLIBZSTD
DEFINES += HPS_VECTOR_RAW_BINARY
#DEFINES += CATCHCHALLENGER_NOXML
# store as file to reduce RAM need
DEFINES += CATCHCHALLENGER_DB_FILE
TEMPLATE = app
include(../../general/general.pri)
include(../catchchallenger-server.pri)
include(../catchchallenger-serverheader.pri)
include(../../general/hps/hps.pri)
include(catchchallenger-server-epoll.pri)
DEFINES += CATCHCHALLENGER_EXTRA_CHECK
#DEFINES += CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#DEFINES += SERVERSSL
#DEFINES += SERVERBENCHMARK
#DEFINES += CATCHCHALLENGER_HARDENED
#DEFINES += CATCHCHALLENGER_LIMIT_254CONNECTED
#DEFINES += CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK
#DEFINES += PROTOCOLPARSINGDEBUG

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

#just to minize size
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS += -Os -flto
    QMAKE_CFLAGS += -Os -flto
    QMAKE_LFLAGS += -Os -flto -s
}

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console

SOURCES += $$PWD/../../general/base/GeneralStructures.hpp
