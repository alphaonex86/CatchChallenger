#include "MapVisibilityAlgorithm_Simple.h"

MapVisibilityAlgorithm_Simple::MapVisibilityAlgorithm_Simple()
{
}

MapVisibilityAlgorithm_Simple::~MapVisibilityAlgorithm_Simple()
{
	wait_the_end.acquire();
}

void MapVisibilityAlgorithm_Simple::insertClient()
{
	loop_size=current_map->clients.size();
	if(loop_size<=generalData->mapVisibility.simple.max)
	{
		//insert the new client
		index=0;
		while(index<loop_size)
		{
			//register on other clients
			current_client=current_map->clients.at(index);
			current_client->insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
			//register the other client on him self
			insertAnotherClient(current_client->player_id,current_client->current_map,current_client->x,current_client->y,current_client->last_direction,current_client->speed);
			index++;
		}
	}
	else
	{
		//drop all show client because it have excess the limit
		//drop on all client
		index=0;
		while(index<loop_size)
		{
			current_map->clients.at(index)->dropAllClients();
			index++;
		}
	}
	//auto insert to know where it have spawn
	insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
}

void MapVisibilityAlgorithm_Simple::moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged)
{
	loop_size=current_map->clients.size();
	if(mapHaveChanged)
	{
		if(loop_size<=generalData->mapVisibility.simple.max)
		{
			//insert the new client
			index=0;
			while(index<loop_size)
			{
				//register on other clients
				current_client=current_map->clients.at(index);
				if(current_client!=this)
				{
					current_client->insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
					//register the other client on him self
					insertAnotherClient(current_client->player_id,current_client->current_map,current_client->x,current_client->y,current_client->last_direction,current_client->speed);
				}
				index++;
			}
		}
		else
		{
			//drop all show client because it have excess the limit
			//drop on all client
			index=0;
			while(index<loop_size)
			{
				current_map->clients.at(index)->dropAllClients();
				index++;
			}
		}
	}
	else
	{
		//here to know how player is affected
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		emit message(QString("after %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_id).arg(directionToString((Direction)direction)).arg(loop_size-1));
		#endif

		//normal operation
		if(loop_size<=generalData->mapVisibility.simple.max)
		{
			index=0;
			while(index<loop_size)
			{
				current_client=current_map->clients.at(index);
				if(current_client!=this)
					current_client->moveAnotherClient(player_id,movedUnit,direction);
				index++;
			}
		}
		else //all client is dropped due to over load on the map
		{
		}
	}
}

void MapVisibilityAlgorithm_Simple::removeClient()
{
	loop_size=current_map->clients.size();
	if(loop_size==(generalData->mapVisibility.simple.max))
	{
		//insert all the client because it start to be visible
		index=0;
		while(index<loop_size)
		{
			static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index))->reinsertAllClient(loop_size);
			index++;
		}
	}
	//nothing removed because all clients are already hide
	else if(loop_size>(generalData->mapVisibility.simple.max+1))
	{
	}
	else //normal working
	{
		index=0;
		while(index<loop_size)
		{
			current_map->clients.at(index)->removeAnotherClient(player_id);
			index++;
		}
	}
}

void MapVisibilityAlgorithm_Simple::mapVisiblity_unloadFromTheMap()
{
	removeClient();
}

void MapVisibilityAlgorithm_Simple::reinsertAllClient(const int &loop_size)
{
	index=0;
	while(index<loop_size)
	{
		current_client=current_map->clients.at(index);
		if(current_client!=this)
			current_client->insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
		index++;
	}
}
