#include "MapVisibilityAlgorithm_Simple.h"
#include "../EventDispatcher.h"

int MapVisibilityAlgorithm_Simple::index;
int MapVisibilityAlgorithm_Simple::loop_size;
MapVisibilityAlgorithm_Simple *MapVisibilityAlgorithm_Simple::current_client;

//temp variable for purge buffer
QByteArray MapVisibilityAlgorithm_Simple::purgeBuffer_outputData;
QByteArray MapVisibilityAlgorithm_Simple::purgeBuffer_outputDataLoop;
int MapVisibilityAlgorithm_Simple::purgeBuffer_index;
int MapVisibilityAlgorithm_Simple::purgeBuffer_list_size;
int MapVisibilityAlgorithm_Simple::purgeBuffer_list_size_internal;
int MapVisibilityAlgorithm_Simple::purgeBuffer_indexMovement;
map_management_move MapVisibilityAlgorithm_Simple::purgeBuffer_move;
QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >::const_iterator MapVisibilityAlgorithm_Simple::i_move;
QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >::const_iterator MapVisibilityAlgorithm_Simple::i_move_end;
QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>::const_iterator MapVisibilityAlgorithm_Simple::i_insert;
QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>::const_iterator MapVisibilityAlgorithm_Simple::i_insert_end;
QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator MapVisibilityAlgorithm_Simple::i_remove;
QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator MapVisibilityAlgorithm_Simple::i_remove_end;

//temp variable to move on the map
map_management_movement MapVisibilityAlgorithm_Simple::moveClient_tempMov;

MapVisibilityAlgorithm_Simple::MapVisibilityAlgorithm_Simple()
{
}

MapVisibilityAlgorithm_Simple::~MapVisibilityAlgorithm_Simple()
{
}

