#include "MoveOnTheMap.h"
#include "CommonMap.h"
#include "DebugClass.h"

using namespace CatchChallenger;

MoveOnTheMap::MoveOnTheMap()
{
    setLastDirection(Direction_look_at_bottom);
}

void MoveOnTheMap::setLastDirection(const Direction &the_direction)
{
    last_direction=the_direction;
    last_step=0;
}

void MoveOnTheMap::newDirection(const Direction &the_new_direction)
{
    #ifdef DEBUG_MESSAGE_MOVEONTHEMAP
    qDebug() << QString("newDirection(%1)").arg(directionToString(the_new_direction));
    #endif
    if(last_direction!=the_new_direction)
    {
        #ifdef DEBUG_MESSAGE_MOVEONTHEMAP
        qDebug() << QString("send_player_move(%1,%2)").arg(last_step).arg(directionToString(the_new_direction));
        #endif
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

Orientation MoveOnTheMap::directionToOrientation(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
            return Orientation_top;
        break;
        case Direction_look_at_right:
            return Orientation_right;
        break;
        case Direction_look_at_bottom:
            return Orientation_bottom;
        break;
        case Direction_look_at_left:
            return Orientation_left;
        break;
        case Direction_move_at_top:
            return Orientation_top;
        break;
        case Direction_move_at_right:
            return Orientation_right;
        break;
        case Direction_move_at_bottom:
            return Orientation_bottom;
        break;
        case Direction_move_at_left:
            return Orientation_left;
        break;
        default:
        break;
    }
    return Orientation_bottom;
}

Direction MoveOnTheMap::directionToDirectionLook(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            return direction;
        break;
        case Direction_move_at_top:
            return Direction_look_at_top;
        break;
        case Direction_move_at_right:
            return Direction_look_at_right;
        break;
        case Direction_move_at_bottom:
            return Direction_look_at_bottom;
        break;
        case Direction_move_at_left:
            return Direction_look_at_left;
        break;
        default:
        break;
    }
    return Direction_look_at_bottom;
}

bool MoveOnTheMap::canGoTo(const Direction &direction,const Map &map,const COORD_TYPE &x,const COORD_TYPE &y,const bool &checkCollision, const bool &allowTeleport)
{
    switch(direction)
    {
        case Direction_move_at_left:
            if(x>0)
            {
                if(checkCollision)
                    if(!isWalkable(map,x-1,y))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x-1,y))
                        return false;
                return true;
            }
            else if(map.border.left.map==NULL)
                return false;
            else if(y<-map.border.left.y_offset)
                return false;
            else
            {
                if(checkCollision)
                    if(!isWalkable(*map.border.left.map,map.border.left.map->width-1,y+map.border.left.y_offset))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(*map.border.left.map,map.border.left.map->width-1,y+map.border.left.y_offset))
                        return false;
                return true;
            }
        break;
        case Direction_move_at_right:
            if(x<(map.width-1))
            {
                if(checkCollision)
                    if(!isWalkable(map,x+1,y))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x+1,y))
                        return false;
                return true;
            }
            else if(map.border.right.map==NULL)
                return false;
            else if(y<-map.border.right.y_offset)
                return false;
            else
            {
                if(checkCollision)
                    if(!isWalkable(*map.border.right.map,0,y+map.border.right.y_offset))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(*map.border.right.map,0,y+map.border.right.y_offset))
                        return false;
                return true;
            }
        break;
        case Direction_move_at_top:
            if(y>0)
            {
                if(checkCollision)
                    if(!isWalkable(map,x,y-1))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x,y-1))
                        return false;
                return true;
            }
            else if(map.border.top.map==NULL)
                return false;
            else if(x<-map.border.top.x_offset)
                return false;
            else
            {
                if(checkCollision)
                    if(!isWalkable(*map.border.top.map,x+map.border.top.x_offset,map.border.top.map->height-1))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(*map.border.top.map,x+map.border.top.x_offset,map.border.top.map->height-1))
                        return false;
                return true;
            }
        break;
        case Direction_move_at_bottom:
            if(y<(map.height-1))
            {
                if(checkCollision)
                    if(!isWalkable(map,x,y+1))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x,y+1))
                        return false;
                return true;
            }
            else if(map.border.bottom.map==NULL)
                return false;
            else if(x<-map.border.bottom.x_offset)
                return false;
            else
            {
                if(checkCollision)
                    if(!isWalkable(*map.border.bottom.map,x+map.border.bottom.x_offset,0))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(*map.border.bottom.map,x+map.border.bottom.x_offset,0))
                        return false;
                return true;
            }
        break;
        default:
            return false;
    }
    return false;
}

CatchChallenger::ParsedLayerLedges MoveOnTheMap::getLedge(const Map &map, const quint8 &x, const quint8 &y)
{
    if(map.parsed_layer.ledges==NULL)
        return CatchChallenger::ParsedLayerLedges_NoLedges;
    quint8 i=map.parsed_layer.ledges[x+y*(map.width)];
    return static_cast<ParsedLayerLedges>((quint32)i);
}

bool MoveOnTheMap::haveGrass(const Map &map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(map.parsed_layer.grass==NULL)
        return false;
    else
        return map.parsed_layer.grass[x+y*(map.width)];
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

bool MoveOnTheMap::needBeTeleported(const Map &map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    return map.teleporter.contains(x+y*map.width);
}

bool MoveOnTheMap::isWalkable(const Map &map, const quint8 &x, const quint8 &y)
{
    if(map.parsed_layer.walkable==NULL)
        return false;
    return map.parsed_layer.walkable[x+y*(map.width)];
}

bool MoveOnTheMap::isWater(const Map &map, const quint8 &x, const quint8 &y)
{
    if(map.parsed_layer.water==NULL)
        return false;
    return map.parsed_layer.water[x+y*(map.width)];
}

bool MoveOnTheMap::isDirt(const Map &map, const quint8 &x, const quint8 &y)
{
    if(map.parsed_layer.dirt==NULL)
        return false;
    return map.parsed_layer.dirt[x+y*(map.width)];
}

bool MoveOnTheMap::isGrass(const Map &map, const quint8 &x, const quint8 &y)
{
    if(map.parsed_layer.grass==NULL)
        return false;
    return map.parsed_layer.grass[x+y*(map.width)];
}

bool MoveOnTheMap::move(Direction direction, Map ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport)
{
    if(!moveWithoutTeleport(direction,map,x,y,checkCollision,allowTeleport))
        return false;
    teleport(map,x,y);
    return true;
}

bool MoveOnTheMap::moveWithoutTeleport(Direction direction, Map ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport)
{
    if(*map==NULL)
        return false;
    if(!canGoTo(direction,**map,*x,*y,checkCollision,allowTeleport))
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
            return true;
        break;
        default:
            return false;
    }
}
