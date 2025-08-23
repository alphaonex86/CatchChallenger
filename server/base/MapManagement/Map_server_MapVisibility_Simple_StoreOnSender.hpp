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
    virtual ~Map_server_MapVisibility_Simple_StoreOnSender();
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
    /* list start at (max player*map id)
     * size is 8Bits if max connected player < 255 else 16Bit
     * only used in individual player diff
     * that's resolve visible player to connected player
     * 
     * DIFF
       * if ordened no problem
       * else NEED: add new player at end, remove imply:
         * move all the rest
         * or invalid the entry
           * and browsing invalid entry
           * or have status invalid entry too with index corelation
     * diff profile is more diff than insert/remove
     * diff CHOOSEN via vector:
       * insert O(n), can be resolved into O(1) if removed index list
       * remove O(n), can be resolved into O(1) if save into Client internal map index
       * diff O(n) should be fast, large hole is rare
       * while(index) if map player[index].id_db==local player[index].id_db send move, else send insert/remove
       * memory: map count*max visible player*8Bits/16Bits index of connected player
       * memory: static array: NO memory allocation, lot of not used but allocated memory, can be dynamic
     * diff BAD via unordered_map:
       * insert O(log(n))
       * remove O(log(n))
       * diff O(2n*log(n))
     * diff via map (imply index is id_db), see doc/algo/visibility/constant-time-player-visibility.png:
       * insert O(2log(n))
       * remove O(2log(n))
       * diff O(n) doble iterator to scan each (imply status player is map too to keep same order)
       * if found then send pos else send insert/remove
       * memory: id_db 32Bits map+8Bits/16Bits index of connected player*player on map
       * memory: only with dynamic memory allocation
     * diff linked list, see doc/algo/visibility/linked-list-player-visibility.png (std::forward_list can't be used, need more pointer):
       * insert O(1)
       * remove O(1)
       * diff O(n) doble iterator to scan each
         * random acess is not predictible then generate cache miss, the need of new player at end for the diff algo randomize the data
         * or reinsert at first hole + index corelantion with player status
       * if found then send pos else send insert/remove
       * memory: dynamic and O(n) players, can use index 8 or 8/16Bits and not pointer, but due to index to the next entry it duplicate the memory size (divide by /2 the data density)
       * memory: only with dynamic memory allocation
     * move BAD: have local copy: imply resolve each time to change position/orientation at EACH move + function call (and may abstraction resolv)
     * move CHOOSEN: just keep index connected to access only at move when need
     * store CHOOSEN index connected player imply read each player attribute
     * store id_db imply read each player attribute only for insert + store resolution id_db to Object
     */
    void * player_id_db_to_index;

    //mostly less remove than don't remove
    std::vector<uint16_t> to_send_remove;
    int to_send_remove_size;
    bool show;
    bool to_send_insert;
    bool send_drop_all;
    bool send_reinsert_all;//threasold, hide after 100 player, reshow below 50
    bool have_change;

    /* drop, scan at each move is more problematic than scan whole map list each 100ms
     * static uint16_t * map_to_update;
    static uint16_t map_to_update_size;*/
};
}

#endif
