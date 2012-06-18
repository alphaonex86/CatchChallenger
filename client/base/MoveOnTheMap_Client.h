#ifndef MOVEONTHEMAP_CLIENT_H
#define MOVEONTHEMAP_CLIENT_H

#include <QObject>

#include "GeneralStructures.h"

struct Map_client;

class MoveOnTheMap_Client
{
public:
	MoveOnTheMap_Client();
protected:
	MoveOnTheMap_Client *map;
	quint16 x,y;
protected:
	bool canGoTo(Direction direction);
	void move(Direction direction);
};

#endif // MOVEONTHEMAP_CLIENT_H
