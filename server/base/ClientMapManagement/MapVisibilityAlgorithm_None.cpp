#include "MapVisibilityAlgorithm_None.h"

using namespace CatchChallenger;

MapVisibilityAlgorithm_None::MapVisibilityAlgorithm_None(ConnectedSocket *socket) :
    Client(socket)
{
}

void MapVisibilityAlgorithm_None::insertClient()
{
    //auto insert to know where it have spawn
    //ClientMapManagement::insertAnotherClient(player_id,map,x,y,direction,speed);
}

void MapVisibilityAlgorithm_None::moveClient(const quint8 &movedUnit,const Direction &direction)
{
    Q_UNUSED(movedUnit);
    Q_UNUSED(direction);
}

void MapVisibilityAlgorithm_None::removeClient()
{
}

void MapVisibilityAlgorithm_None::mapVisiblity_unloadFromTheMap()
{
    removeClient();
}

void MapVisibilityAlgorithm_None::extraStop()
{
}

//map slots, transmited by the current ClientNetworkRead
/*void MapVisibilityAlgorithm_None::put_on_the_map(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,Map_server_MapVisibility_simple *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
    Q_UNUSED(player_id);
    Q_UNUSED(map);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(orientation);
    Q_UNUSED(speed);
}*/

bool MapVisibilityAlgorithm_None::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    Q_UNUSED(previousMovedUnit);
    Q_UNUSED(direction);
    return true;
}

void MapVisibilityAlgorithm_None::purgeBuffer()
{
}

bool MapVisibilityAlgorithm_None::singleMove(const Direction &direction)
{
    Q_UNUSED(direction);
    return true;
}

quint16 MapVisibilityAlgorithm_None::getMaxVisiblePlayerAtSameTime()
{
    return 0;
}
