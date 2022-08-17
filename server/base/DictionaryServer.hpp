#ifndef CATCHCHALLENGER_DictionaryServer_H
#define CATCHCHALLENGER_DictionaryServer_H

#include <string>
#include <vector>
#include "MapServer.hpp"

namespace CatchChallenger {
class DictionaryServer
{
public:
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
        uint16_t datapack_index_item;//just the number of insert item, index++, not linked with DB id
    };
    struct MapAndPointPlant
    {
        MapServer *map;
        uint8_t x;
        uint8_t y;
        /** \warning can have entry in database but not into datapack, deleted
         * used only to send to player the correct pos */
        uint16_t datapack_index_plant;//just the number of insert item, index++, not linked with DB id
    };
    //used at runtime (at player loading)
    static std::vector<MapServer *> dictionary_map_database_to_internal;
    static std::vector<MapAndPointItem> dictionary_pointOnMap_item_database_to_internal;//start with null item, not map to improvement performance due to high density
    static std::vector<MapAndPointPlant> dictionary_pointOnMap_plant_database_to_internal;//start with null item, not map to improvement performance due to high density
};
}

#endif
