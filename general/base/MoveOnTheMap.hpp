#ifndef CATCHCHALLENGER_MOVEONTHEMAP_H
#define CATCHCHALLENGER_MOVEONTHEMAP_H

#include <string>

#include "GeneralStructures.hpp"
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
    static bool canGoTo(const std::vector<MapType> &mapList,const Direction &direction, const CommonMap &map, const uint8_t &x, const uint8_t &y, const bool &checkCollision, const bool &allowTeleport=true);
    template<class MapType>
    static bool teleport(const std::vector<MapType> &mapList,CATCHCHALLENGER_TYPE_MAPID &mapIndex, uint8_t &x, uint8_t &y);
    template<class MapType>
    static bool move(const std::vector<MapType> &mapList,const Direction &direction, CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y, const bool &checkCollision=false, const bool &allowTeleport=true);
    template<class MapType>
    static bool moveWithoutTeleport(const std::vector<MapType> &mapList,const Direction &direction, CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE &x, COORD_TYPE &y, const bool &checkCollision=false, const bool &allowTeleport=true);

    static inline bool isWalkable(const CommonMap &map, const uint8_t &x, const uint8_t &y);
    static int8_t indexOfTeleporter(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y);
    static bool needBeTeleported(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y);
    static ParsedLayerLedges getLedge(const CommonMap &map, const uint8_t &x, const uint8_t &y);

    static bool isWalkableWithDirection(const CommonMap &map, const uint8_t &x, const uint8_t &y,const CatchChallenger::Direction &direction);
    static inline bool isDirt(const CommonMap &map, const uint8_t &x, const uint8_t &y);
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
