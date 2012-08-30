#ifndef POKECRAFT_MAP_CLIENT_H
#define POKECRAFT_MAP_CLIENT_H

#include "../../general/base/Map.h"
#include "../../general/base/GeneralStructures.h"

namespace Pokecraft {
class Map_client : public Map
{
public:
	struct Temp_teleport
	{
		COORD_TYPE source_x,source_y;
		COORD_TYPE destination_x,destination_y;
		QString map;
	};
    QList<Temp_teleport> teleport_semi;
	
    Map_semi_border border_semi;
	
	Map_client();
};
}

#endif // MAP_SERVER_H
