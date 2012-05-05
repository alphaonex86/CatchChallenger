#include "ClientMapManagement.h"

#ifndef MAPVISIBILITYALGORITHM_SIMPLE_H
#define MAPVISIBILITYALGORITHM_SIMPLE_H

class MapVisibilityAlgorithm_Simple : public ClientMapManagement
{
public:
	explicit MapVisibilityAlgorithm_Simple();
	virtual ~MapVisibilityAlgorithm_Simple();
	void reinsertAllClient(const int &loop_size);
protected:
	//add clients linked
	void insertClient();
	void moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged);
	void removeClient();
	void mapVisiblity_unloadFromTheMap();
private:
	int index,loop_size;
	MapVisibilityAlgorithm_Simple *current_client;
	//overwrite
	//remove the move/remove
	#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
	void insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed);
	#endif
	#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
	void moveAnotherClient(const quint32 &player_id,const Map_final *map,const quint8 &movedUnit,const Direction &direction);
	#endif
	#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
	void removeAnotherClient(const quint32 &player_id);
	#endif

	QSet<quint32>						overMove;
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
