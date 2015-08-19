#ifndef CATCHCHALLENGER_MAP_SERVER_CRAFTING_H
#define CATCHCHALLENGER_MAP_SERVER_CRAFTING_H

#include <unordered_map>
#include "../general/base/GeneralStructures.h"

namespace CatchChallenger {

class MapServerCrafting
{
public:
    struct PlantOnMap
    {
        quint32 pointOnMapDbCode;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        quint8 indexOfOnMap;//see for that's Player_private_and_public_informations
        #else
        quint8 plant;//plant id
        quint32 character;//player id
        quint64 mature_at;//timestamp when is mature
        quint64 player_owned_expire_at;//timestamp when is mature
        #endif
    };
    std::unordered_map<std::pair<quint8,quint8>,PlantOnMap,pairhash> plants;//position, plant id
};
}

#endif
