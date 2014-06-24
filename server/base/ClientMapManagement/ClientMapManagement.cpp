#include "ClientMapManagement.h"
#include "../../VariableServer.h"

using namespace CatchChallenger;

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

/// \todo drop this class

bool ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    message(QStringLiteral("ClientMapManagement::moveThePlayer (%1,%2): %3, direction: %4, previousMovedUnit: %5").arg(x).arg(y).arg(player_informations->public_and_private_informations.public_informations.simplifiedId).arg(MoveOnTheMap::directionToString(direction)).arg(previousMovedUnit));
    #endif
    if(Q_UNLIKELY(!MapBasicMove::moveThePlayer(previousMovedUnit,direction)))
        return false;
    moveClient(previousMovedUnit,direction);
    return true;
}

