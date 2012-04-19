#include "ClientMapManagement.h"

ClientMapManagement::ClientMapManagement()
{
	current_map=NULL;
	stopIt=false;
	propagated=false;
	firstInsert=true;
}

ClientMapManagement::~ClientMapManagement()
{
	stopIt=true;
	wait_the_end.acquire();
}

void ClientMapManagement::stop()
{
	stopIt=true;
}

Map_player_info ClientMapManagement::getMapPlayerInfo()
{
	Map_player_info temp;
	if(current_map==NULL)
		temp.loaded=false;
	else
	{
		temp.map		= current_map->map_final;
		temp.map_file_path	= current_map->mapName();
		temp.x			= x;
		temp.y			= y;
		temp.loaded		= true;
	}
	return temp;
}

void ClientMapManagement::setVariable(GeneralData *generalData)
{
	this->generalData=generalData;
}

void ClientMapManagement::askIfIsReadyToStop()
{
	stopIt=true;
	if(current_map==NULL)
	{
		emit isReadyToStop();
		wait_the_end.release();
		return;
	}
	unloadFromCurrentMap();
	to_send_map_management_insert.clear();
	to_send_map_management_move.clear();
	to_send_map_management_remove.clear();
	//virtual stop the player
	if(last_direction>4)
		last_direction=Direction((quint8)last_direction-4);
	Orientation orientation=(Orientation)last_direction;
	if(current_map->mapName()!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation)
	{
		emit updatePlayerPosition(current_map->mapName(),x,y,orientation);
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		DebugClass::debugConsole(
					QString("current_map->mapName(): %1,x: %2,y: %3, orientation: %4")
					.arg(current_map->mapName())
					.arg(x)
					.arg(y)
					.arg(orientation)
					);
		#endif
	}
	unloadMapIfNeeded(current_map);
	current_map=NULL;
	emit isReadyToStop();
	wait_the_end.release();
}

void ClientMapManagement::unloadFromCurrentMap()
{
	//remove player to current map
	if(current_map->clients.removeAll(this)!=1)
	//if(!current_map->clients.removeOne(this))
	{
		emit message("unable to remove the player from the map");
		return;
	}
	//tell at all other client the disconnect of this
	if(current_map->map_loaded)
	{
		int index_maps_clients=0;
		int list_size_maps_clients=current_map->clients.size();
		while(index_maps_clients<list_size_maps_clients)
		{
			current_map->clients.at(index_maps_clients)->removeClient(player_id);
			index_maps_clients++;
		}
	}
}

void ClientMapManagement::unloadMapIfNeeded(Map_custom * map)
{
	//not more client, can unload the map
	if(map->clients.size()==0)
	{
		generalData->map_list.removeOne(map);
		/// \note auto unregistred on other map by destructor
		delete map;
	}
}

void ClientMapManagement::put_on_the_map(const quint32 &player_id,const QString & map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	have_diff=false;
	at_start_x=x;
	at_start_y=y;
	at_start_orientation=orientation;
	last_direction=(Direction)orientation;
	at_start_map_name=map;
	this->player_id=player_id;
	this->x=x;
	this->y=y;
	propagated=false;
	this->speed=speed;
	current_map=getMap(map);
	current_map->clients << this;
	propagate();
}

//x and y can be negative, it's the offset
void ClientMapManagement::put_on_the_map_by_diff(const QString & map,const qint16 &x_offset_or_real,const qint16 &y_offset_or_real)
{
	emit message(QString("try load map by diff for for player: %1, transmited (x,y): %2,%3").arg(player_id).arg(x_offset_or_real).arg(y_offset_or_real));
	have_diff=true;
	this->x=x_offset_or_real;
	this->y=y_offset_or_real;
	propagated=false;
	current_map=getMap(map);
	current_map->clients << this;
	propagate();
	//set to 0 or map max size
	//moveThePlayer() to check colision and do real move
}

