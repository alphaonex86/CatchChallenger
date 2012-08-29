#ifndef POKECRAFT_MOVEONTHEMAP_CLIENT_H
#define POKECRAFT_MOVEONTHEMAP_CLIENT_H

#include <QObject>

#include "../../general/base/GeneralStructures.h"

namespace Pokecraft {
class Map;

class MoveOnTheMap_Client
{
public:
	MoveOnTheMap_Client();
protected:
	Map *map;
	quint16 x,y;
protected:
	bool canGoTo(Direction direction);
	void move(Direction direction);
};
}

#endif // MOVEONTHEMAP_CLIENT_H
