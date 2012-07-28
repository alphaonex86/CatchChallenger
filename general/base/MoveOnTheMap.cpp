#include "MoveOnTheMap.h"
#include "Map.h"
#include "DebugClass.h"

MoveOnTheMap::MoveOnTheMap()
{
	last_direction=Direction_look_at_bottom;
	last_step=0;
}

void MoveOnTheMap::newDirection(const Direction &the_new_direction)
{
	if(last_direction!=the_new_direction)
	{
		send_player_move(last_step,the_new_direction);
		last_step=0;
		last_direction=the_new_direction;
	}
	else
	{
		switch(the_new_direction)
		{
			case Direction_look_at_top:
			case Direction_look_at_right:
			case Direction_look_at_bottom:
			case Direction_look_at_left:
			//to drop the dual same trame
			return;
			break;
			default:
			break;
		}
	}
	switch(the_new_direction)
	{
		case Direction_move_at_top:
		case Direction_move_at_right:
		case Direction_move_at_bottom:
		case Direction_move_at_left:
			last_step++;
		break;
		default:
		break;
	}
}

bool MoveOnTheMap::canGoTo(Direction direction,Map *map,COORD_TYPE x,COORD_TYPE y)
{
	if(map==NULL)
		return false;
	switch(direction)
	{
		case Direction_move_at_left:
			if(x>0)
				return map->parsed_layer.walkable[x-1+y*(map->width)];
			else if(map->border.left.map==NULL)
				return false;
			else if(y<-map->border.left.y_offset)
				return false;
			else
				return map->border.left.map->parsed_layer.walkable[map->border.left.map->width-1+(y+map->border.left.y_offset)*(map->border.left.map->width)];
		break;
		case Direction_move_at_right:
			if(x<(map->width-1))
				return map->parsed_layer.walkable[x+1+y*(map->width)];
			else if(map->border.right.map==NULL)
				return false;
			else if(y<-map->border.right.y_offset)
				return false;
			else
				return map->border.right.map->parsed_layer.walkable[0+(y+map->border.right.y_offset)*(map->border.right.map->width)];
		break;
		case Direction_move_at_top:
			if(y>0)
				return map->parsed_layer.walkable[x+(y-1)*(map->width)];
			else if(map->border.top.map==NULL)
				return false;
			else if(x<-map->border.top.x_offset)
				return false;
			else
				return map->border.top.map->parsed_layer.walkable[x+map->border.top.x_offset+(map->border.top.map->height-1)*(map->border.top.map->width)];
		break;
		case Direction_move_at_bottom:
			if(y<(map->height-1))
				return map->parsed_layer.walkable[x+(y+1)*(map->width)];
			else if(map->border.bottom.map==NULL)
				return false;
			else if(x<-map->border.bottom.x_offset)
				return false;
			else
				return map->border.bottom.map->parsed_layer.walkable[x+map->border.top.x_offset+0];
		break;
		default:
			return false;
	}
}

void MoveOnTheMap::teleport(Map ** map,COORD_TYPE &x,COORD_TYPE &y)
{
	DebugClass::debugConsole(QString("FakeBot::teleport(), map: %1 (%2,%3), teleporter: %4").arg((*map)->map_file).arg(x).arg(y).arg((*map)->teleporter.size()));
	if((*map)->teleporter.contains(x+y*(*map)->width))
	{
		const Map::Teleporter &teleporter=(*map)->teleporter[x+y*(*map)->width];
		DebugClass::debugConsole(QString("FakeBot::teleport(), map: %1 (%2,%3), to: %4 (%5,%6)").arg((*map)->map_file).arg(x).arg(y).arg(teleporter.map->map_file).arg(teleporter.x).arg(teleporter.y));
		x=teleporter.x;
		y=teleporter.y;
		(*map)=teleporter.map;
	}
}

bool MoveOnTheMap::move(Direction direction,Map ** map,COORD_TYPE &x,COORD_TYPE &y)
{
	if(*map==NULL)
		return false;
	if(!canGoTo(direction,*map,x,y))
		return false;
	switch(direction)
	{
		case Direction_move_at_left:
			if(x>0)
				x-=1;
			else
			{
				x=(*map)->border.left.map->width-1;
				y+=(*map)->border.left.y_offset;
				*map=(*map)->border.left.map;
			}
			teleport(map,x,y);
			return true;
		break;
		case Direction_move_at_right:
			if(x<((*map)->width-1))
				x+=1;
			else
			{
				x=0;
				y+=(*map)->border.right.y_offset;
				*map=(*map)->border.right.map;
			}
			teleport(map,x,y);
			return true;
		break;
		case Direction_move_at_top:
			if(y>0)
				y-=1;
			else
			{
				y=(*map)->border.top.map->height-1;
				x+=(*map)->border.top.x_offset;
				*map=(*map)->border.top.map;
			}
			teleport(map,x,y);
			return true;
		break;
		case Direction_move_at_bottom:
			if(y<((*map)->height-1))
				y+=1;
			else
			{
				y=0;
				x+=(*map)->border.bottom.x_offset;
				*map=(*map)->border.bottom.map;
			}
			teleport(map,x,y);
			return true;
		break;
		default:
			return false;
	}
}

