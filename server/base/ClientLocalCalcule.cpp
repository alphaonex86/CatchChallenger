#include "ClientLocalCalcule.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

ClientLocalCalcule::ClientLocalCalcule()
{
	current_map=NULL;
	player_informations=NULL;
	generalData=NULL;
	stopIt=false;
}

ClientLocalCalcule::~ClientLocalCalcule()
{
	stopIt=true;
}

bool ClientLocalCalcule::checkCollision()
{
	if(!current_map->parsed_layer.walkable[x+y*current_map->width])
	{
		emit error(QString("move at top, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->map_file));
		return false;
	}
	else
		return true;
}

void ClientLocalCalcule::put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	MapBasicMove::put_on_the_map(player_id,map,x,y,orientation,speed);

	loadOnTheMap();
}
