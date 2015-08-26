#include "DictionaryServer.h"

using namespace CatchChallenger;

std::vector<MapServer *> DictionaryServer::dictionary_map_database_to_internal;
std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> > DictionaryServer::dictionary_pointOnMap_internal_to_database;
std::vector<DictionaryServer::MapAndPoint> DictionaryServer::dictionary_pointOnMap_database_to_internal;