void ClientMapManagement::propagate()
{
	if(current_map==NULL)
	{
		DebugClass::debugConsole(QString("current_map==NULL and should not: %1").arg(player_id));
		return;
	}
	if(current_map->map_loaded)
	{
		if(propagated)
		{
			emit message(QString("already propagated: %1, (x,y): %2,%3").arg(player_id).arg(x).arg(y));
			return;
		}
		propagated=true;
		if(have_diff)
		{
			quint8 previousMovedUnit=0;
			have_diff=false;
			switch(last_direction)
			{
				/// \todo check if the map is corresponding
				case Direction_move_at_top:
					if(y<0)
					{
						emit error(QString("propagate(): y can't be negative at Direction_move_at_top"));
						return;
					}
					else
						emit message(QString("propagate(): enter and parse Direction_move_at_top"));
					previousMovedUnit=y;
					y=current_map->height;//useless but it's y=0 + offset stored into y
					x+=current_map->map_border_bottom.x;
				break;
				case Direction_move_at_right:
					if(x<0)
					{
						emit error(QString("propagate(): x can't be negative at Direction_move_at_right"));
						return;
					}
					else
						emit message(QString("propagate(): enter and parse Direction_move_at_right"));
					previousMovedUnit=x;
					x=-1;//spawn out of map to check the next tile
					y+=current_map->map_border_left.y;
				break;
				case Direction_move_at_bottom:
					if(y<0)
					{
						emit error(QString("propagate(): x can't be negative at Direction_move_at_bottom"));
						return;
					}
					else
						emit message(QString("propagate(): enter and parse Direction_move_at_bottom"));
					previousMovedUnit=y;
					y=-1;//spawn out of map to check the next tile
					x+=current_map->map_border_top.x;
				break;
				case Direction_move_at_left:
					if(x<0)
					{
						emit error(QString("propagate(): x can't be negative at Direction_move_at_left"));
						return;
					}
					else
						emit message(QString("propagate(): enter and parse Direction_move_at_left"));
					previousMovedUnit=x;
					x=current_map->width;//it's x=height + offset stored into x, then x=x+height
					y+=current_map->map_border_right.y;
				break;
				default:
					emit error(QString("propagate(): direction not managed"));
				break;
			}
			moveThePlayer(previousMovedUnit,last_direction,true);
			if(x<0 || y<0 || x>=current_map->width || y>=current_map->height)
				emit error(QString("internet error, out of the map: (x,y): %1,%2").arg(x).arg(y));
			else
				emit message(QString("new pos for the player: %1, (x,y): %2,%3").arg(player_id).arg(x).arg(y));
			last_direction=last_direction_diff;
			postMapMove();
		}
		else
		{
			current_map->check_client_position(current_map->clients.size()-1);
			int index_maps_clients=0;
			int list_size_map_custom=current_map->clients.size();
			while(index_maps_clients<list_size_map_custom)
			{
				ClientMapManagement *other_client=current_map->clients.at(index_maps_clients);
				if(other_client!=this)
					other_client->insertClient(player_id,current_map->mapName(),x,y,last_direction,speed);
				if(other_client!=this || firstInsert)
				{
					if(other_client==this)
						firstInsert=false;
					insertClient(other_client->player_id,
					     other_client->current_map->mapName(),
					     other_client->x,
					     other_client->y,
					     other_client->last_direction,
					     other_client->speed);
				}
				index_maps_clients++;
			}
			#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
			emit message(QString("normal insert: %1, (x,y): %2,%3").arg(player_id).arg(x).arg(y));
			#endif
		}
	}
	else
		emit message(QString("map not loaded: %1, (x,y): %2,%3").arg(player_id).arg(x).arg(y));
}

/* call before the map chaning, then on the old map
  move and remove all player from the current map and other player view
  */
void ClientMapManagement::preMapMove(const quint8 &previousMovedUnit,const Direction &direction)
{
	QList<ClientMapManagement *> previousClient;
	old_map=current_map;
	old_map->clients.removeOne(this);
	MapMove_nearClient.clear();
	getNearClient(MapMove_nearClient);
	index=0;
	list_size=previousClient.size();
	while(index<list_size)
	{
		previousClient.at(index)->moveClient(player_id,previousMovedUnit,direction);
		previousClient.at(index)->removeClient(player_id);
		index++;
	}
	last_direction_diff=direction;
}

