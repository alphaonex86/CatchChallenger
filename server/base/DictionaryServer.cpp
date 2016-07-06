#include "DictionaryServer.h"

using namespace CatchChallenger;

std::vector<MapServer *> DictionaryServer::dictionary_map_database_to_internal;
std::map<std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> > DictionaryServer::dictionary_pointOnMap_item_internal_to_database;
std::map<std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> > DictionaryServer::dictionary_pointOnMap_plant_internal_to_database;
std::vector<DictionaryServer::MapAndPointItem> DictionaryServer::dictionary_pointOnMap_item_database_to_internal;
std::vector<DictionaryServer::MapAndPointPlant> DictionaryServer::dictionary_pointOnMap_plant_database_to_internal;
uint16_t DictionaryServer::datapack_index_temp_for_item=0;
uint16_t DictionaryServer::datapack_index_temp_for_plant=0;
