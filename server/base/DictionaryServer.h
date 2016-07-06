#ifndef CATCHCHALLENGER_DictionaryServer_H
#define CATCHCHALLENGER_DictionaryServer_H

#include <string>
#include <vector>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/cpp11addition.h"
#include "MapServer.h"

namespace CatchChallenger {
class DictionaryServer
{
public:
    static std::vector<MapServer *> dictionary_map_database_to_internal;
    ///used only at map loading, \see BaseServer::preload_the_map()
    static std::map<std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> > dictionary_pointOnMap_item_internal_to_database;
    static std::map<std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> > dictionary_pointOnMap_plant_internal_to_database;

    struct MapAndPointItem
    {
        MapServer *map;
        uint8_t x;
        uint8_t y;
        /** \warning can have entry in database but not into datapack, deleted
         * used only to send to player the correct pos */
        uint16_t datapack_index_item;
    };
    struct MapAndPointPlant
    {
        MapServer *map;
        uint8_t x;
        uint8_t y;
        /** \warning can have entry in database but not into datapack, deleted
         * used only to send to player the correct pos */
        uint16_t datapack_index_plant;
    };
    static std::vector<MapAndPointItem> dictionary_pointOnMap_item_database_to_internal;
    static std::vector<MapAndPointPlant> dictionary_pointOnMap_plant_database_to_internal;
    static uint16_t datapack_index_temp_for_item;
    static uint16_t datapack_index_temp_for_plant;
};
}

#endif
