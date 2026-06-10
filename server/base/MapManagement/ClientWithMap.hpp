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
};
}

#endif // CATCHCHALLENGER_ClientWithMap_H
