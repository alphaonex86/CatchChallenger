#include "ClientMapManagement.hpp"
#include <iostream>
#include <string>

using namespace CatchChallenger;

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

/// \todo drop this class

bool ClientMapManagement::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    std::cout << "ClientMapManagement::moveThePlayer (" << std::to_string(x)
            << "," << std::to_string(y)
            << "), direction: " << MoveOnTheMap::directionToString(direction)
            << ", previousMovedUnit: " << std::to_string(previousMovedUnit)
            ;
    #endif
    if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
        return false;
    //moveClient(previousMovedUnit,direction);
    return true;
}

