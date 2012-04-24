#include "ClientMapManagement.h"

#ifndef MAPVISIBILITYALGORITHM_SIMPLE_H
#define MAPVISIBILITYALGORITHM_SIMPLE_H

class MapVisibilityAlgorithm_Simple : public ClientMapManagement
{
public:
	explicit MapVisibilityAlgorithm_Simple();
	~MapVisibilityAlgorithm_Simple();
	void reinsertAllClient(const int &loop_size);
protected:
	//add clients linked
	void insertClient();
	void moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged);
	void removeClient();
	void mapVisiblity_unloadFromTheMap();
private:
	int index,loop_size;
	ClientMapManagement *current_client;
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
