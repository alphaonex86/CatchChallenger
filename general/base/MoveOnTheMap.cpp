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

int8_t MoveOnTheMap::indexOfTeleporter(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    if(map.teleporters.size()<=0)
        return -1;
    uint8_t index=0;
    while(index<map.teleporters.size())
    {
        const Teleporter &teleporter=map.teleporters.at(index);
        if(teleporter.source_x==x && teleporter.source_y==y)
            return index;
        index++;
    }
    return -1;
}

bool MoveOnTheMap::needBeTeleported(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    if(map.teleporters.size()<=0)
        return false;
    uint8_t index=0;
    while(index<map.teleporters.size())
    {
        const Teleporter &teleporter=map.teleporters.at(index);
        if(teleporter.source_x==x && teleporter.source_y==y)
            return true;
        index++;
    }
    return false;
}

bool MoveOnTheMap::isWalkable(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    //exclude here ParsedLayerLedges, because don't have info about direction
    return map.flat_simplified_map.at(x+y*map.width)<200;
}

uint8_t MoveOnTheMap::getMapZoneCode(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    return map.flat_simplified_map.at(x+y*map.width);
}

CatchChallenger::ParsedLayerLedges MoveOnTheMap::getLedge(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    const uint8_t &i=map.flat_simplified_map.at(x+y*map.width);
    if(i<250 || i>253)
        return CatchChallenger::ParsedLayerLedges_NoLedges;
    return static_cast<ParsedLayerLedges>((uint32_t)i-250+1);
}

bool MoveOnTheMap::isWalkableWithDirection(const CommonMap &map, const uint8_t &x, const uint8_t &y,const CatchChallenger::Direction &direction)
{
    //exclude here ParsedLayerLedges, because don't have info about direction
    const uint8_t &val=map.flat_simplified_map.at(x+y*map.width);
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
    return map.flat_simplified_map.at(x+y*map.width)==249;
}

MonstersCollisionValue MoveOnTheMap::getZoneCollision(const CommonMap &map, const uint8_t &x, const uint8_t &y)
{
    const uint8_t &val=map.flat_simplified_map.at(x+y*map.width);
    if(map.zones.size()<=val)
        return MonstersCollisionValue();
    return map.zones.at(val);
}
