#ifndef MOVEONTHEMAP_SERVER_H
#define MOVEONTHEMAP_SERVER_H

#include <QObject>

#include "../ServerStructures.h"

class MoveOnTheMap_Server
{
public:
	MoveOnTheMap_Server();
protected:
	Map_server *map;
	quint16 x,y;
protected:
	bool canGoTo(Direction direction);
};

#endif // MOVEONTHEMAP_SERVER_H
