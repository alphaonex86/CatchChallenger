#include <QObject>

#include "GeneralStructures.h"

#ifndef MOVEONTHEMAP_H
#define MOVEONTHEMAP_H

//to group the step by step move into line move
/* template <typename TMap = Map>
class MoveOnTheMap {
   TMap * map_;*/

namespace Pokecraft {

class Map;

class MoveOnTheMap
{
public:
	MoveOnTheMap();
	virtual void newDirection(const Direction &the_direction);
	//debug function
	static QString directionToString(const Direction &direction);

	static bool canGoTo(Direction direction,Map *map,COORD_TYPE x,COORD_TYPE y,bool checkCollision);
	static bool move(Direction direction,Map ** map,COORD_TYPE &x,COORD_TYPE &y);
	static bool teleport(Map ** map,COORD_TYPE &x,COORD_TYPE &y);
protected:
	virtual void send_player_move(const quint8 &moved_unit,const Direction &the_new_direction) = 0;
	Direction last_direction;
	quint8 last_step;
};

}

#endif // MOVEONTHEMAP_H
