#ifndef CATCHCHALLENGER_ClientWithMap_H
#define CATCHCHALLENGER_ClientWithMap_H

#include <string>
#include <vector>

#include "../../general/base/GeneralStructures.hpp"
#include "../Client.hpp"
#include "DensePlayerState.hpp"

namespace CatchChallenger {

class ClientWithMap : public Client
{
public:
    ClientWithMap(const PLAYER_INDEX_FOR_CONNECTED &index_connected_player);
public:
    //max 255 size. SAME slot type as MapVisibilityAlgorithm::tempDenseBuffer
    //(layout toggled by CATCHCHALLENGER_VISIBILITY_TRUNCATED_DB_ID, see
    //DensePlayerState.hpp), so the per-tick diff is one isEqual() per slot
    //and the refresh after a send is a flat memcpy of the dense snapshot.
    std::vector<DensePlayerState> sendedStatus;
    CATCHCHALLENGER_TYPE_MAPID sendedMap;//see mapIndex

    /* min_range() (Minimize_Network) ONLY: what THIS recipient displays right
     * now, one entry by wire slot. With a view range the visible set differs
     * from one recipient to the next, so the slot numbering is per recipient:
     * the 8 bits wire slot then limits what a player SEES (mapVisibility Max)
     * and no more how many players a map can hold.
     * It is also the diff baseline, so a recipient held back by the ACK flow
     * control needs no separate copy: it is served ONE delta covering every
     * tick it missed as soon as it answers.
     * map is kept because 0x66 carries no map id: a visible player changing
     * map has to be re-inserted. Grows to the visible high-water mark of this
     * player and stops allocating (8 bytes by slot with the truncated
     * DensePlayerState layout, so 400 bytes with the default Max=50). */
    struct VisibleSlot
    {
        PLAYER_INDEX_FOR_CONNECTED player;//PLAYER_INDEX_FOR_CONNECTED_MAX: free slot
        CATCHCHALLENGER_TYPE_MAPID map;//map this slot was announced on
        DensePlayerState state;//last x/y/direction/db id sent for this slot
    };
    std::vector<VisibleSlot> visibleSlots;
};
}

#endif // CATCHCHALLENGER_ClientWithMap_H
