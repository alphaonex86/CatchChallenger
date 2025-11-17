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
    ClientWithMap();
    struct SendedStatus {
        uint32_t characterId_db;//0xffffffff if removed
        COORD_TYPE x;
        COORD_TYPE y;
        Direction direction;
    };
public:
    //max 255 size
    std::vector<SendedStatus> sendedStatus;
    CATCHCHALLENGER_TYPE_MAPID sendedMap;//see mapIndex
};
}

#endif // CATCHCHALLENGER_ClientWithMap_H
