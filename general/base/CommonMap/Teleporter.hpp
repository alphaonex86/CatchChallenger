#ifndef CATCHCHALLENGER_Teleporter_H
#define CATCHCHALLENGER_Teleporter_H

#include "../GeneralStructures.hpp"

namespace CatchChallenger {

struct Teleporter
{
    uint8_t source_x,source_y;
    uint8_t destination_x,destination_y;
    /* see GlobalServerData::serverPrivateVariables.flat_map_list for server
     * not pointer to just unserialize without memory allocation + searching
     * 65535 if no map set */
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    MapCondition condition;

    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << source_x << source_y;
        buf << destination_x << destination_y;
        buf << mapIndex;
        buf << condition;
    }
    template <class B>
    void parse(B& buf) {
        buf >> source_x >> source_y;
        buf >> destination_x >> destination_y;
        buf >> mapIndex;
        buf >> condition;
    }
    #endif
};
}

#endif // MAP_H
