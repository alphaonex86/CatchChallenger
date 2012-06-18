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
