#include "MoveOnTheMap.h"
#include "Map.h"
#include "DebugClass.h"

using namespace Pokecraft;

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

QString MoveOnTheMap::directionToString(const Direction &direction)
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

bool MoveOnTheMap::canGoTo(Direction direction,Map *map,COORD_TYPE x,COORD_TYPE y,bool checkCollision)
{
	if(map==NULL)
		return false;
	switch(direction)
	{
		case Direction_move_at_left:
			if(x>0)
			{
				if(!checkCollision)
					return true;
				return map->parsed_layer.walkable[x-1+y*(map->width)];
			}
			else if(map->border.left.map==NULL)
				return false;
			else if(y<-map->border.left.y_offset)
				return false;
			else
			{
				if(!checkCollision)
					return true;
				return map->border.left.map->parsed_layer.walkable[map->border.left.map->width-1+(y+map->border.left.y_offset)*(map->border.left.map->width)];
			}
		break;
		case Direction_move_at_right:
			if(x<(map->width-1))
			{
				if(!checkCollision)
					return true;
				return map->parsed_layer.walkable[x+1+y*(map->width)];
			}
			else if(map->border.right.map==NULL)
				return false;
			else if(y<-map->border.right.y_offset)
				return false;
			else
			{
				if(!checkCollision)
					return true;
				return map->border.right.map->parsed_layer.walkable[0+(y+map->border.right.y_offset)*(map->border.right.map->width)];
			}
		break;
		case Direction_move_at_top:
			if(y>0)
			{
				if(!checkCollision)
					return true;
				return map->parsed_layer.walkable[x+(y-1)*(map->width)];
			}
			else if(map->border.top.map==NULL)
				return false;
			else if(x<-map->border.top.x_offset)
				return false;
			else
			{
				if(!checkCollision)
					return true;
				return map->border.top.map->parsed_layer.walkable[x+map->border.top.x_offset+(map->border.top.map->height-1)*(map->border.top.map->width)];
			}
		break;
		case Direction_move_at_bottom:
			if(y<(map->height-1))
			{
				if(!checkCollision)
					return true;
				return map->parsed_layer.walkable[x+(y+1)*(map->width)];
			}
			else if(map->border.bottom.map==NULL)
				return false;
			else if(x<-map->border.bottom.x_offset)
				return false;
			else
			{
				if(!checkCollision)
					return true;
                return map->border.bottom.map->parsed_layer.walkable[x+map->border.bottom.x_offset+0];
			}
		break;
		default:
			return false;
	}
}

bool MoveOnTheMap::teleport(Map ** map,COORD_TYPE *x,COORD_TYPE *y)
{
    if((*map)->teleporter.contains(*x+*y*(*map)->width))
	{
        const Map::Teleporter &teleporter=(*map)->teleporter[*x+*y*(*map)->width];
        *x=teleporter.x;
        *y=teleporter.y;
        *map=teleporter.map;
		return true;
	}
	return false;
}

bool MoveOnTheMap::move(Direction direction,Map ** map,COORD_TYPE *x,COORD_TYPE *y)
{
	if(*map==NULL)
		return false;
    if(!canGoTo(direction,*map,*x,*y,true))
		return false;
	switch(direction)
	{
		case Direction_move_at_left:
            if(*x>0)
                *x-=1;
			else
			{
                *x=(*map)->border.left.map->width-1;
                *y+=(*map)->border.left.y_offset;
				*map=(*map)->border.left.map;
			}
            teleport(map,x,y);
			return true;
		break;
		case Direction_move_at_right:
            if(*x<((*map)->width-1))
                *x+=1;
			else
			{
                *x=0;
                *y+=(*map)->border.right.y_offset;
				*map=(*map)->border.right.map;
			}
			teleport(map,x,y);
			return true;
		break;
		case Direction_move_at_top:
            if(*y>0)
                *y-=1;
			else
			{
                *y=(*map)->border.top.map->height-1;
                *x+=(*map)->border.top.x_offset;
				*map=(*map)->border.top.map;
			}
			teleport(map,x,y);
			return true;
		break;
		case Direction_move_at_bottom:
            if(*y<((*map)->height-1))
                *y+=1;
			else
			{
                *y=0;
                *x+=(*map)->border.bottom.x_offset;
				*map=(*map)->border.bottom.map;
			}
			teleport(map,x,y);
			return true;
		break;
		default:
			return false;
	}
}

