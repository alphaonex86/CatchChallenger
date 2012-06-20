#include <QObject>

#include "../ServerStructures.h"

class Map_server;

#ifndef MOVEONTHEMAP_SERVER_H
#define MOVEONTHEMAP_SERVER_H

class MoveOnTheMap_Server
{
public:
	MoveOnTheMap_Server();
protected:
	Map_server *map;
	quint16 x,y;
protected:
	bool canGoTo(Direction direction);
	void move(Direction direction);
};

#endif // MOVEONTHEMAP_SERVER_H
