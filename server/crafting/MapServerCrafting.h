#ifndef POKECRAFT_MAP_SERVER_CRAFTING_H
#define POKECRAFT_MAP_SERVER_CRAFTING_H

#include <QHash>

namespace Pokecraft {

class MapServerCrafting
{
public:
    struct PlantOnMap
    {
        quint8 x;
        quint8 y;
        quint8 plant;//plant id
        quint32 player_id;//player id
        quint64 mature_at;//timestamp when is mature
        quint64 player_owned_expire_at;//timestamp when is mature
    };
    QList<PlantOnMap> plants;//position, plant id
};
}

#endif
