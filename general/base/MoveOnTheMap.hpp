#ifndef CATCHCHALLENGER_MOVEONTHEMAP_H
#define CATCHCHALLENGER_MOVEONTHEMAP_H

#include <string>

#include "GeneralStructures.hpp"
#include "CommonMap.hpp"
#include "lib.h"

//to group the step by step move into line move
/* template <typename TMap = Map>
class MoveOnTheMap {
   TMap * map_;*/

namespace CatchChallenger {

class CommonMap;

class DLL_PUBLIC MoveOnTheMap
{
public:
    MoveOnTheMap();
    virtual void newDirection(const Direction &the_direction);
    virtual void setLastDirection(const Direction &the_direction);
    void stopMove();
    //debug function
    static std::string directionToString(const Direction &direction);
    static Orientation directionToOrientation(const Direction &direction);
    static Direction directionToDirectionLook(const Direction &direction);

    //this map list access
    template<class MapType>
    static bool canGoTo(const std::vector<MapType> &mapList,const Direction &direction,const MapType &map,const COORD_TYPE &x,const COORD_TYPE &y,const bool &checkCollision, const bool &allowTeleport=true)
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
                    const MapType &mapTop=mapList.at(map.border.top.mapIndex);
                    if(checkCollision)
                    {
                        if(!isWalkableWithDirection(mapTop,x+static_cast<uint8_t>(map.border.top.x_offset),mapTop.height-1,direction))
                            return false;
                    }
                    if(!allowTeleport)
                    {
                        if(needBeTeleported(mapTop,x+static_cast<uint8_t>(map.border.top.x_offset),mapTop.height-1))
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
                    const MapType &mapRight=mapList.at(map.border.right.mapIndex);
                    if(checkCollision)
                    {
                        if(!isWalkableWithDirection(mapRight,0,y+static_cast<int8_t>(map.border.right.y_offset),direction))
                            return false;
                    }
                    if(!allowTeleport)
                    {
                        if(needBeTeleported(mapRight,0,y+static_cast<int8_t>(map.border.right.y_offset)))
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
                    const MapType &mapBottom=mapList.at(map.border.bottom.mapIndex);
                    if(checkCollision)
                    {
                        if(!isWalkableWithDirection(mapBottom,x+static_cast<uint8_t>(map.border.bottom.x_offset),0,direction))
                            return false;
                    }
                    if(!allowTeleport)
                    {
                        if(needBeTeleported(mapBottom,x+static_cast<uint8_t>(map.border.bottom.x_offset),0))
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
                    const MapType &mapLeft=mapList.at(map.border.left.mapIndex);
                    if(checkCollision)
                    {
                        if(!isWalkableWithDirection(mapLeft,mapLeft.width-1,y+static_cast<int8_t>(map.border.left.y_offset),direction))
                            return false;
                    }
                    if(!allowTeleport)
                    {
                        if(needBeTeleported(mapLeft,mapLeft.width-1,y+static_cast<int8_t>(map.border.left.y_offset)))
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
    template<class MapType>
    static bool teleport(const std::vector<MapType> &mapList,CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y)
    {
        const MapType &map=mapList.at(mapIndex);
        if(map.teleporters.size()<=0)
            return false;
        uint8_t index=0;
        while(index<map.teleporters.size())
        {
            const Teleporter &teleporter=map.teleporters.at(index);
            if(teleporter.source_x==x && teleporter.source_y==y)
            {
                x=teleporter.destination_x;
                y=teleporter.destination_y;
                mapIndex=teleporter.mapIndex;
                return true;
            }
            index++;
        }
        return false;
    }

    template<class MapType>
    static bool move(const std::vector<MapType> &mapList,const Direction &direction, CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y, const bool &checkCollision, const bool &allowTeleport=true)
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
        if(!moveWithoutTeleport(mapList,direction,mapIndex,x,y,checkCollision,allowTeleport))
            return false;
        teleport(mapList,mapIndex,x,y);
        return true;
    }

    template<class MapType>
    static bool moveWithoutTeleport(const std::vector<MapType> &mapList,const Direction &direction, CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y, const bool &checkCollision, const bool &allowTeleport=true)
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
        const MapType &map=mapList.at(mapIndex);
        if(!canGoTo(mapList,direction,map,x,y,checkCollision,allowTeleport))
            return false;
        switch(direction)
        {
            case Direction_move_at_top:
                if(y>0)
                    y-=1;
                else
                {
                    const MapType &mapTop=mapList.at(map.border.top.mapIndex);
                    y=mapTop.height-1;
                    x+=map.border.top.x_offset;
                    mapIndex=map.border.top.mapIndex;
                }
                return true;
            break;
            case Direction_move_at_right:
                if(x<(map.width-1))
                    x+=1;
                else
                {
                    x=0;
                    y+=map.border.right.y_offset;
                    mapIndex=map.border.right.mapIndex;
                }
                return true;
            break;
            case Direction_move_at_bottom:
                if(y<(map.height-1))
                    y+=1;
                else
                {
                    y=0;
                    x+=map.border.bottom.x_offset;
                    mapIndex=map.border.bottom.mapIndex;
                }
                return true;
            break;
            case Direction_move_at_left:
                if(x>0)
                    x-=1;
                else
                {
                    const MapType &mapLeft=mapList.at(map.border.left.mapIndex);
                    x=mapLeft.width-1;
                    y+=map.border.left.y_offset;
                    mapIndex=map.border.left.mapIndex;
                }
                return true;
            break;
            default:
                return false;
        }
    }

    static uint8_t getMapZoneCode(const CommonMap &map, const uint8_t &x, const uint8_t &y);
    static bool isWalkable(const CommonMap &map, const uint8_t &x, const uint8_t &y);
    static int8_t indexOfTeleporter(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y);
    static bool needBeTeleported(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y);
    static ParsedLayerLedges getLedge(const CommonMap &map, const uint8_t &x, const uint8_t &y);

    static bool isWalkableWithDirection(const CommonMap &map, const uint8_t &x, const uint8_t &y,const CatchChallenger::Direction &direction);
    static bool isDirt(const CommonMap &map, const uint8_t &x, const uint8_t &y);
    static MonstersCollisionValue getZoneCollision(const CommonMap &map, const uint8_t &x, const uint8_t &y);
    Direction getDirection() const;
protected:
    virtual void send_player_move_internal(const uint8_t &moved_unit,const Direction &the_new_direction) = 0;
    Direction last_direction;
    uint8_t last_step;
    bool last_direction_is_set;
};

}

#endif // MOVEONTHEMAP_H
