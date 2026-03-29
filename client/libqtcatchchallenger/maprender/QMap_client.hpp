#ifndef CATCHCHALLENGER_QMAP_CLIENT_H
#define CATCHCHALLENGER_QMAP_CLIENT_H

#include "../../general/base/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/GeneralType.hpp"
#include "../libqtcatchchallenger/DisplayStructures.hpp"

#include <QString>
#include <QList>
#include <QHash>
#include <QTimer>

namespace Tiled {
class MapObject;
}

namespace CatchChallenger {
class ClientPlantWithTimer : public QTimer
{
public:
    Tiled::MapObject * mapObject;
    COORD_TYPE x,y;
    uint8_t plant_id;
    uint64_t mature_at;
};
}

#endif // QMAP_SERVER_H
