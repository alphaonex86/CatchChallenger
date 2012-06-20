#ifndef MAP_SERVER_H
#define MAP_SERVER_H

#include "../../general/base/Map.h"

class ClientMapManagement;

class Map_server : public Map
{
public:
	Map_server();

	QList<ClientMapManagement *> clients;//manipulated by thread of ClientMapManagement()

	struct MapVisibility_simple
	{
		bool show;
	};
	struct MapVisibility
	{
		MapVisibility_simple simple;
	};
	MapVisibility mapVisibility;
};

#endif // MAP_SERVER_H
