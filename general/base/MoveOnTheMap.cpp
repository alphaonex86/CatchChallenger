#include "MoveOnTheMap.hpp"
#include "CommonMap.hpp"
#include "GeneralVariable.hpp"
#include <iostream>

using namespace CatchChallenger;

MoveOnTheMap::MoveOnTheMap()
{
    last_step=255;
    last_direction=Direction_look_at_bottom;
    last_direction_is_set=false;
}

void MoveOnTheMap::setLastDirection(const Direction &the_direction)
{
    if(last_direction_is_set!=false)
        abort();
    last_direction_is_set=true;
    last_direction=the_direction;
    last_step=0;
}

Direction MoveOnTheMap::getDirection() const
{
    if(last_direction_is_set==false)
        abort();
    return last_direction;
}

void MoveOnTheMap::newDirection(const Direction &the_new_direction)
{
    if(last_direction_is_set==false)
    {
        std::cerr << "can't call move now, you need wait firstly the insert_player() with current player (abort)" << std::endl;
        abort();
    }
    #ifdef DEBUG_MESSAGE_MOVEONTHEMAP
    std::cout << "newDirection(" << directionToString(the_new_direction) << "): " << __LINE__ << std::endl;
    #endif
    if(last_direction!=the_new_direction)
    {
        #ifdef DEBUG_MESSAGE_MOVEONTHEMAP
        std::cout << "newDirection(): send_player_move_internal(" << std::to_string(last_step) << "," << directionToString(the_new_direction) << ")" << std::endl;
        #endif
        send_player_move_internal(last_step,the_new_direction);
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
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(last_step>0)
                abort();
            if(last_direction==the_new_direction)
                std::cerr << "dual move dropped" << std::endl;
            #endif
            last_step=0;
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

void MoveOnTheMap::stopMove()
{
    if(last_direction_is_set==false)
        abort();
    switch(last_direction)
    {
        case Direction_move_at_top:
        case Direction_move_at_right:
        case Direction_move_at_bottom:
        case Direction_move_at_left:
            last_direction=(CatchChallenger::Direction)((uint8_t)last_direction-4);
            send_player_move_internal(last_step,last_direction);
            last_step=0;
        break;
        default:
        break;
    }
}

std::string MoveOnTheMap::directionToString(const Direction &direction)
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

bool MoveOnTheMap::canGoTo(const Direction &direction,const CommonMap &map,const COORD_TYPE &x,const COORD_TYPE &y,const bool &checkCollision, const bool &allowTeleport)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
            abort();//wrong direction send
    }
    #endif
    switch(direction)
    {
        case Direction_move_at_top:
            if(y>0)
            {
                if(checkCollision)
                    if(!isWalkableWithDirection(map,x,y-1,direction))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x,y-1))
                        return false;
                return true;
            }
            else if(map.border.top.mapIndex==65535)
                return false;
            else if(x<-map.border.top.x_offset)
                return false;
            else
            {
                if(checkCollision)
                {
                    const CommonMap * const mapTop=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.top.mapIndex));
                    if(!isWalkableWithDirection(*mapTop,x+static_cast<uint8_t>(map.border.top.x_offset),mapTop->height-1,direction))
                        return false;
                }
                if(!allowTeleport)
                {
                    const CommonMap * const mapTop=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.top.mapIndex));
                    if(needBeTeleported(*mapTop,x+static_cast<uint8_t>(map.border.top.x_offset),mapTop->height-1))
                        return false;
                }
                return true;
            }
        break;
        case Direction_move_at_right:
            if(x<(map.width-1))
            {
                if(checkCollision)
                    if(!isWalkableWithDirection(map,x+1,y,direction))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x+1,y))
                        return false;
                return true;
            }
            else if(map.border.right.mapIndex==65535)
                return false;
            else if(y<-map.border.right.y_offset)
                return false;
            else
            {
                if(checkCollision)
                {
                    const CommonMap * const mapRight=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.right.mapIndex));
                    if(!isWalkableWithDirection(*mapRight,0,y+static_cast<int8_t>(map.border.right.y_offset),direction))
                        return false;
                }
                if(!allowTeleport)
                {
                    const CommonMap * const mapRight=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.right.mapIndex));
                    if(needBeTeleported(*mapRight,0,y+static_cast<int8_t>(map.border.right.y_offset)))
                        return false;
                }
                return true;
            }
        break;
        case Direction_move_at_bottom:
            if(y<(map.height-1))
            {
                if(checkCollision)
                    if(!isWalkableWithDirection(map,x,y+1,direction))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x,y+1))
                        return false;
                return true;
            }
            else if(map.border.bottom.mapIndex==65535)
                return false;
            else if(x<-map.border.bottom.x_offset)
                return false;
            else
            {
                if(checkCollision)
                {
                    const CommonMap * const mapBottom=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.bottom.mapIndex));
                    if(!isWalkableWithDirection(*mapBottom,x+static_cast<uint8_t>(map.border.bottom.x_offset),0,direction))
                        return false;
                }
                if(!allowTeleport)
                {
                    const CommonMap * const mapBottom=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.bottom.mapIndex));
                    if(needBeTeleported(*mapBottom,x+static_cast<uint8_t>(map.border.bottom.x_offset),0))
                        return false;
                }
                return true;
            }
        break;
        case Direction_move_at_left:
            if(x>0)
            {
                if(checkCollision)
                    if(!isWalkableWithDirection(map,x-1,y,direction))
                        return false;
                if(!allowTeleport)
                    if(needBeTeleported(map,x-1,y))
                        return false;
                return true;
            }
            else if(map.border.left.mapIndex==65535)
                return false;
            else if(y<-map.border.left.y_offset)
                return false;
            else
            {
                if(checkCollision)
                {
                    const CommonMap * const mapLeft=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.left.mapIndex));
                    if(!isWalkableWithDirection(*mapLeft,mapLeft->width-1,y+static_cast<int8_t>(map.border.left.y_offset),direction))
                        return false;
                }
                if(!allowTeleport)
                {
                    const CommonMap * const mapLeft=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map.border.left.mapIndex));
                    if(needBeTeleported(*mapLeft,mapLeft->width-1,y+static_cast<int8_t>(map.border.left.y_offset)))
                        return false;
                }
                return true;
            }
        break;
        default:
            return false;
    }
    return false;
}

