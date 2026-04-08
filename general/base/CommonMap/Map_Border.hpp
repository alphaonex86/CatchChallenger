#ifndef CATCHCHALLENGER_Map_Border_H
#define CATCHCHALLENGER_Map_Border_H

#include "../GeneralStructures.hpp"

namespace CatchChallenger {

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
}

#endif // MAP_H
