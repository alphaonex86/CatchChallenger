#ifndef CATCHCHALLENGER_MAP_SERVER_CRAFTING_H
#define CATCHCHALLENGER_MAP_SERVER_CRAFTING_H

#include <unordered_map>
#include "../../general/base/GeneralStructures.h"

namespace CatchChallenger {

class MapServerCrafting
{
public:
    struct PlantOnMap
    {
        uint32_t pointOnMapDbCode;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        uint8_t indexOfOnMap;//see for that's Player_private_and_public_informations
        #else
        uint8_t plant;//plant id
        uint32_t character;//player id
        uint64_t mature_at;//timestamp when is mature
        uint64_t player_owned_expire_at;//timestamp when is mature
        #endif
    };
    std::unordered_map<std::pair<uint8_t,uint8_t>,PlantOnMap,pairhash> plants;//position, plant id
};
}

#endif
