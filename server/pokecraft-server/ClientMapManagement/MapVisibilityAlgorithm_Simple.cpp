#include "MapVisibilityAlgorithm_Simple.h"

MapVisibilityAlgorithm_Simple::MapVisibilityAlgorithm_Simple()
{
}

MapVisibilityAlgorithm_Simple::~MapVisibilityAlgorithm_Simple()
{
}

void MapVisibilityAlgorithm_Simple::insertClient(const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	if(current_map->clients.size()<=generalData->mapVisibility.simple.max)
	{
		//insert the new client
		index=0;
		loop_size=current_map->clients.size();
		while(index<loop_size)
		{
			current_map->clients.at(index)->insertAnotherClient(player_id,current_map,x,y,static_cast<Direction>(orientation),speed);
			index++;
		}
	}
	else
	{
		//drop all show client because it have excess the limit
		dropAllClients();
	}
}

void MapVisibilityAlgorithm_Simple::moveClient(const quint8 &movedUnit,const Direction &direction)
{

}

void MapVisibilityAlgorithm_Simple::removeClient()
{
	if(current_map->clients.size()==(generalData->mapVisibility.simple.max))
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
	//nothing removed because all clients are already hide
	else if(current_map->clients.size()>(generalData->mapVisibility.simple.max+1))
	{
	}
	else //normal working
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
