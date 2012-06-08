#include "MapVisibilityAlgorithm_Simple.h"
#include "../EventDispatcher.h"

int MapVisibilityAlgorithm_Simple::index;
int MapVisibilityAlgorithm_Simple::loop_size;

MapVisibilityAlgorithm_Simple::MapVisibilityAlgorithm_Simple()
{
}

MapVisibilityAlgorithm_Simple::~MapVisibilityAlgorithm_Simple()
{
}

void MapVisibilityAlgorithm_Simple::insertClient()
{
	loop_size=current_map->clients.size();
	if(loop_size<=EventDispatcher::generalData.mapVisibility.simple.max)
	{
		//insert the new client
		index=0;
		while(index<loop_size)
		{
			//register on other clients
			current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
			current_client->insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
			//register the other client on him self
			insertAnotherClient(current_client->player_id,current_client->current_map,current_client->x,current_client->y,current_client->last_direction,current_client->speed);
			index++;
		}
	}
	else
	{
		if(current_map->mapVisibility.simple.show)
		{
			current_map->mapVisibility.simple.show=false;
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
	//auto insert to know where it have spawn
	insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
}

void MapVisibilityAlgorithm_Simple::moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged)
{
	loop_size=current_map->clients.size();
	if(mapHaveChanged)
	{
		if(loop_size<=EventDispatcher::generalData.mapVisibility.simple.max)
		{
			//insert the new client
			index=0;
			while(index<loop_size)
			{
				//register on other clients
				current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
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
		if(loop_size<=EventDispatcher::generalData.mapVisibility.simple.max)
		{
			index=0;
			while(index<loop_size)
			{
				current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
				if(current_client!=this)
					#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
					current_client->moveAnotherClient(player_id,current_map,movedUnit,direction);
					#else
					current_client->moveAnotherClient(player_id,movedUnit,direction);
					#endif
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
	if(loop_size==(EventDispatcher::generalData.mapVisibility.simple.reshow) && current_map->mapVisibility.simple.show==false)
	{
		current_map->mapVisibility.simple.show=true;
		//insert all the client because it start to be visible
		index=0;
		while(index<loop_size)
		{
			static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index))->reinsertAllClient(loop_size);
			index++;
		}
	}
	//nothing removed because all clients are already hide
	else if(loop_size>(EventDispatcher::generalData.mapVisibility.simple.max+1))
	{
	}
	else //normal working
	{
		index=0;
		while(index<loop_size)
		{
			//current_map -> wrong pointer
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
		current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
		if(current_client!=this)
			current_client->insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
		index++;
	}
}

#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
//remove the move/remove
void MapVisibilityAlgorithm_Simple::insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed)
{
	to_send_map_management_remove.remove(player_id);
	to_send_map_management_move.remove(player_id);
	ClientMapManagement::insertAnotherClient(player_id,map,x,y,direction,speed);
}
#endif

#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
void MapVisibilityAlgorithm_Simple::moveAnotherClient(const quint32 &player_id,const Map_final *map,const quint8 &movedUnit,const Direction &direction)
{
	if(overMove.contains(player_id))
	{
		to_send_map_management_remove.remove(player_id);
		ClientMapManagement::insertAnotherClient(player_id,map,x,y,direction,speed);
	}
	else if((to_send_map_management_move[player_id].size()*(sizeof(quint8)+sizeof(quint8))+sizeof(quint8))>=(sizeof(quint32)+map->map_file.size()*sizeof(quint16)+sizeof(quint16)+sizeof(quint16)+sizeof(quint8)+sizeof(quint16)))
	{
		to_send_map_management_move.remove(player_id);
		overMove << player_id;
	}
	else
		ClientMapManagement::moveAnotherClient(player_id,movedUnit,direction);
}
#endif

#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
//remove the move/insert
void MapVisibilityAlgorithm_Simple::removeAnotherClient(const quint32 &player_id)
{
	to_send_map_management_insert.remove(player_id);
	to_send_map_management_move.remove(player_id);
	ClientMapManagement::removeAnotherClient(player_id);
}
#endif
