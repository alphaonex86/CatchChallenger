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
    };
    std::map<std::pair<uint8_t,uint8_t>,PlantOnMap> plants;//position, plant id
};
}

#endif
