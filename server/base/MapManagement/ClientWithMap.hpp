#ifndef CATCHCHALLENGER_ClientWithMap_H
#define CATCHCHALLENGER_ClientWithMap_H

#include <string>
#include <vector>

#include "../../general/base/GeneralStructures.hpp"
#include "../Client.hpp"

namespace CatchChallenger {

class ClientWithMap : public Client
{
public:
    ClientWithMap(const PLAYER_INDEX_FOR_CONNECTED &index_connected_player);
    struct SendedStatus {
        uint32_t characterId_db;//0xffffffff if removed
        //x, y and direction packed into one word: x | (y<<8) | (direction<<16),
        //high byte always 0. Lets the per-tick diff compare position+direction
        //with a single 32-bit compare instead of 3 byte compares, and makes the
        //struct exactly 8 bytes with NO padding (every byte defined) so a whole
        //SendedStatus can be compared/copied as one 64-bit word.
        uint32_t xyd;
    };
public:
    //max 255 size
    std::vector<SendedStatus> sendedStatus;
    CATCHCHALLENGER_TYPE_MAPID sendedMap;//see mapIndex
};
}

#endif // CATCHCHALLENGER_ClientWithMap_H
