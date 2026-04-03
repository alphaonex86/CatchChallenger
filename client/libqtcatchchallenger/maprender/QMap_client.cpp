#include "QMap_client.hpp"

using namespace CatchChallenger;

std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,QMap_client *> QMap_client::all_map;
std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,QMap_client *> QMap_client::old_all_map;
std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,uint8_t> QMap_client::old_all_map_time_count;


