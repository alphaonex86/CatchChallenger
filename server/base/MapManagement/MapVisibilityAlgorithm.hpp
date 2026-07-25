#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_H

#include "../MapServer.hpp"
#include "DensePlayerState.hpp"

#include <vector>

#ifndef CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER
#define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER 128*1024
#endif

#define CATCHCHALLENGER_DYNAMIC_MAP_LIST 1

namespace CatchChallenger {
class ClientWithMap;

class MapVisibilityAlgorithm : public MapServer
{
public:
    MapVisibilityAlgorithm();
    virtual ~MapVisibilityAlgorithm();
    void purgeBuffer();

    //void send_dropAll();
    //void send_reinsertAll();
    //void send_SYNCAll();
    //void send_insert(unsigned int &clientsToSendDataSizeNewClients,unsigned int &clientsToSendDataSizeOldClients);
    unsigned int send_reinsertAll(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,char *output, const size_t &clients_size);
    unsigned int send_reinsertAllWithFilter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,char *output,const size_t &clients_size,const size_t &skipped_id);
    // broadcast all, no filter then resend same data
    void min_CPU(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    // filter if already send, then consume CPU
    void min_network(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    // Emit ONE delta between a recipient's PRIVATE baseline
    // (ClientWithMap::sendedStatus, what it last actually received) and the
    // current snapshot. Only used for a client that has just caught up
    // after lagging -- see the flow-control note in min_network().
    void sendCoalescedDelta(ClientWithMap &clientWithMap,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,
                            const unsigned int index_client,const size_t dense_size);
    //to prevent allocate memory
    //Layout [code][size:4][count:1][entries...]. The code byte is written
    //ONCE in the constructor and never touched again, so the hot path
    //never re-emits a constant; min_network() fills size+count after its
    //diff and then copies the whole pre-composed packet with one memcpy.
    static char tempBigBufferForChanges[1+4+1+255*(1+1+1+1)];
    static char tempBigBufferForRemove[1+4+1+255];
    //Slots needing a full re-insert this tick. Only the SLOT is shared:
    //the entries themselves stay per-recipient because their bytes are
    //variable length and one recipient must have its own entry removed.
    static uint8_t tempInsertSlots[255];
    // Dense buffer of pre-composed player slots (layout — full 8-byte db
    // id, or one uint32_t per slot with
    // CATCHCHALLENGER_VISIBILITY_TRUNCATED_DB_ID — in DensePlayerState.hpp).
    // ClientWithMap::sendedStatus uses the SAME type, so the per-recipient
    // diff is one isEqual() per slot and the sent-state refresh is a flat
    // memcpy of this snapshot.
    static DensePlayerState tempDenseBuffer[255];

    /* Last state broadcast for THIS map, one entry per slot.
     *
     * Replaces the former per-recipient ClientWithMap::sendedStatus. Every
     * recipient on a map was diffing against a copy of the SAME snapshot
     * (each copy refreshed by memcpy from tempDenseBuffer), so the diff
     * result was identical for all of them apart from each one's own slot
     * -- which is the one slot it never compares. Keeping it once per map
     * makes the diff O(slots) instead of O(recipients*slots) and the state
     * O(slots) instead of O(recipients*slots) bytes.
     *
     * Grows to the map's slot high-water mark and stops allocating:
     * map_clients_id never shrinks and removeOnMap/insertOnMap recycle
     * slots through the free lists, so this reaches its final size and
     * stays there -- no per-tick allocation. */
    std::vector<DensePlayerState> previousDenseBuffer;

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
    static std::vector<MapVisibilityAlgorithm> flat_map_list;//std::vector<CommonMap *> will request 2x more memory fetch, one to get the pointer, one to get the data. With the actual pointer, just get the data, need one list for server and multiple list for client
};
}

#endif
