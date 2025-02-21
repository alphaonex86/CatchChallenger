#ifndef CATCHCHALLENGER_MAP_H
#define CATCHCHALLENGER_MAP_H

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "GeneralStructures.hpp"
#include "cpp11addition.hpp"
#include "lib.h"

namespace CatchChallenger {
class DLL_PUBLIC CommonMap
{
public:
    struct Map_Border
    {
        struct Map_BorderContent_TopBottom
        {
            /* see GlobalServerData::serverPrivateVariables.flat_map_list for server
             * not pointer to just unserialize without memory allocation + searching
             * 65535 if no map set */
            CATCHCHALLENGER_TYPE_MAPID mapIndex;
            int16_t x_offset;//the max map size is 255, then offset have range: -255 to 255, see Map_loader::tryLoadMap() check size
        };
        struct Map_BorderContent_LeftRight
        {
            /* see GlobalServerData::serverPrivateVariables.flat_map_list for server
             * not pointer to just unserialize without memory allocation + searching
             * 65535 if no map set */
            CATCHCHALLENGER_TYPE_MAPID mapIndex;
            int16_t y_offset;//the max map size is 255, then offset have range: -255 to 255, see Map_loader::tryLoadMap() check size
        };
        Map_BorderContent_TopBottom top;
        Map_BorderContent_TopBottom bottom;
        Map_BorderContent_LeftRight left;
        Map_BorderContent_LeftRight right;
    };
    struct Teleporter
    {
        uint8_t source_x,source_y;
        uint8_t destination_x,destination_y;
        /* see GlobalServerData::serverPrivateVariables.flat_map_list for server
         * not pointer to just unserialize without memory allocation + searching
         * 65535 if no map set */
        CATCHCHALLENGER_TYPE_MAPID mapIndex;
        MapCondition condition;
    };

    Map_Border border;

    Teleporter* teleporter;//for very small list < 20 teleporter, it's this structure the more fast, code not ready for more than 127
    uint8_t teleporter_list_size;

    //std::string map_file;-> use heap and dynamic size generate big serialiser overhead, sloud be in debug only or for cache datapack debugging
    uint8_t width;//why uint16_t if a map is not allowed be more than 255?
    uint8_t height;//why uint16_t if a map is not allowed be more than 255?
    //uint32_t group;

    /* on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id and find the right pointer
     * on client, MapVisualiserThread set this variable */
    CATCHCHALLENGER_TYPE_MAPID id;

    //the index is position (x+y*width)
    ParsedLayer parsed_layer;

    std::unordered_map<uint8_t/*npc id*/,BotFight> botFights;//id is bot id to save what have win
    std::unordered_map<std::pair<uint8_t,uint8_t>,Shop,pairhash> shops;//force 1 shop by x,y
    std::unordered_map<std::pair<uint8_t,uint8_t>,uint8_t/*npc id*/,pairhash> botsFight;//force 1 fight by x,y
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint8_t>,pairhash> botsFightTrigger;//trigger line in front of bot fight

    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::unordered_map<std::pair<uint8_t,uint8_t>,ZONE_TYPE,pairhash> zonecapture;//x,y bot to Map_loader::zoneNumber
    std::vector<MonsterDrops> monsterDrops;//to prevent send network packet for item when luck is 100%

    /* WHY NOT HERE?
     * Server use ServerMap, Client use Common Map
     * Then the pointer don't have fixed size
     * Then can't just use pointer archimectic
    static CommonMap * flat_map_list;//size set via MapServer::mapListSize, NO holes, map valid and exists, NOT map_list.size() to never load the path
    static uint32_t flat_map_size;
    */

    static void removeParsedLayer(const ParsedLayer &parsed_layer);
};
}

#endif // MAP_H
