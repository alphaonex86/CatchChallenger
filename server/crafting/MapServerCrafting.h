#ifndef CATCHCHALLENGER_MAP_SERVER_CRAFTING_H
#define CATCHCHALLENGER_MAP_SERVER_CRAFTING_H

#include <map>
#include "../../general/base/GeneralStructures.h"

namespace CatchChallenger {

class MapServerCrafting
{
public:
    struct PlantOnMap
    {
        uint16_t pointOnMapDbCode;
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        uint8_t plant;//plant id
        uint32_t character;//player id
        uint64_t mature_at;//timestamp when is mature
        uint64_t player_owned_expire_at;//timestamp when is mature
        #endif
    };
    std::map<std::pair<uint8_t,uint8_t>,PlantOnMap> plants;//position, plant id
};
}

#endif
