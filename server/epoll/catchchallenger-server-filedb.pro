DEFINES += EXTERNALLIBZSTD
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
#DEFINES += CATCHCHALLENGER_NOXML
#DEFINES += CATCHCHALLENGER_HARDENED
#DEFINES += CATCHCHALLENGER_LIMIT_254CONNECTED

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

# store as file to reduce RAM need
DEFINES += CATCHCHALLENGER_DB_FILE

CONFIG += c++17

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console

SOURCES += $$PWD/../../general/base/GeneralStructures.hpp
