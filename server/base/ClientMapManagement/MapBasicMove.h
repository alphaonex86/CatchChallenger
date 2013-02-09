#ifndef CATCHCHALLENGER_MapBasicMove_H
#define CATCHCHALLENGER_MapBasicMove_H

#include <QObject>
#include <QList>
#include <QString>

#include "../../../general/base/DebugClass.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../ServerStructures.h"
#include "../../VariableServer.h"

/** \warning No static variable due to thread access to this class!!! */

namespace CatchChallenger {
class MapBasicMove : public QObject
{
    Q_OBJECT
public:
    explicit MapBasicMove();
    virtual ~MapBasicMove();
    virtual void setVariable(Player_internal_informations *player_informations);
    //info linked

    Direction getLastDirection();
    Map* getMap();
    COORD_TYPE getX();
    COORD_TYPE getY();

    //internal var
    Player_internal_informations *player_informations;
protected:
    //pass to the Map management visibility algorithm
    virtual bool singleMove(const Direction &direction) = 0;
    COORD_TYPE			x,y;//can't be negative
    Map*                map;

    //related to stop
    //volatile bool stopCurrentMethod;
    //volatile bool stopIt;
    virtual void extraStop();
signals:
    //normal signals
    void error(const QString &error);
    void message(const QString &message);
    void isReadyToStop();
    void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray());
    void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray());
public slots:
    //map slots, transmited by the current ClientNetworkRead
    virtual void put_on_the_map(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void teleportValidatedTo(Map *map, const quint8 &x, const quint8 &y, const Orientation &orientation);
    //normal slots
    virtual void askIfIsReadyToStop();
private:
    //temp variable for put on map
    /*quint8 moveThePlayer_index_move;*/ /// \warning not static because have multiple thread access
    //map vector informations
    Direction			last_direction;
};
}

#endif // MapBasicMove_H
