#ifndef MAPVISIBILITYALGORITHM_NONE_H
#define MAPVISIBILITYALGORITHM_NONE_H

#include "ClientMapManagement.h"

class MapVisibilityAlgorithm_None : public ClientMapManagement
{
public:
	explicit MapVisibilityAlgorithm_None();
	virtual ~MapVisibilityAlgorithm_None();
protected:
	//add clients linked
	void insertClient();
	void moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged);
	void removeClient();
	void mapVisiblity_unloadFromTheMap();
public slots:
	//map slots, transmited by the current ClientNetworkRead
	virtual void put_on_the_map(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,Map_server *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	virtual void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
public slots:
	virtual void purgeBuffer();
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