CatchChallenger::ParsedLayerLedges MoveOnTheMap::getLedge(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size)
    {
        std::cerr << "MoveOnTheMap::getLedge() map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif
    const uint8_t &i=*(CommonMap::flat_simplified_map+map.parsed_layer.simplifiedMapIndex+x+y*map.width);
    if(i<250 || i>253)
        return CatchChallenger::ParsedLayerLedges_NoLedges;
    return static_cast<ParsedLayerLedges>((uint32_t)i-250+1);
}

bool MoveOnTheMap::teleport(CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y)
{
    const CommonMap * const map=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(mapIndex));
    const uint8_t &teleporter_list_size=map->teleporter_list_size;
    if(teleporter_list_size<=0)
        return false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonMap::flat_teleporter==nullptr)
    {
        std::cerr << "MoveOnTheMap::teleport() CommonMap::flat_teleporter==nullptr" << std::endl;
        abort();
    }
    #endif
    const CommonMap::Teleporter * const teleporter=CommonMap::flat_teleporter+map->teleporter_first_index;
    uint8_t index=0;
    while(index<teleporter_list_size)
    {
        if(teleporter[index].source_x==x && teleporter[index].source_y==y)
        {
            x=teleporter[index].destination_x;
            y=teleporter[index].destination_y;
            mapIndex=teleporter[index].mapIndex;
            return true;
        }
        index++;
    }
    return false;
}

int8_t MoveOnTheMap::indexOfTeleporter(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    const CommonMap::Teleporter * const teleporter=CommonMap::flat_teleporter+map.teleporter_first_index;;
    uint8_t index=0;
    while(index<map.teleporter_list_size)
    {
        if(teleporter[index].source_x==x && teleporter[index].source_y==y)
            return index;
        index++;
    }
    return -1;
}

