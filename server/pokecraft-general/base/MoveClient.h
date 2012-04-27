#ifndef MOVECLIENT_H
#define MOVECLIENT_H

#include <QObject>

#include "GeneralStructures.h"

class MoveClient
{
public:
	MoveClient();
	virtual void newDirection(const Direction &the_direction);
private:
	virtual void send_player_move(const quint8 &moved_unit,const Direction &the_new_direction) = 0;
protected:
	Direction last_direction;
	quint8 last_step;
};

#endif // MOVECLIENT_H