/* call after the map chaning, then on the new map
  just insert from the new map, because the move will be call into propagate() by moveThePlayer()
  */
void ClientMapManagement::postMapMove()
{
	if(stopIt)
		return;
	MapMove_nearClient.clear();
	getNearClient(MapMove_nearClient);
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message(QString("postMapMove start: %1, getNearClient: %2, x,y: %3,%4, last_direction: %5, speed: %6").arg(player_id).arg(MapMove_nearClient.size()).arg(x).arg(y).arg((int)last_direction).arg(speed));
	#endif // DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	index=0;
	postMapMove_size=MapMove_nearClient.size();
	while(index<postMapMove_size)
	{
		if(x<0 || x>(current_map->width-1) || y<0 || y>(current_map->height-1))
		{
			emit message(QString("insertClient(): Wrong x,y: %1,%2").arg(x).arg(y));
			return;
		}
		MapMove_nearClient.at(index)->insertClient(player_id,current_map->mapName(),x,y,(Direction)last_direction,speed);
		index++;
	}
	current_map->clients << this;
	unloadMapIfNeeded(old_map);
}

/*void ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	moveThePlayer(previousMovedUnit,direction,false);
}*/

/// \note The second heavy function
void ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction,bool with_diff)
{
	if(stopIt)
		return;
	if(current_map==NULL)
	{
		emit message(QString("internal error, map pointer == NULL, for player: %1").arg(player_id));
		return;
	}
	if(current_map->map_loaded)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
		emit message(QString("for player (%1,%2): %3, previousMovedUnit: %4, direction: %5").arg(x).arg(y).arg(player_id).arg(previousMovedUnit).arg(Map_custom::directionToString((Direction)direction)));
		#endif
		moveThePlayer_index_move=0;
		if(last_direction==direction && with_diff==false)
		{
			emit error(QString("Previous action is same direction: %1").arg(last_direction));
			return;
		}
		switch(last_direction)
		{
			case Direction_move_at_top:
				if(previousMovedUnit==0)
					emit error(QString("Direction_move_at_top: Previous action is moving: %1").arg(last_direction));
				while(moveThePlayer_index_move<previousMovedUnit)
				{
					if(0==y)
					{
						if(current_map->map_border_top.fileName.isEmpty())
							emit error(QString("moveThePlayer(), out of map: %1, file name: \"%2\"").arg(current_map->mapName()).arg(current_map->map_border_top.fileName));
						else
						{
							preMapMove(previousMovedUnit,direction);
							emit message(QString("moveThePlayer(): move at top, out of map: %1, file name: \"%2\", current_map->map_border_top.y_offset: %3-%4=%5").arg(current_map->mapName()).arg(current_map->map_border_top.fileName).arg(x).arg(current_map->map_border_top.x).arg(x-current_map->map_border_top.x));
							put_on_the_map_by_diff(current_map->map_border_top.fileName,x-current_map->map_border_top.x,previousMovedUnit-moveThePlayer_index_move);
						}
						return;
					}
					y--;
					if(!current_map->is_walkalble(x,y))
					{
						emit error(QString("move at top, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->mapName()));
						return;
					}
					moveThePlayer_index_move++;
				}
			break;
			case Direction_look_at_top:
				if(previousMovedUnit>0 && with_diff==false)
					emit error(QString("Direction_look_at_top: Previous action is not moving: %1").arg(last_direction));
			break;
			case Direction_move_at_right:
				if(previousMovedUnit==0)
					emit error(QString("Direction_move_at_right: Previous action is moving: %1").arg(last_direction));
				while(moveThePlayer_index_move<previousMovedUnit)
				{
					if((current_map->width-1)==x)
					{
						if(current_map->map_border_right.fileName.isEmpty())
							emit error(QString("moveThePlayer(): move at right, out of map: %1, file name: \"%2\"").arg(current_map->mapName()).arg(current_map->map_border_right.fileName));
						else
						{
							preMapMove(previousMovedUnit,direction);
							emit message(QString("moveThePlayer(): move at right, out of map: %1, file name: \"%2\", current_map->map_border_right.y_offset: %3-%4=%5").arg(current_map->mapName()).arg(current_map->map_border_right.fileName).arg(y).arg(current_map->map_border_right.y).arg(y-current_map->map_border_right.y));
							put_on_the_map_by_diff(current_map->map_border_right.fileName,previousMovedUnit-moveThePlayer_index_move,y-current_map->map_border_right.y);
						}
						return;
					}
					x++;
					if(!current_map->is_walkalble(x,y))
					{
						emit error(QString("move at right, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->mapName()));
						return;
					}
					moveThePlayer_index_move++;
				}
			break;
			case Direction_look_at_right:
				if(previousMovedUnit>0 && with_diff==false)
					emit error(QString("Direction_look_at_right: Previous action is not moving: %1").arg(last_direction));
			break;
			case Direction_move_at_bottom:
				if(previousMovedUnit==0)
					emit error(QString("Direction_move_at_bottom: Previous action is moving: %1").arg(last_direction));
				while(moveThePlayer_index_move<previousMovedUnit)
				{
					if((current_map->height-1)==y)
					{
						if(current_map->map_border_bottom.fileName.isEmpty())
							emit error(QString("moveThePlayer(): move at bottom, out of map: %1, file name: \"%2\"").arg(current_map->mapName()).arg(current_map->map_border_bottom.fileName));
						else
						{
							preMapMove(previousMovedUnit,direction);
							emit message(QString("moveThePlayer(): move at bottom, out of map: %1, file name: \"%2\", current_map->map_border_bottom.y_offset: %3-%4=%5").arg(current_map->mapName()).arg(current_map->map_border_bottom.fileName).arg(x).arg(current_map->map_border_bottom.x).arg(x-current_map->map_border_bottom.x));
							put_on_the_map_by_diff(current_map->map_border_bottom.fileName,x-current_map->map_border_bottom.x,previousMovedUnit-moveThePlayer_index_move);
						}
						return;
					}
					y++;
					if(!current_map->is_walkalble(x,y))
					{
						emit error(QString("move at bottom, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->mapName()));
						return;
					}
					moveThePlayer_index_move++;
				}
			break;
			case Direction_look_at_bottom:
				if(previousMovedUnit>0 && with_diff==false)
					emit error(QString("Direction_look_at_bottom: Previous action is not moving: %1").arg(last_direction));
			break;
			case Direction_move_at_left:
				if(previousMovedUnit==0)
					emit error(QString("Direction_move_at_left: Previous action is moving: %1").arg(last_direction));
				while(moveThePlayer_index_move<previousMovedUnit)
				{
					if(0==x)
					{
						if(current_map->map_border_left.fileName.isEmpty())
							emit error(QString("moveThePlayer(): move at left, out of map: %1, file name: \"%2\"").arg(current_map->mapName()).arg(current_map->map_border_left.fileName));
						else
						{
							preMapMove(previousMovedUnit,direction);
							emit message(QString("moveThePlayer(): move at left, out of map: %1, file name: \"%2\", current_map->map_border_left.y_offset: %3-%4=%5").arg(current_map->mapName()).arg(current_map->map_border_left.fileName).arg(y).arg(current_map->map_border_left.y).arg(y-current_map->map_border_left.y));
							put_on_the_map_by_diff(current_map->map_border_left.fileName,previousMovedUnit-moveThePlayer_index_move,y-current_map->map_border_left.y);
						}
						return;
					}
					x--;
					if(!current_map->is_walkalble(x,y))
					{
						emit error(QString("move at left, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->mapName()));
						return;
					}
					moveThePlayer_index_move++;
				}
			break;
			case Direction_look_at_left:
				if(previousMovedUnit>0 && with_diff==false)
					emit error(QString("Direction_look_at_left: Previous action is not moving: %1").arg(last_direction));
			break;
			default:
				emit error(QString("moveThePlayer(): direction not managed"));
			break;
		}
		moveThePlayer_returnList.clear();
		getNearClient(moveThePlayer_returnList);
		moveThePlayer_index=0;
		moveThePlayer_list_size=moveThePlayer_returnList.size();
		while(moveThePlayer_index<moveThePlayer_list_size)
		{
			if(moveThePlayer_returnList[moveThePlayer_index]->player_id!=player_id)
				moveThePlayer_returnList[moveThePlayer_index]->moveClient(player_id,previousMovedUnit,(Direction)direction);
			moveThePlayer_index++;
		}
		last_direction=direction;
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		emit message(QString("after %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_id).arg(Map_custom::directionToString((Direction)direction)).arg(moveThePlayer_list_size));
		#endif
	}
	else
	{
/*		map_management_movement temp;
		temp.movedUnit=previousMovedUnit;
		temp.direction=(Direction)direction;
		delayed_map_management_move << temp;*/
	}
}

//send client only on near map loaded and not the current player
void ClientMapManagement::getNearClient(QList<ClientMapManagement *> & returnList)
{
	if(!current_map->map_loaded)
		return;
	returnList << current_map->clients;
	/// \todo uncomment and fix this code
	/*
	getNearClient_index=0;
	getNearClient_list_size=current_map->clients.size();
	while(getNearClient_index<getNearClient_list_size)
	{
		if(current_map->clients.at(getNearClient_index)!=this)
			returnList << current_map->clients.at(getNearClient_index);
		getNearClient_index++;
	}*/
}

void ClientMapManagement::insertClient(const quint32 &player_id,const QString &map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed)
{
	if(current_map==NULL)
	{
		emit message("internal error, map pointer == NULL at insertClient()");
		return;
	}
	if(x>(current_map->width-1) || y>(current_map->height-1))
	{
		emit message(QString("insertClient(): Wrong x,y: %1,%2").arg(x).arg(y));
		return;
	}
	insertClient_temp.fileName=map;
	insertClient_temp.id=player_id;
	insertClient_temp.direction=direction;
	insertClient_temp.x=x;
	insertClient_temp.y=y;
	insertClient_temp.speed=speed;
	to_send_map_management_insert << insertClient_temp;
}

/// \note The first heavy function
void ClientMapManagement::moveClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("moveClient(%1,%2,%3)").arg(player_id).arg(movedUnit).arg(Map_custom::directionToString((Direction)direction)));
	#endif
	moveClient_tempMov.movedUnit=movedUnit;
	moveClient_tempMov.direction=direction;
	to_send_map_management_move[player_id] << moveClient_tempMov;
}

void ClientMapManagement::removeClient(const quint32 &player_id)
{
	if(current_map==NULL)
	{
		emit message("internal error, map pointer == NULL at removeClient()");
		return;
	}
	if(stopIt)
		return;
	if(to_send_map_management_remove.contains(player_id))
		emit message("try dual remove");
	else
	{
		to_send_map_management_move.remove(player_id);
		to_send_map_management_remove << player_id;
	}
}

void ClientMapManagement::purgeBuffer()
{
	if(to_send_map_management_insert.size()==0 && to_send_map_management_move.size()==0 && to_send_map_management_remove.size()==0)
		return;
	if(stopIt)
		return;

	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message("purgeBuffer() runing....");
	#endif

	purgeBuffer_player_affected=0;

	purgeBuffer_outputData.clear();
	QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC0;
	purgeBuffer_outputDataLoop.clear();
	QDataStream outLoop(&purgeBuffer_outputDataLoop, QIODevice::WriteOnly);
	outLoop.setVersion(QDataStream::Qt_4_4);

	purgeBuffer_list_size=to_send_map_management_remove.size();
	if(purgeBuffer_list_size>0)
	{
		purgeBuffer_index=0;
		while(purgeBuffer_index<purgeBuffer_list_size)
		{
			if(stopIt)
				return;
			#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
			emit message(
						QString("player_id to remove: %1, for player: %2")
						.arg(to_send_map_management_remove.at(purgeBuffer_index))
						.arg(player_id)
						 );
			#endif
			outLoop << to_send_map_management_remove.at(purgeBuffer_index);
			outLoop << (quint8)0x03;
			purgeBuffer_index++;
			purgeBuffer_player_affected++;
		}
		to_send_map_management_remove.clear();
	}

	purgeBuffer_list_size=to_send_map_management_insert.size();
	if(purgeBuffer_list_size>0)
	{
		purgeBuffer_index=0;
		while(purgeBuffer_index<purgeBuffer_list_size)
		{
			if(stopIt)
				return;
			#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
			emit message(
						QString("insert player_id: %1, mapName %2, x: %3, y: %4,direction: %5, for player: %6")
						.arg(to_send_map_management_insert.at(purgeBuffer_index).id)
						.arg(to_send_map_management_insert.at(purgeBuffer_index).fileName)
						.arg(to_send_map_management_insert.at(purgeBuffer_index).x)
						.arg(to_send_map_management_insert.at(purgeBuffer_index).y)
						.arg(Map_custom::directionToString(to_send_map_management_insert.at(purgeBuffer_index).direction))
						.arg(player_id)
						 );
			#endif
			outLoop << to_send_map_management_insert.at(purgeBuffer_index).id;
			outLoop << (quint8)0x01;
			outLoop << to_send_map_management_insert.at(purgeBuffer_index).fileName;
			outLoop << to_send_map_management_insert.at(purgeBuffer_index).x;
			outLoop << to_send_map_management_insert.at(purgeBuffer_index).y;
			outLoop << (quint8)to_send_map_management_insert.at(purgeBuffer_index).direction;
			outLoop << to_send_map_management_insert.at(purgeBuffer_index).speed;
			purgeBuffer_index++;
			purgeBuffer_player_affected++;
		}
		to_send_map_management_insert.clear();
	}

	purgeBuffer_list_size_internal=0;
	purgeBuffer_indexMovement=0;
	QHash<quint32, QList<map_management_movement> >::const_iterator i = to_send_map_management_move.constBegin();
	while (i != to_send_map_management_move.constEnd())
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
					QString("move player_id: %1, for player: %2")
					.arg(i.key())
					.arg(player_id)
					 );
		#endif
		outLoop << i.key();
		outLoop << (quint8)0x02;
		purgeBuffer_list_size_internal=i.value().size();
		if(purgeBuffer_list_size_internal==0)
			DebugClass::debugConsole(QString("for player: %1, list_size_internal==0").arg(this->player_id));
		outLoop << (quint8)purgeBuffer_list_size_internal;
		purgeBuffer_indexMovement=0;
		while(purgeBuffer_indexMovement<purgeBuffer_list_size_internal)
		{
			outLoop << i.value().at(purgeBuffer_indexMovement).movedUnit;
			outLoop << (quint8)i.value().at(purgeBuffer_indexMovement).direction;
			purgeBuffer_indexMovement++;
		}
		purgeBuffer_player_affected++;
		if(stopIt)
			return;
		++i;
	}
	to_send_map_management_move.clear();

	if(stopIt)
		return;

	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("player_affected: %1").arg(purgeBuffer_player_affected));
	#endif
	out << purgeBuffer_player_affected;
	purgeBuffer_outputData+=purgeBuffer_outputDataLoop;
	emit sendPacket(purgeBuffer_outputData);
}

Map_custom * ClientMapManagement::getMap(const QString & mapName)
{
	int index=0;
	int map_list_size=generalData->map_list.size();
	while(index<map_list_size)
	{
		//current map found in already loaded
		if(generalData->map_list.at(index)->mapName()==mapName)
			return generalData->map_list.at(index);
		index++;
	}
	generalData->map_list << new Map_custom(generalData->eventThreaderList.at(3),&generalData->map_list);
	generalData->map_list.last()->moveToThread(generalData->eventThreaderList.at(3));
	generalData->map_list.last()->loadMap(mapName,generalData->mapBasePath);
	return generalData->map_list.last();
}

void ClientMapManagement::mapError(const QString &errorString)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint32)0x00000008;
	out << (quint8)0x03;
	out << "Unable to load the current map";
	emit sendPacket(outputData);
	emit error(errorString);
}
