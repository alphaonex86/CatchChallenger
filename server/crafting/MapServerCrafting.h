#ifndef CATCHCHALLENGER_MAP_SERVER_CRAFTING_H
#define CATCHCHALLENGER_MAP_SERVER_CRAFTING_H

#include <map>
#include "../../general/base/GeneralStructures.h"

namespace CatchChallenger {

class MapServerCrafting
{
public:
    class PlantOnMap
    {
    public:
        uint16_t pointOnMapDbCode;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << pointOnMapDbCode;
        }
        template <class B>
        void parse(B& buf) {
            buf >> pointOnMapDbCode;
        }
        #endif
    };
    std::map<std::pair<uint8_t,uint8_t>,PlantOnMap> plants;//position, plant id
};
}

#endif
