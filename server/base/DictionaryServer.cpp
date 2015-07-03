#include "DictionaryServer.h"

using namespace CatchChallenger;

QList<MapServer *> DictionaryServer::dictionary_map_database_to_internal;
QHash<QString,QHash<QPair<quint8/*x*/,quint8/*y*/>,quint16/*db code*/> > DictionaryServer::dictionary_pointOnMap_internal_to_database;
QList<DictionaryServer::MapAndPoint> DictionaryServer::dictionary_pointOnMap_database_to_internal;
