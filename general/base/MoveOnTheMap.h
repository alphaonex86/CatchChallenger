#ifndef MOVEONTHEMAP_H
#define MOVEONTHEMAP_H

#include <QObject>

#include "GeneralStructures.h"

//to group the step by step move into line move
class MoveOnTheMap
{
public:
	MoveOnTheMap();
	virtual void newDirection(const Direction &the_direction);
private:
	virtual void send_player_move(const quint8 &moved_unit,const Direction &the_new_direction) = 0;
protected:
	Direction last_direction;
	quint8 last_step;
};

#endif // MOVEONTHEMAP_H
