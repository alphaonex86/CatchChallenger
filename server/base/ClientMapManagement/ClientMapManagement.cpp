#include "ClientMapManagement.h"
#include "../EventDispatcher.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

ClientMapManagement::ClientMapManagement()
{
}

ClientMapManagement::~ClientMapManagement()
{
}

Map_player_info ClientMapManagement::getMapPlayerInfo()
{
	Map_player_info temp;
	temp.map		= current_map;
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

void ClientMapManagement::put_on_the_map(Map_server *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message(QString("ClientMapManagement put_on_the_map(): map: %1, x: %2, y: %3").arg(map->map_file).arg(x).arg(y));
	#endif
	MapBasicMove::put_on_the_map(map,x,y,orientation);

	//call MapVisibilityAlgorithm to insert
	insertClient();

	loadOnTheMap();
}

/// \note The second heavy function
bool ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_MOVE
	emit message(QString("ClientMapManagement::moveThePlayer (%1,%2): %3, direction: %4, previousMovedUnit: %5").arg(x).arg(y).arg(player_informations->public_and_private_informations.public_informations.simplifiedId).arg(directionToString(direction)).arg(previousMovedUnit));
	#endif
	mapHaveChanged=false;
	if(unlikely(!MapBasicMove::moveThePlayer(previousMovedUnit,direction)))
		return false;
	moveClient(previousMovedUnit,direction,mapHaveChanged);
	return true;
}

void ClientMapManagement::loadOnTheMap()
{
	current_map->clients << this;
}

void ClientMapManagement::unloadFromTheMap()
{
	mapHaveChanged=true;
	current_map->clients.removeOne(this);
	mapVisiblity_unloadFromTheMap();
}

void ClientMapManagement::dropAllClients()
{
	emit sendPacket(0xC4);
}
