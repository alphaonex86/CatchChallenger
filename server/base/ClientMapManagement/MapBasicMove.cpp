#include "MapBasicMove.h"
#include "../Map_server.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/MoveOnTheMap.h"

using namespace Pokecraft;

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

MapBasicMove::MapBasicMove()
{
    map=NULL;
    player_informations=NULL;
}

MapBasicMove::~MapBasicMove()
{
}

void MapBasicMove::setVariable(Player_internal_informations *player_informations)
{
    this->player_informations=player_informations;
}

void MapBasicMove::askIfIsReadyToStop()
{
    if(map==NULL)
    {
        emit isReadyToStop();
        return;
    }
    extraStop();

    map=NULL;
    emit isReadyToStop();
}

void MapBasicMove::extraStop()
{
}

void MapBasicMove::put_on_the_map(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    //store the starting informations
    last_direction=static_cast<Direction>(orientation);

    //store the current information about player on the map
    this->x=x;
    this->y=y;
    this->map=map;

    #ifdef POKECRAFT_SERVER_EXTRA_CHECK
    if(this->x>(map->width-1))
    {
        emit message(QString("put_on_the_map(): Wrong x: %1").arg(x));
        this->x=map->width-1;
    }
    if(this->y>(map->height-1))
    {
        emit message(QString("put_on_the_map(): Wrong y: %1").arg(y));
        this->y=map->height-1;
    }
    #endif
}

bool MapBasicMove::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("for player (%1,%2): %3, previousMovedUnit: %4 (%5), next direction: %6").arg(x).arg(y).arg(player_informations->public_and_private_informations.public_informations.simplifiedId).arg(previousMovedUnit).arg(MoveOnTheMap::directionToString(last_direction)).arg(MoveOnTheMap::directionToString(direction)));
    #endif
    quint8 moveThePlayer_index_move=0;
    if(unlikely(last_direction==direction))
    {
        emit error(QString("Previous action is same direction: %1").arg(last_direction));
        return false;
    }
    switch(last_direction)
    {
        case Direction_move_at_top:
            if(unlikely(previousMovedUnit==0))
            {
                emit error(QString("Direction_move_at_top: Previous action is moving: %1").arg(last_direction));
                return false;
            }
            while(moveThePlayer_index_move<previousMovedUnit)
            {
                if(!singleMove(Direction_move_at_top))
                    return false;
                moveThePlayer_index_move++;
            }
        break;
        case Direction_look_at_top:
            if(unlikely(previousMovedUnit>0))
            {
                emit error(QString("Direction_look_at_top: Previous action is not moving: %1").arg(last_direction));
                return false;
            }
        break;
        case Direction_move_at_right:
            if(unlikely(previousMovedUnit==0))
            {
                emit error(QString("Direction_move_at_right: Previous action is moving: %1").arg(last_direction));
                return false;
            }
            while(moveThePlayer_index_move<previousMovedUnit)
            {
                if(!singleMove(Direction_move_at_right))
                    return false;
                moveThePlayer_index_move++;
            }
        break;
        case Direction_look_at_right:
            if(unlikely(previousMovedUnit>0))
            {
                emit error(QString("Direction_look_at_right: Previous action is not moving: %1").arg(last_direction));
                return false;
            }
        break;
        case Direction_move_at_bottom:
            if(unlikely(previousMovedUnit==0))
            {
                emit error(QString("Direction_move_at_bottom: Previous action is moving: %1").arg(last_direction));
                return false;
            }
            while(moveThePlayer_index_move<previousMovedUnit)
            {
                if(!singleMove(Direction_move_at_bottom))
                    return false;
                moveThePlayer_index_move++;
            }
        break;
        case Direction_look_at_bottom:
            if(unlikely(previousMovedUnit>0))
            {
                emit error(QString("Direction_look_at_bottom: Previous action is not moving: %1").arg(last_direction));
                return false;
            }
        break;
        case Direction_move_at_left:
            if(unlikely(previousMovedUnit==0))
            {
                emit error(QString("Direction_move_at_left: Previous action is moving: %1").arg(last_direction));
                return false;
            }
            while(moveThePlayer_index_move<previousMovedUnit)
            {
                if(!singleMove(Direction_move_at_left))
                    return false;
                moveThePlayer_index_move++;
            }
        break;
        case Direction_look_at_left:
            if(unlikely(previousMovedUnit>0))
            {
                emit error(QString("Direction_look_at_left: Previous action is not moving: %1").arg(last_direction));
                return false;
            }
        break;
        default:
            emit error(QString("moveThePlayer(): direction not managed"));
            return false;
        break;
    }
    last_direction=direction;
    return true;
}
