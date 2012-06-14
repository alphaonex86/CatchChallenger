#include "ClientMapManagement.h"

#ifndef MAPVISIBILITYALGORITHM_NONE_H
#define MAPVISIBILITYALGORITHM_NONE_H

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
private:
	MapVisibilityAlgorithm_None *current_client;
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
