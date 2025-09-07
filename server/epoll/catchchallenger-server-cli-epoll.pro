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
#DEFINES += CATCHCHALLENGER_NOXML
#DEFINES += CATCHCHALLENGER_HARDENED

DEFINES += CATCHCHALLENGER_CLASS_ALLINONESERVER
#DEFINES += CATCHCHALLENGERSERVERDROPIFCLENT

# postgresql 9+
DEFINES += CATCHCHALLENGER_DB_POSTGRESQL
LIBS    += -lpq
SOURCES += $$PWD/db/EpollPostgresql.cpp
HEADERS += $$PWD/db/EpollPostgresql.hpp
DEFINES += CATCHCHALLENGER_DB_PREPAREDSTATEMENT
# mysql 5.7+
#LIBS    += -lmysqlclient
#DEFINES += CATCHCHALLENGER_DB_MYSQL
#SOURCES += $$PWD/db/EpollMySQL.cpp
#HEADERS += $$PWD/db/EpollMySQL.hpp
# drop all data, only keep temporary in RAM
#DEFINES += CATCHCHALLENGER_DB_BLACKHOLE
# store as file to reduce RAM need
#DEFINES += CATCHCHALLENGER_DB_FILE

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console
