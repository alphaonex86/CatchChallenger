#ifndef CATCHCHALLENGER_ClientPlantWithTimer_H
#define CATCHCHALLENGER_ClientPlantWithTimer_H

#include "../../general/base/GeneralStructures.hpp"
#include <QTimer>

namespace Tiled {
class MapObject;
}

namespace CatchChallenger {
class ClientPlantWithTimer : public QTimer
{
public:
    ClientPlantWithTimer();
public:
    Tiled::MapObject * mapObject;
};
}

#endif // QMAP_SERVER_H
