#include "ClientMapManagement.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/

ClientMapManagement::ClientMapManagement()
{
	current_map=NULL;
	stopIt=false;
}

ClientMapManagement::~ClientMapManagement()
{
	stopIt=true;
	wait_the_end.acquire();
}

Map_player_info ClientMapManagement::getMapPlayerInfo()
{
	Map_player_info temp;
	temp.map		= current_map;
	temp.x			= x;
	temp.y			= y;
	return temp;
}

void ClientMapManagement::setVariable(GeneralData *generalData)
{
	this->generalData=generalData;
	connect(&generalData->timer_to_send_insert_move_remove,	SIGNAL(timeout()),this,SLOT(purgeBuffer()),Qt::QueuedConnection);
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
	unloadFromTheMap();
	//call MapVisibilityAlgorithm to remove
	removeClient();
	to_send_map_management_insert.clear();
	to_send_map_management_move.clear();
	to_send_map_management_remove.clear();
	//virtual stop the player
	if(last_direction>4)
		last_direction=Direction((quint8)last_direction-4);
	Orientation orientation=(Orientation)last_direction;
	if(current_map->map_file!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation)
	{
		emit updatePlayerPosition(current_map->map_file,x,y,orientation);
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		DebugClass::debugConsole(
					QString("current_map->map_file: %1,x: %2,y: %3, orientation: %4")
					.arg(current_map->map_file)
					.arg(x)
					.arg(y)
					.arg(orientation)
					);
		#endif
	}
	current_map=NULL;
	emit isReadyToStop();
	wait_the_end.release();
}

void ClientMapManagement::put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	//store the starting informations
	at_start_x=x;
	at_start_y=y;
	at_start_orientation=orientation;
	last_direction=(Direction)orientation;
	at_start_map_name=map->map_file;

	//store the current information about player on the map
	this->player_id=player_id;
	this->x=x;
	this->y=y;
	this->speed=speed;
	current_map=map;
	loadOnTheMap();

	#ifdef POKECRAFT_SERVER_EXTRA_CHECK
	if(this->x>(current_map->width-1))
	{
		emit message(QString("put_on_the_map(): Wrong x: %1").arg(x));
		this->x=current_map->width-1;
	}
	if(this->y>(current_map->height-1))
	{
		emit message(QString("put_on_the_map(): Wrong y: %1").arg(y));
		this->y=current_map->height-1;
	}
	#endif

	//call MapVisibilityAlgorithm to insert
	insertClient(x,y,orientation,speed);
}

//x and y can be negative, it's the offset
/*void ClientMapManagement::put_on_the_map_by_diff(const QString & map,const qint16 &x_offset_or_real,const qint16 &y_offset_or_real)
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
}*/

/*void ClientMapManagement::propagate()
{
	if(current_map==NULL)
	{
		emit message(QString("current_map==NULL and should not: %1").arg(player_id));
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
					x+=current_map->border.bottom.x;
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
					y+=current_map->border.left.y;
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
					x+=current_map->border.top.x;
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
					y+=current_map->border.right.y;
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
					other_client->insertAnotherClient(player_id,current_map->map_file,x,y,last_direction,speed);
				if(other_client!=this || firstInsert)
				{
					if(other_client==this)
						firstInsert=false;
					insertAnotherClient(other_client->player_id,
					     other_client->current_map->map_file,
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
}*/

/* call before the map chaning, then on the old map
  move and remove all player from the current map and other player view
  */
/*void ClientMapManagement::preMapMove(const quint8 &previousMovedUnit,const Direction &direction)
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
		previousClient.at(index)->moveAnotherClient(player_id,previousMovedUnit,direction);
		previousClient.at(index)->removeAnotherClient(player_id);
		index++;
	}
	last_direction_diff=direction;
}*/

/* call after the map chaning, then on the new map
  just insert from the new map, because the move will be call into propagate() by moveThePlayer()
  */
/*void ClientMapManagement::postMapMove()
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
		MapMove_nearClient.at(index)->insertAnotherClient(player_id,current_map->map_file,x,y,(Direction)last_direction,speed);
		index++;
	}
	current_map->clients << this;
	unloadMapIfNeeded(old_map);
}*/

/*void ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	moveThePlayer(previousMovedUnit,direction,false);
}*/

