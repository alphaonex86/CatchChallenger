#include "MapBasicMove.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

MapBasicMove::MapBasicMove()
{
	current_map=NULL;
	player_informations=NULL;
	generalData=NULL;
	stopIt=false;
}

MapBasicMove::~MapBasicMove()
{
	stopIt=true;
}

void MapBasicMove::setVariable(GeneralData *generalData,Player_private_and_public_informations *player_informations)
{
	this->generalData=generalData;
	this->player_informations=player_informations;
}

void MapBasicMove::askIfIsReadyToStop()
{
	if(stopIt)
		return;
	stopIt=true;
	if(current_map==NULL)
	{
		emit isReadyToStop();
		return;
	}
	unloadFromTheMap();

	extraStop();

	current_map=NULL;
	emit isReadyToStop();
}

void MapBasicMove::extraStop()
{
}

void MapBasicMove::stop()
{
	deleteLater();
}

void MapBasicMove::put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	//store the starting informations
	last_direction=static_cast<Direction>(orientation);

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
}

void MapBasicMove::mapTeleporterUsed()
{
	const Map_final::Teleporter &teleporter=current_map->teleporter[x+y*current_map->width];
	if(teleporter.map==current_map)
	{
		x=teleporter.x;
		y=teleporter.y;
		emit message(QString("moveThePlayer(): pass on local teleporter (%1,%2)").arg(x).arg(y));
	}
	else
	{
		unloadFromTheMap();
		x=teleporter.x;
		y=teleporter.y;
		current_map=teleporter.map;
		emit message(QString("moveThePlayer(): pass on remote teleporter %1 (%2,%3)").arg(current_map->map_file).arg(x).arg(y));
		loadOnTheMap();
	}
}

bool MapBasicMove::checkCollision()
{
	return true;
}

/// \note The second heavy function
void MapBasicMove::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message(QString("for player (%1,%2): %3, previousMovedUnit: %4, direction: %5").arg(x).arg(y).arg(player_id).arg(previousMovedUnit).arg(directionToString((Direction)direction)));
	#endif
	moveThePlayer_index_move=0;
	stopCurrentMethod=false;
	if(last_direction==direction)
	{
		emit error(QString("Previous action is same direction: %1").arg(last_direction));
		stopCurrentMethod=true;
		return;
	}
	switch(last_direction)
	{
		case Direction_move_at_top:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_top: Previous action is moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if(0==y)
				{
					if(current_map->border.top.map==NULL)
					{
						emit error(QString("moveThePlayer(), out of map: %1 by top").arg(current_map->map_file));
						stopCurrentMethod=true;
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at top, leave the map: %1, enter to the map: %2, the enter add x offset=%3").arg(current_map->map_file).arg(current_map->border.top.map->map_file).arg(x-current_map->border.top.x_offset));
						unloadFromTheMap();
						y=current_map->border.top.map->height;
						x+=current_map->border.top.x_offset;
						current_map=current_map->border.top.map;
						loadOnTheMap();
					}
					return;
				}
				else
					y--;
				if(!checkCollision())
					return;
				if(current_map->teleporter.contains(x+y*current_map->width))
					mapTeleporterUsed();
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_top:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_top: Previous action is not moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
		break;
		case Direction_move_at_right:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_right: Previous action is moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if((current_map->width-1)==x)
				{
					if(current_map->border.right.map==NULL)
					{
						emit error(QString("moveThePlayer(): move at right, out of map: %1, by right").arg(current_map->map_file));
						stopCurrentMethod=true;
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at right, leave the map: %1, enter to the map: %2, the enter add y offset=%3").arg(current_map->map_file).arg(current_map->border.right.map->map_file).arg(y-current_map->border.right.y_offset));
						unloadFromTheMap();
						y=0;
						x+=current_map->border.right.y_offset;
						current_map=current_map->border.right.map;
						loadOnTheMap();
					}
					return;
				}
				else
					x++;
				if(!checkCollision())
					return;
				if(current_map->teleporter.contains(x+y*current_map->width))
					mapTeleporterUsed();
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_right:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_right: Previous action is not moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
		break;
		case Direction_move_at_bottom:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_bottom: Previous action is moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if((current_map->height-1)==y)
				{
					if(current_map->border.bottom.map==NULL)
					{
						emit error(QString("moveThePlayer(): move at bottom, out of map: %1, by bottom").arg(current_map->map_file));
						stopCurrentMethod=true;
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at bottom, leave the map: %1, enter to the map: %2, the enter add x offset=%3").arg(current_map->map_file).arg(current_map->border.bottom.map->map_file).arg(x-current_map->border.bottom.x_offset));
						unloadFromTheMap();
						y=0;
						x+=current_map->border.bottom.x_offset;
						current_map=current_map->border.bottom.map;
						loadOnTheMap();
					}
					return;
				}
				else
					y++;
				if(!checkCollision())
					return;
				if(current_map->teleporter.contains(x+y*current_map->width))
					mapTeleporterUsed();
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_bottom:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_bottom: Previous action is not moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
		break;
		case Direction_move_at_left:
			if(previousMovedUnit==0)
			{
				emit error(QString("Direction_move_at_left: Previous action is moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
			while(moveThePlayer_index_move<previousMovedUnit)
			{
				if(0==x)
				{
					if(current_map->border.left.map==NULL)
					{
						emit error(QString("moveThePlayer(): move at left, out of map: %1, by left").arg(current_map->map_file));
						stopCurrentMethod=true;
						return;
					}
					else
					{
						emit message(QString("moveThePlayer(): move at left, leave the map: %1, enter to the map: %2, the enter add y offset=%3").arg(current_map->map_file).arg(current_map->border.left.map->map_file).arg(y-current_map->border.left.y_offset));
						unloadFromTheMap();
						y=current_map->border.left.map->width;
						x+=current_map->border.left.y_offset;
						current_map=current_map->border.left.map;
						loadOnTheMap();
					}
					return;
				}
				else
					x--;
				if(!checkCollision())
					return;
				if(current_map->teleporter.contains(x+y*current_map->width))
					mapTeleporterUsed();
				moveThePlayer_index_move++;
			}
		break;
		case Direction_look_at_left:
			if(previousMovedUnit>0)
			{
				emit error(QString("Direction_look_at_left: Previous action is not moving: %1").arg(last_direction));
				stopCurrentMethod=true;
				return;
			}
		break;
		default:
			emit error(QString("moveThePlayer(): direction not managed"));
			stopCurrentMethod=true;
			return;
		break;
	}
	last_direction=direction;
}

void MapBasicMove::loadOnTheMap()
{
}

void MapBasicMove::unloadFromTheMap()
{
}

QString MapBasicMove::directionToString(const Direction &direction)
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
