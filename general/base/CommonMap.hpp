#ifndef CATCHCHALLENGER_MAP_H
#define CATCHCHALLENGER_MAP_H

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "GeneralStructures.hpp"
#include "cpp11addition.hpp"
#include "lib.h"
#include "../tinyXML2/tinyxml2.hpp"

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

    //any where on the map, need broadcast the state
    std::vector<Industry> industries;
    //to load after industries and check data coherency
    std::vector<IndustryStatus> industriesStatus;

    std::unordered_map<uint8_t/*npc id*/,BotFight> botFights;//id is bot id to save what have win
    //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> dirt;-> stored into ParsedLayer

    // in same map
    std::unordered_map<std::pair<uint8_t,uint8_t>,Shop,pairhash> shops;//6% of the map
    std::unordered_map<std::pair<uint8_t,uint8_t>,uint8_t/*npc id*/,pairhash> botsFightTrigger;//force 1 fight by x,y
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//2% of the map
    /* ONLY SERVER, see MapServer.hpp
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::vector<std::pair<uint8_t,uint8_t>> rescue_points;*/

    std::vector<MonsterDrops> monsterDrops;//to prevent send network packet for item when luck is 100%
};

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

//the only visible map is loaded on client, all map on server
class DLL_PUBLIC CommonMap : public BaseMap
{
public:
    Map_Border border;

    /* on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id and find the right pointer
     * on client, MapVisualiserThread set this variable */
    CATCHCHALLENGER_TYPE_MAPID id;

    std::vector<Teleporter> teleporters;

    /* see flat_simplified_map
     * after resolution the index is position (x+y*width)
     * not optimal, but memory safe, simply 2 cache miss max
     */
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
    std::vector<uint8_t> flat_simplified_map;//can't be pointer, server can have unique pointer, but client need another pointer or pointer mulitple in case of multi-bots

    //extra parse function
    virtual bool parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text);
    virtual bool parseUnknownObject(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text);
    virtual bool parseUnknownBotStep(uint32_t object_x,uint32_t object_y,const tinyxml2::XMLElement *step);

};
}

#endif // MAP_H
