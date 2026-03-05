#ifndef CATCHCHALLENGER_MAP_CLIENT_H
#define CATCHCHALLENGER_MAP_CLIENT_H

#include "../../general/base/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/GeneralType.hpp"

namespace CatchChallenger {
class Map_client : public CommonMap
{
public:
    /*why?std::vector<CommonMap *> near_map;//only the border (left, right, top, bottom) AND them self
    std::vector<CommonMap *> linked_map;//not only the border, with tp, door, ...*/

    /*via sync loadingstd::vector<Map_semi_teleport> teleport_semi;
    std::vector<std::string> teleport_condition_texts;*/
    //std::vector<Map_to_send::Map_Point> rescue_points;
    //std::vector<Map_to_send::Map_Point> bot_spawn_points;

    Map_semi_border border_semi;
    uint32_t group;

    Map_client();
};
}

#endif // MAP_SERVER_H