bool MoveOnTheMap::needBeTeleported(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    const CommonMap::Teleporter * const teleporter=CommonMap::flat_teleporter+map.teleporter_first_index;;
    int8_t index=0;
    while(index<map.teleporter_list_size)
    {
        if(teleporter[index].source_x==x && teleporter[index].source_y==y)
            return true;
        index++;
    }
    return false;
}

bool MoveOnTheMap::isWalkable(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    //exclude here ParsedLayerLedges, because don't have info about direction
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size)
    {
        std::cerr << "MoveOnTheMap::isWalkable() map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif
    return *(CommonMap::flat_simplified_map+map.parsed_layer.simplifiedMapIndex+x+y*map.width)<200;
}

bool MoveOnTheMap::isWalkableWithDirection(const CommonMap &map, const uint8_t &x, const uint8_t &y,const CatchChallenger::Direction &direction)
{
    //exclude here ParsedLayerLedges, because don't have info about direction
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size)
    {
        std::cerr << "MoveOnTheMap::isWalkableWithDirection() map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif
    const uint8_t &val=*(CommonMap::flat_simplified_map+map.parsed_layer.simplifiedMapIndex+x+y*map.width);
    if(val<200)
        return true;
    switch(direction)
    {
        case Direction_look_at_left:
        case Direction_move_at_left:
            if(val==250)
                return true;
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            if(val==251)
                return true;
        break;
        case Direction_look_at_top:
        case Direction_move_at_top:
            if(val==252)
                return true;
        break;
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            if(val==253)
                return true;
        break;
        default:
        return false;
    }
    return false;
}

bool MoveOnTheMap::isDirt(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size)
    {
        std::cerr << "MoveOnTheMap::isDirt() map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif
    return *(CommonMap::flat_simplified_map+map.parsed_layer.simplifiedMapIndex+x+y*map.width)==249;
}

MonstersCollisionValue MoveOnTheMap::getZoneCollision(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size)
    {
        std::cerr << "MoveOnTheMap::getZoneCollision() map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif
    const uint8_t &val=*(CommonMap::flat_simplified_map+map.parsed_layer.simplifiedMapIndex+x+y*map.width);
    if(map.parsed_layer.monstersCollisionList.size()<=val)
        return MonstersCollisionValue();
    return map.parsed_layer.monstersCollisionList.at(val);
}

bool MoveOnTheMap::move(Direction direction, CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y, const bool &checkCollision, const bool &allowTeleport)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
            abort();//wrong direction send
    }
    #endif
    if(!moveWithoutTeleport(direction,mapIndex,x,y,checkCollision,allowTeleport))
        return false;
    teleport(mapIndex,x,y);
    return true;
}

bool MoveOnTheMap::moveWithoutTeleport(Direction direction, CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y, const bool &checkCollision, const bool &allowTeleport)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
            abort();//wrong direction send
    }
    #endif
    if(mapIndex==65535)
        return false;
    const CommonMap * const map=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(mapIndex));
    if(!canGoTo(direction,*map,x,y,checkCollision,allowTeleport))
        return false;
    switch(direction)
    {
        case Direction_move_at_top:
            if(y>0)
                y-=1;
            else
            {
                const CommonMap * const mapTop=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map->border.top.mapIndex));
                y=mapTop->height-1;
                x+=map->border.top.x_offset;
                mapIndex=map->border.top.mapIndex;
            }
            return true;
        break;
        case Direction_move_at_right:
            if(x<(map->width-1))
                x+=1;
            else
            {
                x=0;
                y+=map->border.right.y_offset;
                mapIndex=map->border.right.mapIndex;
            }
            return true;
        break;
        case Direction_move_at_bottom:
            if(y<(map->height-1))
                y+=1;
            else
            {
                y=0;
                x+=map->border.bottom.x_offset;
                mapIndex=map->border.bottom.mapIndex;
            }
            return true;
        break;
        case Direction_move_at_left:
            if(x>0)
                x-=1;
            else
            {
                const CommonMap * const mapLeft=reinterpret_cast<const CommonMap *>(CommonMap::indexToMap(map->border.left.mapIndex));
                x=mapLeft->width-1;
                y+=map->border.left.y_offset;
                mapIndex=map->border.left.mapIndex;
            }
            return true;
        break;
        default:
            return false;
    }
}
