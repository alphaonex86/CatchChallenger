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

//data directly related to map, then not need resolv external map
class DLL_PUBLIC BaseMap
{
public:
    //std::string map_file;-> use heap and dynamic size generate big serialiser overhead, sloud be in debug only or for cache datapack debugging
    //map not allowed be more than 127*127
    uint8_t width;
    uint8_t height;

    /* see flat_simplified_map
     * after resolution the index is position (x+y*width)*/
    ParsedLayer parsed_layer;

    std::vector<Shop> shopByIndex;
    std::unordered_map<uint8_t/*npc id*/,BotFight> botFights;//id is bot id to save what have win
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint8_t>,pairhash> botsFightTrigger;//trigger line in front of bot fight, on same to to have light load
    //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> dirt;-> stored into ParsedLayer

    // in same map
    std::unordered_map<std::pair<uint8_t,uint8_t>,uint8_t/*shop index*/,pairhash> shops;
    std::unordered_map<std::pair<uint8_t,uint8_t>,uint8_t/*npc id*/,pairhash> botsFight;//force 1 fight by x,y
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::unordered_map<std::pair<uint8_t,uint8_t>,ZONE_TYPE,pairhash> zoneCapture;//x,y bot to Map_loader::zoneNumber

    std::vector<MonsterDrops> monsterDrops;//to prevent send network packet for item when luck is 100%
};

class DLL_PUBLIC CommonMap : public BaseMap
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
            int8_t x_offset;//the max map size is 127*127, then offset have range: -127 to 127, see Map_loader::tryLoadMap() check size
        };
        struct Map_BorderContent_LeftRight
        {
            /* see GlobalServerData::serverPrivateVariables.flat_map_list for server
             * not pointer to just unserialize without memory allocation + searching
             * 65535 if no map set */
            CATCHCHALLENGER_TYPE_MAPID mapIndex;
            int8_t y_offset;//the max map size is 127*127, then offset have range: -127 to 127 see Map_loader::tryLoadMap() check size
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

    /*for very small list < 20 teleporter, it's this structure the more fast, code not ready for more than 127
     * to have map index do: flat_teleporter+teleporter_first_index */
    CATCHCHALLENGER_TYPE_TELEPORTERID teleporter_first_index;
    uint8_t teleporter_list_size;//limit to 127 max to prevent statured server

    /* on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id and find the right pointer
     * on client, MapVisualiserThread set this variable */
    CATCHCHALLENGER_TYPE_MAPID id;

    /* WHY HERE?
     * Server use ServerMap, Client use Common Map
     * Then the pointer don't have fixed size
     * Then can't just use pointer archimectic
     * then Object size save into CommonMap
     * have to be initialised toghter */
    /* WHY use unique large block:
     * Each time you call malloc the pointer should be random to improve the security
     * Each time you call malloc the space should be memset 0 to prevent get previous data
     * Each time you call malloc the allocated space can have metadata
     * Reduce the memory fragmentation
     * The space can be allocated in uncontinuous space, then you will have memory holes (more memory and less data density) linked too with block alignement */
    //size set via MapServer::mapListSize, NO holes, map valid and exists, NOT map_list.size() to never load the path
    static void * flat_map_list;
    static CATCHCHALLENGER_TYPE_MAPID flat_map_list_size;
    static size_t flat_map_object_size;//store in full length to easy multiply by index (16Bits) and have full size pointer
    static inline const void * indexToMap(const CATCHCHALLENGER_TYPE_MAPID &index);
    static inline void * indexToMapWritable(const CATCHCHALLENGER_TYPE_MAPID &index);

    static Teleporter*                          flat_teleporter;
    static CATCHCHALLENGER_TYPE_TELEPORTERID    flat_teleporter_list_size;//temp, used as size when finish

    static uint8_t *                            flat_simplified_map;
    static CATCHCHALLENGER_TYPE_MAPID           flat_simplified_map_list_size;//temp, used as size when finish
};
}

#endif // MAP_H
