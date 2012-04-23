#include "ClientMapManagement.h"

#ifndef MAPVISIBILITYALGORITHM_SIMPLE_H
#define MAPVISIBILITYALGORITHM_SIMPLE_H

class MapVisibilityAlgorithm_Simple : public ClientMapManagement
{
public:
	explicit MapVisibilityAlgorithm_Simple();
	~MapVisibilityAlgorithm_Simple();
	void reinsertAllClient();
protected:
	//add clients linked
	void insertClient(const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	void moveClient(const quint8 &movedUnit,const Direction &direction);
	void removeClient();
	void changeMap(Map_final *old_map,Map_final *new_map);
private:
	int index,loop_size;
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
