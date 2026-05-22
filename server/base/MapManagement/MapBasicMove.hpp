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

    //see MapVisibilityAlgorithm::flat_map_list; on server
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    COORD_TYPE x,y;
    Direction last_direction;

    // Trivial accessors inlined in the header: profiling (callgrind) showed
    // ~5% Ir lost to out-of-line calls of these from MapVisibilityAlgorithm
    // ::min_network's tight loops. One-liners, safe to inline.
    inline Direction getLastDirection() const { return last_direction; }
    inline CATCHCHALLENGER_TYPE_MAPID getMapId() const { return mapIndex; }
    inline COORD_TYPE getX() const { return x; }
    inline COORD_TYPE getY() const { return y; }
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
