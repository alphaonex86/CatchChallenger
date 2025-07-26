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

class ItemOnMap
{
public:
    CATCHCHALLENGER_TYPE_ITEM item;
    bool infinite;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << item << infinite;
    }
    template <class B>
    void parse(B& buf) {
        buf >> item >> infinite;
    }
    #endif
};

//data directly related to map, then not need resolv external map
class DLL_PUBLIC BaseMap
{
public:
    //std::string map_file;-> use heap and dynamic size generate big serialiser overhead, sloud be in debug only or for cache datapack debugging
    //map not allowed be more than 127*127
    uint8_t width;
    uint8_t height;

    std::vector<MonstersCollisionValue> monstersCollisionList;

    //any where on the map
    std::vector<Industry> industries;
    //to load after industries and check data coherency
    std::vector<IndustryStatus> industriesStatus;

    std::unordered_map<uint8_t/*npc id*/,BotFight> botFights;//id is bot id to save what have win
    //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> dirt;-> stored into ParsedLayer

    // in same map
    std::unordered_map<std::pair<uint8_t,uint8_t>,Shop,pairhash> shops;//6% of the map
    std::unordered_map<std::pair<uint8_t,uint8_t>,uint8_t/*npc id*/,pairhash> botsFightTrigger;//force 1 fight by x,y
    std::unordered_map<std::pair<uint8_t,uint8_t>,ZONE_TYPE,pairhash> zoneCapture;//x,y bot to Map_loader::zoneNumber, 5% of the map
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//2% of the map
    /* ONLY SERVER, see MapServer.hpp
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::vector<std::pair<uint8_t,uint8_t>> rescue_points;*/

    std::vector<MonsterDrops> monsterDrops;//to prevent send network packet for item when luck is 100%
};

//the only visible map is loaded on client, all map on server
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

    uint32_t flat_simplified_map_first_index;

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
     * The space can be allocated in uncontinuous space, then you will have memory holes (more memory and less data density) linked too with block alignement
     * Check too Binary space partition
     * https://byjus.com/gate/internal-fragmentation-in-os-notes/ or search memory fragmentation, maybe can be mitigated with 16Bits pointer
     * WHY NO MORE SIMPLE? WHY JUST NOT POINTER BY OBJECT?
     * continus space improve fragementation, loading from cache... it's server optimised version, the client will always load limited list of map
     */
    //size set via MapServer::mapListSize, NO holes, map valid and exists, NOT map_list.size() to never load the path
    static void * flat_map_list;//std::vector<CommonMap *> will request 2x more memory fetch, one to get the pointer, one to get the data. With the actual pointer, just get the data
    static CATCHCHALLENGER_TYPE_MAPID flat_map_list_count;
    static size_t flat_map_object_size;//store in full length to easy multiply by index (16Bits) and have full size pointer

    static Teleporter*                          flat_teleporter;
    static CATCHCHALLENGER_TYPE_TELEPORTERID    flat_teleporter_list_size;//temp, used as size when finish

    /* see flat_simplified_map
     * after resolution the index is position (x+y*width)*/
    /* 0 walkable: index = 0 for monster is used into cave
     * 254 not walkable
     * 253 ParsedLayerLedges_LedgesBottom
     * 252 ParsedLayerLedges_LedgesTop
     * 251 ParsedLayerLedges_LedgesRight
     * 250 ParsedLayerLedges_LedgesLeft
     * 249 dirt
     * 200 - 248 reserved
     * 0 cave def
     * 1-199 monster def and condition */
    static uint8_t *                            flat_simplified_map;
    static CATCHCHALLENGER_TYPE_MAPID           flat_simplified_map_list_size;//temp, used as size when finish
    
    
    //facility function
    static inline const void * indexToMap(const CATCHCHALLENGER_TYPE_MAPID &index);
    static inline void * indexToMapWritable(const CATCHCHALLENGER_TYPE_MAPID &index);
};
}

#endif // MAP_H
