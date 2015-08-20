#ifndef CATCHCHALLENGER_MapBasicMove_H
#define CATCHCHALLENGER_MapBasicMove_H

#include <QObject>
#include <QList>
#include <QString>

#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../ServerStructures.h"
#include "../../VariableServer.h"

/** \warning No static variable due to thread access to this class!!! */

namespace CatchChallenger {
class MapBasicMove
{
public:
    explicit MapBasicMove();
    virtual ~MapBasicMove();
    //info linked

    CommonMap *map;
    COORD_TYPE x,y;
    Direction last_direction;

    Direction getLastDirection() const;
    CommonMap* getMap() const;
    COORD_TYPE getX() const;
    COORD_TYPE getY() const;
public:
    //map slots, transmited by the current ClientNetworkRead
    virtual void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void teleportValidatedTo(CommonMap *map, const quint8 &x, const quint8 &y, const Orientation &orientation);
protected:
    //normal management related
    virtual void errorOutput(const QString &errorString);
    virtual void normalOutput(const QString &message) const;
    virtual bool singleMove(const Direction &direction) = 0;
};
}

#endif // MapBasicMove_H
