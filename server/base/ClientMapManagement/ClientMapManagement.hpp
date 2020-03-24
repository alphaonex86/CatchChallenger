#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENT_H

#include <string>

#include "../ServerStructures.hpp"
#include "../EventThreader.hpp"
#include "../../VariableServer.hpp"
#include "MapBasicMove.hpp"

namespace CatchChallenger {
#ifdef EPOLLCATCHCHALLENGERSERVER
class Client;
#endif
class Map_custom;

class ClientMapManagement : public MapBasicMove
{
public:
    /// \bug is not thread safe, and called by another thread, error can occure
    //map slots, transmited by the current ClientNetworkRead
    bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction);
    virtual void purgeBuffer() = 0;
protected:
    //pass to the Map management visibility algorithm
    virtual void insertClient() = 0;
    virtual void moveClient(const uint8_t &previousMovedUnit,const Direction &direction) = 0;
};
}

#endif // CLIENTMAPMANAGEMENT_H
