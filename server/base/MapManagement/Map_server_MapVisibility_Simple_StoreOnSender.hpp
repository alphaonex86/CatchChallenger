#ifndef CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H

#include "../MapServer.hpp"

#include <vector>

#ifndef CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER
#define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER 128*1024
#endif

#define CATCHCHALLENGER_DYNAMIC_MAP_LIST 1

namespace CatchChallenger {
class ClientWithMap;

class Map_server_MapVisibility_Simple_StoreOnSender : public MapServer
{
public:
    Map_server_MapVisibility_Simple_StoreOnSender();
    virtual ~Map_server_MapVisibility_Simple_StoreOnSender();
    void purgeBuffer();

    //void send_dropAll();
    //void send_reinsertAll();
    //void send_SYNCAll();
    //void send_insert(unsigned int &clientsToSendDataSizeNewClients,unsigned int &clientsToSendDataSizeOldClients);
    unsigned int send_reinsertAll(char *output, const size_t &clients_size);
    unsigned int send_reinsertAllWithFilter(char *output,const size_t &clients_size,const size_t &skipped_id);
    // broadcast all, no filter then resend same data
    void min_CPU();
    // filter if already send, then consume CPU
    void min_network();
    //to prevent allocate memory
    static char tempBigBufferForChanges[1+4+1+255*(1+1+1+1)];
    static char tempBigBufferForRemove[1+4+1+255];

    /* WHY HERE?
     * Server use ServerMap, Client use Common Map
     * Then the pointer don't have fixed size
     * Then can't just use pointer archimectic
     * then Object size save into CommonMap
     * have to be initialised toghter */
    /* WHY use unique large block:
     * Each time you call malloc the pointer should be random to improve the security
     * Each time you call malloc the space should be memset 0 to prevent get previous data
     * Each time you call malloc the allocated space can have metadata
     * Reduce the memory fragmentation
     * The space can be allocated in uncontinuous space, then you will have memory holes (more memory and less data density) linked too with block alignement
     * Check too Binary space partition
     * https://byjus.com/gate/internal-fragmentation-in-os-notes/ or search memory fragmentation, maybe can be mitigated with 16Bits pointer
     * WHY NO MORE SIMPLE? WHY JUST NOT POINTER BY OBJECT?
     * continus space improve fragementation, loading from cache... it's server optimised version, the client will always load limited list of map
     * index imply always pass the list map and type to always be able to resolv index to data
     */
    //size set via MapServer::mapListSize, NO holes, map valid and exists, NOT map_list.size() to never load the path
    static std::vector<Map_server_MapVisibility_Simple_StoreOnSender> flat_map_list;//std::vector<CommonMap *> will request 2x more memory fetch, one to get the pointer, one to get the data. With the actual pointer, just get the data, need one list for server and multiple list for client
};
}

#endif
