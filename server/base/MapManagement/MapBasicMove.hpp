#ifndef CATCHCHALLENGER_MapBasicMove_H
#define CATCHCHALLENGER_MapBasicMove_H

#include <string>

#include "../../../general/base/MoveOnTheMap.hpp"
#include "../ServerStructures.hpp"

/** \warning No static variable due to thread access to this class!!! */

namespace CatchChallenger {
class MapBasicMove
{
public:
    explicit MapBasicMove();
    virtual ~MapBasicMove();
    //info linked

    //see Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list; on server
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    COORD_TYPE x,y;
    Direction last_direction;

    Direction getLastDirection() const;
    CommonMap* getMap() const;
    COORD_TYPE getX() const;
    COORD_TYPE getY() const;
public:
    //map slots, transmited by the current ClientNetworkRead
    virtual void put_on_the_map(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    virtual bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction);
    virtual void teleportValidatedTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const uint8_t &x, const uint8_t &y, const Orientation &orientation);
protected:
    //normal management related
    virtual void errorOutput(const std::string &errorString);
    virtual void normalOutput(const std::string &message) const;
    virtual bool singleMove(const Direction &direction) = 0;
};
}

#endif // MapBasicMove_H
