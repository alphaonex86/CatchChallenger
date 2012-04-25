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
	last_direction=static_cast<Direction>(orientation);
	at_start_map_name=map->map_file;

	//store the current information about player on the map
	this->player_id=player_id;
	this->x=x;
	this->y=y;
	this->speed=speed;
	current_map=map;

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
	insertClient();

	loadOnTheMap();
}

/// \note The second heavy function
void ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message(QString("for player (%1,%2): %3, previousMovedUnit: %4, direction: %5").arg(x).arg(y).arg(player_id).arg(previousMovedUnit).arg(directionToString((Direction)direction)));
	#endif
	moveThePlayer_index_move=0;
	if(last_direction==direction)
	{
		emit error(QString("Previous action is same direction: %1").arg(last_direction));
		return;
	}
	mapHaveChanged=false;
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
						mapHaveChanged=true;
						emit message(QString("moveThePlayer(): move at top, leave the map: %1, enter to the map: %2, the enter add x offset=%3").arg(current_map->map_file).arg(current_map->border.top.map->map_file).arg(x-current_map->border.top.x_offset));
						unloadFromTheMap();
						mapVisiblity_unloadFromTheMap();
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
						mapHaveChanged=true;
						emit message(QString("moveThePlayer(): move at right, leave the map: %1, enter to the map: %2, the enter add y offset=%3").arg(current_map->map_file).arg(current_map->border.right.map->map_file).arg(y-current_map->border.right.y_offset));
						unloadFromTheMap();
						mapVisiblity_unloadFromTheMap();
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
						mapHaveChanged=true;
						emit message(QString("moveThePlayer(): move at bottom, leave the map: %1, enter to the map: %2, the enter add x offset=%3").arg(current_map->map_file).arg(current_map->border.bottom.map->map_file).arg(x-current_map->border.bottom.x_offset));
						unloadFromTheMap();
						mapVisiblity_unloadFromTheMap();
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
						mapHaveChanged=true;
						emit message(QString("moveThePlayer(): move at left, leave the map: %1, enter to the map: %2, the enter add y offset=%3").arg(current_map->map_file).arg(current_map->border.left.map->map_file).arg(y-current_map->border.left.y_offset));
						unloadFromTheMap();
						mapVisiblity_unloadFromTheMap();
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

	moveClient(previousMovedUnit,direction,mapHaveChanged);
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

void ClientMapManagement::insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed)
{
	map_management_insert insertClient_temp;
	insertClient_temp.fileName=map->map_file;
	insertClient_temp.direction=direction;
	insertClient_temp.x=x;
	insertClient_temp.y=y;
	insertClient_temp.speed=speed;
	to_send_map_management_insert[player_id]=insertClient_temp;
}

/// \note The first heavy function
/// \todo put extra check to check if into remove
void ClientMapManagement::moveAnotherClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("moveClient(%1,%2,%3)").arg(player_id).arg(movedUnit).arg(directionToString((Direction)direction)));
	#endif

	moveClient_tempMov.movedUnit=movedUnit;
	moveClient_tempMov.direction=direction;
	to_send_map_management_move[player_id] << moveClient_tempMov;
}

void ClientMapManagement::removeAnotherClient(const quint32 &player_id)
{
	#ifdef POKECRAFT_SERVER_EXTRA_CHECK
	if(to_send_map_management_remove.contains(player_id))
	{
		emit message("try dual remove");
		return;
	}
	#endif

	to_send_map_management_move.remove(player_id);
	to_send_map_management_remove << player_id;
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

	i_remove = to_send_map_management_remove.constBegin();
	i_remove_end = to_send_map_management_remove.constEnd();
	while (i_remove != i_remove_end)
	{
		if(stopIt)
			return;
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("player_id to remove: %1, for player: %2")
			.arg(*i_remove)
			.arg(player_id)
			 );
		#endif
		outLoop << *i_remove;
		outLoop << (quint8)0x03;
		purgeBuffer_player_affected++;
	}
	to_send_map_management_remove.clear();

	i_insert = to_send_map_management_insert.constBegin();
	i_insert_end = to_send_map_management_insert.constEnd();
	while (i_insert != i_insert_end)
	{
		if(stopIt)
			return;
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("insert player_id: %1, mapName %2, x: %3, y: %4,direction: %5, for player: %6")
			.arg(i_insert.key())
			.arg(i_insert.value().fileName)
			.arg(i_insert.value().x)
			.arg(i_insert.value().y)
			.arg(directionToString(i_insert.value().direction))
			.arg(player_id)
			 );
		#endif
		outLoop << i_insert.key();
		outLoop << (quint8)0x01;
		outLoop << i_insert.value().fileName;
		outLoop << i_insert.value().x;
		outLoop << i_insert.value().y;
		outLoop << (quint8)i_insert.value().direction;
		outLoop << i_insert.value().speed;
		i_insert++;
		purgeBuffer_player_affected++;
	}
	to_send_map_management_insert.clear();

	purgeBuffer_list_size_internal=0;
	purgeBuffer_indexMovement=0;
	i_move = to_send_map_management_move.constBegin();
	i_move_end = to_send_map_management_move.constEnd();
	while (i_move != i_move_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("move player_id: %1, for player: %2")
			.arg(i_move.key())
			.arg(player_id)
			 );
		#endif
		outLoop << i_move.key();
		outLoop << (quint8)0x02;
		purgeBuffer_list_size_internal=i_move.value().size();
/*		if(purgeBuffer_list_size_internal==0)
			DebugClass::debugConsole(QString("for player: %1, list_size_internal==0").arg(this->player_id));*/
		outLoop << (quint8)purgeBuffer_list_size_internal;
		purgeBuffer_indexMovement=0;
		while(purgeBuffer_indexMovement<purgeBuffer_list_size_internal)
		{
			outLoop << i_move.value().at(purgeBuffer_indexMovement).movedUnit;
			outLoop << (quint8)i_move.value().at(purgeBuffer_indexMovement).direction;
			purgeBuffer_indexMovement++;
		}
		purgeBuffer_player_affected++;
		if(stopIt)
			return;
		++i_move;
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

QString ClientMapManagement::directionToString(const Direction &direction)
{
	switch(direction)
	{
		case Direction_look_at_top:
			return "look at top";
		break;
		case Direction_look_at_right:
			return "look at right";
		break;
		case Direction_look_at_bottom:
			return "look at bottom";
		break;
		case Direction_look_at_left:
			return "look at left";
		break;
		case Direction_move_at_top:
			return "move at top";
		break;
		case Direction_move_at_right:
			return "move at right";
		break;
		case Direction_move_at_bottom:
			return "move at bottom";
		break;
		case Direction_move_at_left:
			return "move at left";
		break;
		default:
		break;
	}
	return "???";
}