void MapVisibilityAlgorithm_Simple::insertClient()
{
	loop_size=current_map->clients.size();
	if(likely(loop_size<=EventDispatcher::generalData.serverSettings.mapVisibility.simple.max))
	{
		//insert the new client
		index=0;
		while(index<loop_size)
		{
			//register on other clients
			current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
			current_client->insertAnotherClient(player_id,this);
			this->insertAnotherClient(current_client->player_id,current_client);
			index++;
		}
	}
	else
	{
		if(unlikely(current_map->mapVisibility.simple.show))
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
	//auto insert to know where it have spawn, now in charge of ClientLocalCalcule
	//insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
}

void MapVisibilityAlgorithm_Simple::moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged)
{
	loop_size=current_map->clients.size();
	if(unlikely(mapHaveChanged))
	{
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		emit message(QString("map have change %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_id).arg(directionToString((Direction)direction)).arg(loop_size-1));
		#endif
		if(likely(loop_size<=EventDispatcher::generalData.serverSettings.mapVisibility.simple.max))
		{
			//insert the new client
			index=0;
			while(index<loop_size)
			{
				//register on other clients
				current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
				if(likely(current_client!=this))
				{
					current_client->insertAnotherClient(player_id,this);
					//register the other client on him self
					this->insertAnotherClient(current_client->player_id,current_client);
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
		if(likely(loop_size<=EventDispatcher::generalData.serverSettings.mapVisibility.simple.max))
		{
			index=0;
			while(index<loop_size)
			{
				current_client=static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index));
				if(likely(current_client!=this))
					#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
					current_client->moveAnotherClientWithMap(player_id,this,movedUnit,direction);
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

//drop all clients
void MapVisibilityAlgorithm_Simple::dropAllClients()
{
	to_send_insert.clear();
	to_send_move.clear();
	to_send_remove.clear();
	to_send_over_move.clear();

	ClientMapManagement::dropAllClients();
}

void MapVisibilityAlgorithm_Simple::removeClient()
{
	loop_size=current_map->clients.size();
	if(unlikely(loop_size==(EventDispatcher::generalData.serverSettings.mapVisibility.simple.reshow) && current_map->mapVisibility.simple.show==false))
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
	else if(unlikely(loop_size>(EventDispatcher::generalData.serverSettings.mapVisibility.simple.max+1)))
	{
	}
	else //normal working
	{
		index=0;
		while(index<loop_size)
		{
			//current_map -> wrong pointer
			static_cast<MapVisibilityAlgorithm_Simple*>(current_map->clients.at(index))->removeAnotherClient(player_id);
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
		if(likely(current_client!=this))
		{
			current_client->insertAnotherClient(player_id,this);
			this->insertAnotherClient(player_id,current_client);
		}
		index++;
	}
}

#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
//remove the move/remove
void MapVisibilityAlgorithm_Simple::insertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple *the_another_player)
{
	to_send_remove.remove(player_id);
	to_send_move.remove(player_id);

	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("insertAnotherClient(%1,%2,%3,%4)").arg(player_id).arg(the_another_player->current_map->map_file).arg(the_another_player->x).arg(the_another_player->y));
	#endif
	to_send_insert[player_id]=the_another_player;
}
#endif

#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
void MapVisibilityAlgorithm_Simple::moveAnotherClientWithMap(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple *the_another_player,const quint8 &movedUnit,const Direction &direction)
{
	//already into over move
	if(to_send_insert.contains(player_id) || to_send_over_move.contains(player_id))
	{
		//to_send_map_management_remove.remove(player_id); -> what?
		return;//quit now
	}
	//go into over move
	else if(unlikely(
				(to_send_move[player_id].size()*(sizeof(quint8)+sizeof(quint8))+sizeof(quint8))//the size of one move
				>=
					//size of on insert
					EventDispatcher::generalData.serverPrivateVariables.sizeofInsertRequest
				))
	{
		to_send_move.remove(player_id);
		to_send_over_move[player_id]=the_another_player;
	}
	else
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(QString("moveClient(%1,%2,%3)").arg(player_id).arg(movedUnit).arg(directionToString((Direction)direction)));
		#endif

		moveClient_tempMov.movedUnit=movedUnit;
		moveClient_tempMov.direction=direction;
		to_send_move[player_id] << moveClient_tempMov;
	}
}
#endif

#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
//remove the move/insert
void MapVisibilityAlgorithm_Simple::removeAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id)
{
	#ifdef POKECRAFT_SERVER_EXTRA_CHECK
	if(unlikely(!stopIt && to_send_remove.contains(player_id)))
	{
		emit message("removeAnotherClient() try dual remove");
		return;
	}
	#endif

	to_send_insert.remove(player_id);
	to_send_move.remove(player_id);
	#ifdef POKECRAFT_SERVER_MAP_DROP_OVER_MOVE
	to_send_over_move.remove(player_id);
	#endif

	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	if(unlikely(!stopIt))
		emit message(QString("removeAnotherClient(%1)").arg(player_id));
	#endif

	/* Do into the upper class, like MapVisibilityAlgorithm_Simple
	 * #ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
	//remove the move/insert
	to_send_map_management_insert.remove(player_id);
	to_send_map_management_move.remove(player_id);
	#endif */
	to_send_remove << player_id;
}
#endif

void MapVisibilityAlgorithm_Simple::extraStop()
{
	to_send_insert.clear();
	to_send_move.clear();
	to_send_remove.clear();
}

void MapVisibilityAlgorithm_Simple::purgeBuffer()
{
	/// \todo suspend when is into fighting
	if(to_send_insert.size()==0 && to_send_move.size()==0 && to_send_remove.size()==0)
		return;
	if(stopIt)
		return;

	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message("purgeBuffer() runing....");
	#endif

	purgeBuffer_outputData.clear();
	QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);

	//////////////////////////// insert //////////////////////////
	if(to_send_insert.size()==0)
		out << (quint8)0x00;
	else
	{
		/* can be only this map with this algo, then 1 map */
		out << (quint8)0x01;
		purgeBuffer_outputData+=current_map->rawMapFile;
		out.device()->seek(out.device()->pos()+current_map->rawMapFile.size());
		if(EventDispatcher::generalData.serverSettings.max_players<=255)
			out << (quint8)to_send_insert.size();
		else
			out << (quint16)to_send_insert.size();

		i_insert = to_send_insert.constBegin();
		i_insert_end = to_send_insert.constEnd();
		while (i_insert != i_insert_end)
		{
			#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
			emit message(
				QString("insert player_id: %1, mapName %2, x: %3, y: %4,direction: %5, for player: %6")
				.arg(i_insert.key())
				.arg(i_insert.value()->current_map->map_file)
				.arg(i_insert.value()->x)
				.arg(i_insert.value()->y)
				.arg(directionToString(i_insert.value()->last_direction))
				.arg(player_id)
				 );
			#endif
			if(EventDispatcher::generalData.serverSettings.max_players<=255)
				out << (quint8)i_insert.key();
			else
				out << (quint16)i_insert.key();
			out << i_insert.value()->x;
			out << i_insert.value()->y;
			out << ((quint8)i_insert.value()->last_direction | (quint8)i_insert.value()->player_informations->public_and_private_informations.public_informations.type);
			out << i_insert.value()->player_informations->public_and_private_informations.public_informations.speed;
			//clan
			out << i_insert.value()->player_informations->public_and_private_informations.public_informations.clan;
			//pseudo
			purgeBuffer_outputData+=i_insert.value()->player_informations->rawPseudo;
			out.device()->seek(out.device()->pos()+i_insert.value()->player_informations->rawPseudo.size());
			//skin
			purgeBuffer_outputData+=i_insert.value()->player_informations->rawSkin;
			out.device()->seek(out.device()->pos()+i_insert.value()->player_informations->rawSkin.size());

			++i_insert;
		}
		to_send_insert.clear();
	}

	//////////////////////////// move //////////////////////////
	purgeBuffer_list_size_internal=0;
	purgeBuffer_indexMovement=0;
	i_move = to_send_move.constBegin();
	i_move_end = to_send_move.constEnd();
	if(EventDispatcher::generalData.serverSettings.max_players<=255)
		out << (quint8)to_send_move.size();
	else
		out << (quint16)to_send_move.size();
	while (i_move != i_move_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("move player_id: %1, for player: %2")
			.arg(i_move.key())
			.arg(player_id)
			 );
		#endif
		if(EventDispatcher::generalData.serverSettings.max_players<=255)
			out << (quint8)i_move.key();
		else
			out << (quint16)i_move.key();

		purgeBuffer_list_size_internal=i_move.value().size();
		out << (quint8)purgeBuffer_list_size_internal;
		purgeBuffer_indexMovement=0;
		while(purgeBuffer_indexMovement<purgeBuffer_list_size_internal)
		{
			out << i_move.value().at(purgeBuffer_indexMovement).movedUnit;
			out << (quint8)i_move.value().at(purgeBuffer_indexMovement).direction;
			purgeBuffer_indexMovement++;
		}
		++i_move;
	}
	to_send_move.clear();

	//////////////////////////// remove //////////////////////////
	i_remove = to_send_remove.constBegin();
	i_remove_end = to_send_remove.constEnd();
	if(EventDispatcher::generalData.serverSettings.max_players<=255)
		out << (quint8)to_send_remove.size();
	else
		out << (quint16)to_send_remove.size();
	while (i_remove != i_remove_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("player_id to remove: %1, for player: %2")
			.arg((quint32)(*i_remove))
			.arg(player_id)
			 );
		#endif
		if(EventDispatcher::generalData.serverSettings.max_players<=255)
			out << (quint8)*i_remove;
		else
			out << (quint16)*i_remove;
		++i_remove;
	}
	to_send_remove.clear();

	purgeBuffer_outputData+=purgeBuffer_outputDataLoop;
	emit sendPacket(0xC0,purgeBuffer_outputData);
	#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
	send_reinsert();
	#endif
}

#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
void MapVisibilityAlgorithm_Simple::send_reinsert()
{
	if(to_send_over_move.size()==0)
		return;

	purgeBuffer_outputData.clear();
	QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);

	//////////////////////////// re-insert //////////////////////////
	if(EventDispatcher::generalData.serverSettings.max_players<=255)
		out << (quint8)to_send_over_move.size();
	else
		out << (quint16)to_send_over_move.size();

	i_insert = to_send_over_move.constBegin();
	i_insert_end = to_send_over_move.constEnd();
	while (i_insert != i_insert_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("reinsert player_id: %1, x: %2, y: %3,direction: %4, for player: %5")
			.arg(i_insert.key())
			.arg(i_insert.value()->x)
			.arg(i_insert.value()->y)
			.arg(directionToString(i_insert.value()->last_direction))
			.arg(player_id)
			 );
		#endif
		if(EventDispatcher::generalData.serverSettings.max_players<=255)
			out << (quint8)i_insert.key();
		else
			out << (quint16)i_insert.key();
		out << i_insert.value()->x;
		out << i_insert.value()->y;
		out << (quint8)i_insert.value()->last_direction;

		++i_insert;
	}
}
#endif
