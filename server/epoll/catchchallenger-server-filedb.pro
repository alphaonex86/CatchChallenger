DEFINES += EXTERNALLIBZSTD
#DEFINES += HPS_VECTOR_RAW_BINARY
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
#DEFINES += CATCHCHALLENGER_HARDENED
#DEFINES += CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK
#DEFINES += PROTOCOLPARSINGDEBUG

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console

SOURCES += $$PWD/../../general/base/GeneralStructures.hpp
