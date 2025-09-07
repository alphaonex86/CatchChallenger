DEFINES += EXTERNALLIBZSTD
TEMPLATE = app
include(../../general/general.pri)
include(../catchchallenger-server.pri)
include(../catchchallenger-serverheader.pri)
include(../../general/hps/hps.pri)
include(catchchallenger-server-epoll.pri)

#DEFINES += CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#DEFINES += SERVERSSL
#DEFINES += SERVERBENCHMARK

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

# drop all data, only keep temporary in RAM
DEFINES += CATCHCHALLENGER_DB_BLACKHOLE

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll

