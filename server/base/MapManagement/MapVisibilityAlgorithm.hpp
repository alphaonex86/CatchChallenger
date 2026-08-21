#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_H

#include "../MapServer.hpp"
#include "DensePlayerState.hpp"

#include <vector>

#ifndef CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER
#define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER 128*1024
#endif

/* View range of min_range() (GameServerSettings Minimize_Network), in tiles
 * around the player. Half extents: the view is a
 * (2*VIEW_X+1) x (2*VIEW_Y+1) RECTANGLE, the shape of the client window
 * (800x600 map area / 16px tile = 50x37 tiles). Integer only, no distance,
 * no sqrt/sin/cos: 2 compares by candidate.
 * MARGIN is the hysteresis: a player is INSERTED at the view limit but only
 * REMOVED past view+margin, else somebody walking on the edge costs a full
 * insert (~25 bytes) + a remove EVERY tick instead of a 4 bytes move. */
#ifndef CATCHCHALLENGER_SERVER_MAP_VIEW_X
#define CATCHCHALLENGER_SERVER_MAP_VIEW_X 25
#endif
#ifndef CATCHCHALLENGER_SERVER_MAP_VIEW_Y
#define CATCHCHALLENGER_SERVER_MAP_VIEW_Y 19
#endif
#ifndef CATCHCHALLENGER_SERVER_MAP_VIEW_MARGIN
#define CATCHCHALLENGER_SERVER_MAP_VIEW_MARGIN 2
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
    // filter if already send, then consume CPU (GameServerSettings "balanced")
    void min_network(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    // only the players inside the view range, on this map OR on a border map
    // (GameServerSettings "network")
    void min_range(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    // one recipient view delta, see min_range()
    void sendViewDelta(ClientWithMap &recipient,const PLAYER_INDEX_FOR_CONNECTED &recipientIndex,
                       const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
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

    /* min_range() scratch, all reused by every recipient of every map:
     * - tempInsertPlayers/tempInsertSlots: the pending full inserts, the slot
     *   is 255 while it still has to be allocated (see min_range()).
     * - tempSeenSlot: "this slot is still visible this tick", reset by the
     *   same walk that indexes the slots, so no memset by recipient.
     * - tempSlotOfPlayer: sparse index connected player -> slot+1 (0: not
     *   displayed), the O(1) "do I already show him?" lookup. Only the
     *   entries of the recipient being composed are written AND cleared, so
     *   the cost stays O(visible slots) and not O(max_players). Grows to the
     *   highest connected index seen and then stops allocating. */
    static PLAYER_INDEX_FOR_CONNECTED tempInsertPlayers[255];
    static uint8_t tempSeenSlot[255];
    static std::vector<uint8_t> tempSlotOfPlayer;

    /* Map reachable from this one by its borders, with the integer
     * translation of ITS local coordinates into THIS map frame (the crossing
     * formulas are in MoveOnTheMap.hpp, the offsets are already resolved by
     * Map_loader). Built once at load by resolveNeighbours(): the 4 borders
     * plus what is reached in 2 hops and still TOUCHES this map rect, which
     * is exactly the map set the client displays around its own map
     * (MapVisualiser::rectTouch) -- a player is never announced on a map the
     * client will not load, else its insert stays forever in the client
     * delayedActions. ~8 entries by map, 6 bytes each. */
    struct NeighbourMap
    {
        CATCHCHALLENGER_TYPE_MAPID mapIndex;
        int16_t offset_x,offset_y;
    };
    std::vector<NeighbourMap> neighbours;
    //call once, after the map list AND the border offsets are resolved
    static void resolveNeighbours();

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
