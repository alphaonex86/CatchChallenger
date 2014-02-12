#include <QObject>

#include "GeneralStructures.h"

#ifndef CATCHCHALLENGER_MOVEONTHEMAP_H
#define CATCHCHALLENGER_MOVEONTHEMAP_H

//to group the step by step move into line move
/* template <typename TMap = Map>
class MoveOnTheMap {
   TMap * map_;*/

namespace CatchChallenger {

class CommonMap;

class MoveOnTheMap
{
public:
    MoveOnTheMap();
    virtual void newDirection(const Direction &the_direction);
    virtual void setLastDirection(const Direction &the_direction);
    //debug function
    static QString directionToString(const Direction &direction);
    static Orientation directionToOrientation(const Direction &direction);
    static Direction directionToDirectionLook(const Direction &direction);

    static bool canGoTo(const Direction &direction, const CommonMap &map, const quint8 &x, const quint8 &y, const bool &checkCollision, const bool &allowTeleport=true);
    static ParsedLayerLedges getLedge(const CommonMap &map, const quint8 &x, const quint8 &y);
    static bool move(Direction direction, CommonMap ** map, quint8 *x, quint8 *y, const bool &checkCollision=false, const bool &allowTeleport=true);
    static bool moveWithoutTeleport(Direction direction, CommonMap ** map, quint8 *x, quint8 *y, const bool &checkCollision=false, const bool &allowTeleport=true);
    static bool teleport(CommonMap ** map, quint8 *x, quint8 *y);
    static bool needBeTeleported(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y);

    static bool isWalkable(const CommonMap &map, const quint8 &x, const quint8 &y);
    static bool isDirt(const CommonMap &map, const quint8 &x, const quint8 &y);
    static MonstersCollisionValue getZoneCollision(const CommonMap &map, const quint8 &x, const quint8 &y);
protected:
    virtual void send_player_move(const quint8 &moved_unit,const Direction &the_new_direction) = 0;
    Direction last_direction;
    quint8 last_step;
};

}

#endif // MOVEONTHEMAP_H
