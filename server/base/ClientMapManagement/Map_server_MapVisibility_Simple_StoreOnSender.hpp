#ifndef CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H

#include "../MapServer.hpp"

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

    void send_dropAll();
    void send_reinsertAll();
    void send_pingAll();
    void send_insert(unsigned int &clientsToSendDataSizeNewClients,unsigned int &clientsToSendDataSizeOldClients);
    void send_insert_exclude();
    void send_remove(unsigned int &clientsToSendDataSizeOldClients);
    void send_samllreinsert_reemit(unsigned int &clientsToSendDataSizeOldClients);
    void send_insertcompose_header(char *buffer,int &posOutput);
    void send_insertcompose_map(char *buffer,int &posOutput);
    void send_insertcompose_playercount(char *buffer,int &posOutput);
    void send_insertcompose_content_and_send(char *buffer,int &posOutput);

    //mostly less remove than don't remove
    std::vector<uint16_t> to_send_remove;
    int to_send_remove_size;
    bool show;
    bool to_send_insert;
    bool send_drop_all;
    bool send_reinsert_all;//threasold, hide after 100 player, reshow below 50
    bool have_change;

    //maybe need have to be optimized
    static MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataNewClients[65535];//temp space to work
    static MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataOldClients[65535];//temp space to work
    static Map_server_MapVisibility_Simple_StoreOnSender ** map_to_update;
    static uint32_t map_to_update_size;
};
}

#endif
