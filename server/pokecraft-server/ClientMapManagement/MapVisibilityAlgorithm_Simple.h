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
	//overwrite
	//remove the move/remove
	void insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed);
	void removeAnotherClient(const quint32 &player_id);
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
