#include <QObject>

#include "GeneralStructures.h"
#include "Map.h"

#ifndef MOVEONTHEMAP_H
#define MOVEONTHEMAP_H

//to group the step by step move into line move
class MoveOnTheMap
{
public:
	MoveOnTheMap();
	virtual void newDirection(const Direction &the_direction);
protected:
	virtual void send_player_move(const quint8 &moved_unit,const Direction &the_new_direction) = 0;
	Direction last_direction;
	quint8 last_step;
	Map *map;
	quint16 x,y;
protected:
	bool canGoTo(Direction direction);
	void move(Direction direction);
};

#endif // MOVEONTHEMAP_H
