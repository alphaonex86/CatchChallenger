#ifndef CATCHCHALLENGER_BaseMap_H
#define CATCHCHALLENGER_BaseMap_H

#include "../GeneralStructures.hpp"
#include "ItemOnMap.hpp"
#include "../../general/base/cpp11addition.hpp"

namespace CatchChallenger {

//data directly related to map, then not need resolv external map
class BaseMap
{
public:
    //std::string map_file;-> use heap and dynamic size generate big serialiser overhead, sloud be in debug only or for cache datapack debugging
    //map not allowed be more than 127*127
    uint8_t width;
    uint8_t height;

    std::vector<MonstersCollisionValue> zones;

    //use intermediary index to keep state even if is moved
    //come from datapack
    std::vector<Industry> industries;

    catchchallenger_datapack_map<uint8_t/*npc id*/,BotFight> botFights;//id is bot id to save what have win
    //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> dirt;-> stored into ParsedLayer

    // in same map
    std::map<std::pair<uint8_t,uint8_t>,uint8_t> industries_pos;
    catchchallenger_datapack_map<std::pair<uint8_t,uint8_t>,Shop> shops;//6% of the map
    catchchallenger_datapack_map<std::pair<uint8_t,uint8_t>,uint8_t/*npc id*/> botsFightTrigger;//force 1 fight by x,y, never cross line of fight (this reduce lot of the memory allocation and presure, each std::vector is 1 pointer)
    catchchallenger_datapack_map<std::pair<uint8_t,uint8_t>,ItemOnMap> items;//2% of the map
    /* ONLY SERVER, see MapServer.hpp
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::vector<std::pair<uint8_t,uint8_t>> rescue_points;*/

    std::vector<MonsterDrops> monsterDrops;//to prevent send network packet for item when luck is 100%
};
}

#endif // MAP_H
