#ifndef CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H

#include "../MapServer.h"
#include "MapVisibilityAlgorithm_Simple_StoreOnSender.h"

#include <vector>

#ifndef CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER
#define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER 128*1024
#endif

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnSender;

class Map_server_MapVisibility_Simple_StoreOnSender : public MapServer
{
public:
    Map_server_MapVisibility_Simple_StoreOnSender();
    void purgeBuffer();
    std::vector<MapVisibilityAlgorithm_Simple_StoreOnSender *> clients;//manipulated by thread of ClientMapManagement()

    //mostly less remove than don't remove
    std::vector<uint16_t> to_send_remove;
    int to_send_remove_size;
    bool show;
    bool to_send_insert;
    bool send_drop_all;
    bool send_reinsert_all;
    bool have_change;

    static MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataNewClients[65535];
    static MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataOldClients[65535];
    static Map_server_MapVisibility_Simple_StoreOnSender ** map_to_update;
    static uint32_t map_to_update_size;
};
}

#endif