/// \note The second heavy function
void ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message(QString("for player (%1,%2): %3, previousMovedUnit: %4, direction: %5").arg(x).arg(y).arg(player_id).arg(previousMovedUnit).arg(Map_custom::directionToString((Direction)direction)));
	#endif
	moveThePlayer_index_move=0;
	if(last_direction==direction)
	{
		emit error(QString("Previous action is same direction: %1").arg(last_direction));
		return;
	}
	switch(last_direction)
	{
		case Direction_move_at_top:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_top: Previous action is moving: %1").arg(last_direction));
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if(0==y)
				{
					if(current_map->border.top.map==NULL)
					{
						emit error(QString("moveThePlayer(), out of map: %1 by top").arg(current_map->map_file));
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at top, leave the map: %1, enter to the map: %2, the enter add x offset=%3").arg(current_map->map_file).arg(current_map->border.top.map->map_file).arg(x-current_map->border.top.x_offset));
						unloadFromTheMap();
						changeMap(current_map,current_map->border.top.map);
						y=current_map->border.top.map->height;
						x+=current_map->border.top.x_offset;
						current_map=current_map->border.top.map;
						loadOnTheMap();
					}
					return;
				}
				else
					y--;
				if(!walkable[x+y*width])
				{
					emit error(QString("move at top, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->map_file));
					return;
				}
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_top:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_top: Previous action is not moving: %1").arg(last_direction));
				return;
			}
		break;
		case Direction_move_at_right:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_right: Previous action is moving: %1").arg(last_direction));
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if((current_map->width-1)==x)
				{
					if(current_map->border.right.map==NULL)
					{
						emit error(QString("moveThePlayer(): move at right, out of map: %1, by right").arg(current_map->map_file));
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at right, leave the map: %1, enter to the map: %2, the enter add y offset=%3").arg(current_map->map_file).arg(current_map->border.right.map->map_file).arg(y-current_map->border.right.y_offset));
						unloadFromTheMap();
						changeMap(current_map,current_map->border.right.map);
						y=0;
						x+=current_map->border.right.y_offset;
						current_map=current_map->border.right.map;
						loadOnTheMap();
					}
					return;
				}
				else
					x++;
				if(!walkable[x+y*width])
				{
					emit error(QString("move at right, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->map_file));
					return;
				}
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_right:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_right: Previous action is not moving: %1").arg(last_direction));
				return;
			}
		break;
		case Direction_move_at_bottom:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_bottom: Previous action is moving: %1").arg(last_direction));
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if((current_map->height-1)==y)
				{
					if(current_map->border.bottom.map==NULL)
					{
						emit error(QString("moveThePlayer(): move at bottom, out of map: %1, by bottom").arg(current_map->map_file));
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at bottom, leave the map: %1, enter to the map: %2, the enter add x offset=%3").arg(current_map->map_file).arg(current_map->border.bottom.map->map_file).arg(x-current_map->border.bottom.x_offset));
						unloadFromTheMap();
						changeMap(current_map,current_map->border.bottom.map);
						y=0;
						x+=current_map->border.bottom.x_offset;
						current_map=current_map->border.bottom.map;
						loadOnTheMap();
					}
					return;
				}
				else
					y++;
				if(!walkable[x+y*width])
				{
					emit error(QString("move at bottom, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->map_file));
					return;
				}
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_bottom:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_bottom: Previous action is not moving: %1").arg(last_direction));
				return;
			}
		break;
		case Direction_move_at_left:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_left: Previous action is moving: %1").arg(last_direction));
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if(0==x)
				{
					if(current_map->border.left.map==NULL)
					{
						emit error(QString("moveThePlayer(): move at left, out of map: %1, by left").arg(current_map->map_file));
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at left, leave the map: %1, enter to the map: %2, the enter add y offset=%3").arg(current_map->map_file).arg(current_map->border.left.map->map_file).arg(y-current_map->border.left.y_offset));
						unloadFromTheMap();
						changeMap(current_map,current_map->border.left.map);
						y=current_map->border.left.map->width;
						x+=current_map->border.left.y_offset;
						current_map=current_map->border.left.map;
						loadOnTheMap();
					}
					return;
				}
				else
					x--;
				if(!walkable[x+y*width])
				{
					emit error(QString("move at left, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->map_file));
					return;
				}
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_left:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_left: Previous action is not moving: %1").arg(last_direction));
				return;
			}
		break;
		default:
			emit error(QString("moveThePlayer(): direction not managed"));
			return;
		break;
	}
	last_direction=direction;

	#ifdef DEBUG_MESSAGE_CLIENT_MOVE
	emit message(QString("after %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_id).arg(Map_custom::directionToString((Direction)direction)).arg(moveThePlayer_list_size));
	#endif
	moveClient(previousMovedUnit,direction);
}

void ClientMapManagement::loadOnTheMap()
{
	walkable=current_map->parsed_layer.walkable;
	width=current_map->width;
	current_map->clients << this;
}

void ClientMapManagement::unloadFromTheMap()
{
	current_map->clients.removeOne(this);
}

//send client only on near map loaded and not the current player
/*QList<ClientMapManagement *> ClientMapManagement::getNearClient()
{
	returnList << current_map->clients;
	/// \todo uncomment and fix this code
	getNearClient_index=0;
	getNearClient_list_size=current_map->clients.size();
	while(getNearClient_index<getNearClient_list_size)
	{
		if(current_map->clients.at(getNearClient_index)!=this)
			returnList << current_map->clients.at(getNearClient_index);
		getNearClient_index++;
	}
}*/

void ClientMapManagement::insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed)
{
	map_management_insert insertClient_temp;
	insertClient_temp.fileName=map->map_file;
	insertClient_temp.id=player_id;
	insertClient_temp.direction=direction;
	insertClient_temp.x=x;
	insertClient_temp.y=y;
	insertClient_temp.speed=speed;
	to_send_map_management_insert << insertClient_temp;
}

/// \note The first heavy function
void ClientMapManagement::moveAnotherClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("moveClient(%1,%2,%3)").arg(player_id).arg(movedUnit).arg(Map_custom::directionToString((Direction)direction)));
	#endif
	moveClient_tempMov.movedUnit=movedUnit;
	moveClient_tempMov.direction=direction;
	to_send_map_management_move[player_id] << moveClient_tempMov;
}

void ClientMapManagement::removeAnotherClient(const quint32 &player_id)
{
	if(to_send_map_management_remove.contains(player_id))
		emit message("try dual remove");
	else
	{
		to_send_map_management_move.remove(player_id);
		to_send_map_management_remove << player_id;
	}
}

void ClientMapManagement::dropAllClients()
{
	to_send_map_management_insert.clear();
	to_send_map_management_move.clear();
	to_send_map_management_remove.clear();
	QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint16)0x000A;
	emit sendPacket(purgeBuffer_outputData);
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
/*		if(purgeBuffer_list_size_internal==0)
			DebugClass::debugConsole(QString("for player: %1, list_size_internal==0").arg(this->player_id));*/
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

