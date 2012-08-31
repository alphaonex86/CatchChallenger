#ifndef POKECRAFT_MAP_CLIENT_H
#define POKECRAFT_MAP_CLIENT_H

#include "../../general/base/Map.h"
#include "../../general/base/GeneralStructures.h"

#include <QString>
#include <QList>

namespace Pokecraft {
class Map_client : public Map
{
public:
    QList<Map_semi_teleport> teleport_semi;
    QList<Map_to_send::Rescue_Point> rescue_points;
    QList<Map_to_send::Bot_Spawn_Point> bot_spawn_points;
	
    Map_semi_border border_semi;
	
	Map_client();
};
}

#endif // MAP_SERVER_H
