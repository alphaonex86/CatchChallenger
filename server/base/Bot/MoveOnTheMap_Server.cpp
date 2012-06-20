#include "MoveOnTheMap_Server.h"

MoveOnTheMap_Server::MoveOnTheMap_Server()
{
	map=NULL;
}

bool MoveOnTheMap_Server::canGoTo(Direction direction)
{
	if(map==NULL)
		return false;
	switch(direction)
	{
		case Direction_move_at_left:
			if(x>0)
				return map->parsed_layer.walkable[x-1+y*map->width];
			else if(map->border.left.map==NULL)
				return false;
			else if(y<-map->border.left.y_offset)
				return false;
			else
				return map->border.left.map->parsed_layer.walkable[map->border.left.map->width-1+(y+map->border.left.y_offset)*map->border.left.map->width];
		break;
		case Direction_move_at_right:
			if(x<map->width)
				return map->parsed_layer.walkable[x+1+y*map->width];
			else if(map->border.right.map==NULL)
				return false;
			else if(y<-map->border.right.y_offset)
				return false;
			else
				return map->border.right.map->parsed_layer.walkable[0+(y+map->border.right.y_offset)*map->border.right.map->width];
		break;
		case Direction_move_at_top:
			if(y>0)
				return map->parsed_layer.walkable[x+(y-1)*map->width];
			else if(map->border.top.map==NULL)
				return false;
			else if(x<-map->border.top.x_offset)
				return false;
			else
				return map->border.top.map->parsed_layer.walkable[x+map->border.top.x_offset+(map->border.top.map->height-1)*map->border.top.map->width];
		break;
		case Direction_move_at_bottom:
			if(y<map->height)
				return map->parsed_layer.walkable[x+(y+1)*map->width];
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

void MoveOnTheMap_Server::move(Direction direction)
{
	if(map==NULL)
		return;
	if(!canGoTo(direction))
		return;
	switch(direction)
	{
		case Direction_move_at_left:
			if(x>0)
			{
				x-=1;
				return;
			}
			else
			{
				x=map->border.left.map->width-1;
				y+=map->border.left.y_offset;
				map=static_cast<Map_server *>(map->border.left.map);
				return;
			}
		break;
		case Direction_move_at_right:
			if(x<map->width)
			{
				x+=1;
				return;
			}
			else
			{
				x=0;
				y+=map->border.right.y_offset;
				map=static_cast<Map_server *>(map->border.right.map);
				return;
			}
		break;
		case Direction_move_at_top:
			if(y>0)
			{
				y-=1;
				return;
			}
			else
			{
				y=map->border.top.map->height-1;
				x+=map->border.top.x_offset;
				map=static_cast<Map_server *>(map->border.top.map);
				return;
			}
		break;
		case Direction_move_at_bottom:
			if(y<map->height)
			{
				y+=1;
				return;
			}
			else
			{
				y=0;
				x+=map->border.bottom.x_offset;
				map=static_cast<Map_server *>(map->border.bottom.map);
				return;
			}
		break;
		default:
			return;
	}
}
