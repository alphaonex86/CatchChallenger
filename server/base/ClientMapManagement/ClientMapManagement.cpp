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

ClientMapManagement::ClientMapManagement()
{
}

ClientMapManagement::~ClientMapManagement()
{
}

Map_player_info ClientMapManagement::getMapPlayerInfo()
{
    Map_player_info temp;
    temp.map		= map;
    temp.x			= x;
    temp.y			= y;
    return temp;
}

void ClientMapManagement::setVariable(Player_internal_informations *player_informations)
{
    MapBasicMove::setVariable(player_informations);
}

void ClientMapManagement::extraStop()
{
    //call MapVisibilityAlgorithm to remove
    //removeClient(); -> do by unload from map
}

bool ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(QStringLiteral("ClientMapManagement::moveThePlayer (%1,%2): %3, direction: %4, previousMovedUnit: %5").arg(x).arg(y).arg(player_informations->public_and_private_informations.public_informations.simplifiedId).arg(MoveOnTheMap::directionToString(direction)).arg(previousMovedUnit));
    #endif
    if(unlikely(!MapBasicMove::moveThePlayer(previousMovedUnit,direction)))
        return false;
    moveClient(previousMovedUnit,direction);
    return true;
}

void ClientMapManagement::dropAllClients()
{
    emit sendPacket(0xC4);
}
