#include "DictionaryServer.h"

using namespace CatchChallenger;

QList<MapServer *> DictionaryServer::dictionary_map_database_to_internal;
QHash<QString,QHash<QPair<quint8/*x*/,quint8/*y*/>,quint16/*db code*/> > DictionaryServer::dictionary_itemOnMap_internal_to_database;
QList<quint8> DictionaryServer::dictionary_itemOnMap_database_to_internal;
