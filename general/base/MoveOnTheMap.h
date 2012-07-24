#include <QObject>

#include "GeneralStructures.h"

#ifndef MOVEONTHEMAP_H
#define MOVEONTHEMAP_H

//to group the step by step move into line move
/* template <typename TMap = Map>
class MoveOnTheMap {
   TMap * map_;*/

class Map;

class MoveOnTheMap
{
public:
	MoveOnTheMap();
	virtual void newDirection(const Direction &the_direction);
protected:
	virtual void send_player_move(const quint8 &moved_unit,const Direction &the_new_direction) = 0;
	Direction last_direction;
	quint8 last_step;
protected:
	static bool canGoTo(Direction direction,Map *map,quint16 x,quint16 y);
	static bool move(Direction direction,Map ** map,quint16 &x,quint16 &y);
	static void teleport(Map ** map,quint16 &x,quint16 &y);
};

#endif // MOVEONTHEMAP_H
