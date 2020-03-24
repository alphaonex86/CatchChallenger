#ifndef CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_WITHBORDER_STOREONSENDER_H
#define CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_WITHBORDER_STOREONSENDER_H

#include "../MapServer.hpp"
#include "MapVisibilityAlgorithm_WithBorder_StoreOnSender.hpp"

#include <set>
#include <vector>

namespace CatchChallenger {
class MapVisibilityAlgorithm_WithBorder_StoreOnSender;
class Map_server_MapVisibility_WithBorder_StoreOnSender : public MapServer
{
public:
    Map_server_MapVisibility_WithBorder_StoreOnSender();
    void purgeBuffer();
    std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> clients;//manipulated by thread of ClientMapManagement()

    std::unordered_set<SIMPLIFIED_PLAYER_ID_TYPE>     to_send_remove;
    uint16_t clientsOnBorder;
    bool showWithBorder;
    bool show;
};
}

#endif
