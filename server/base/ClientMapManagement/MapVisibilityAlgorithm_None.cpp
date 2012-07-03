#include "MapVisibilityAlgorithm_None.h"
#include "../EventDispatcher.h"

MapVisibilityAlgorithm_None::MapVisibilityAlgorithm_None()
{
}

MapVisibilityAlgorithm_None::~MapVisibilityAlgorithm_None()
{
}

void MapVisibilityAlgorithm_None::insertClient()
{
	//auto insert to know where it have spawn
	//ClientMapManagement::insertAnotherClient(player_id,map,x,y,direction,speed);
}

void MapVisibilityAlgorithm_None::moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged)
{
	Q_UNUSED(movedUnit);
	Q_UNUSED(direction);
	Q_UNUSED(mapHaveChanged);
}

void MapVisibilityAlgorithm_None::removeClient()
{
}

void MapVisibilityAlgorithm_None::mapVisiblity_unloadFromTheMap()
{
	removeClient();
}

//map slots, transmited by the current ClientNetworkRead
void MapVisibilityAlgorithm_None::put_on_the_map(const quint32 &player_id,Map_server *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	Q_UNUSED(player_id);
	Q_UNUSED(map);
	Q_UNUSED(x);
	Q_UNUSED(y);
	Q_UNUSED(orientation);
	Q_UNUSED(speed);
}

void MapVisibilityAlgorithm_None::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	Q_UNUSED(previousMovedUnit);
	Q_UNUSED(direction);
}
