#include "MapVisibilityAlgorithm_Simple.h"

MapVisibilityAlgorithm_Simple::MapVisibilityAlgorithm_Simple()
{
}

MapVisibilityAlgorithm_Simple::~MapVisibilityAlgorithm_Simple()
{
}

void MapVisibilityAlgorithm_Simple::insertClient(const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	if(generalData->connected_players<generalData->mapVisibility.simple.max)
	{
		//insert the new client
	}
	else
	{
		//drop all show client because it have excess the limit
	}
}

void MapVisibilityAlgorithm_Simple::moveClient(const quint8 &movedUnit,const Direction &direction)
{
#ifdef DEBUG_MESSAGE_CLIENT_MOVE
emit message(QString("after %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_id).arg(Map_custom::directionToString((Direction)direction)).arg(moveThePlayer_list_size));
#endif
}

void MapVisibilityAlgorithm_Simple::removeClient()
{
	if(generalData->connected_players==(generalData->mapVisibility.simple.max+1))
	{
		//insert all the client because it start to be visible
		index=0;
		loop_size=current_map->clients.size();
		while(index<loop_size)
		{
			static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index))->reinsertAllClient();
			index++;
		}
	}
	else //is already visible, remove it
	{
		index=0;
		loop_size=current_map->clients.size();
		while(index<loop_size)
		{
			current_map->clients.at(index)->removeAnotherClient(player_id);
			index++;
		}
	}
}

void MapVisibilityAlgorithm_Simple::changeMap(Map_final *old_map,Map_final *new_map)
{
}

void MapVisibilityAlgorithm_Simple::reinsertAllClient()
{
	index=0;
	loop_size=current_map->clients.size();
	while(index<loop_size)
	{
		if(current_map->clients.at(index)!=this)
			current_map->clients.at(index)->insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
		index++;
	}
}
