#ifndef CATCHCHALLENGER_MAP_CLIENT_H
#define CATCHCHALLENGER_MAP_CLIENT_H

#include "../../general/base/Map.h"
#include "../../general/base/GeneralStructures.h"
#include <tiled/mapobject.h>
#include "ClientStructures.h"
#include "DisplayStructures.h"

#include <QString>
#include <QList>
#include <QHash>
#include <QTimer>
#include <QDomElement>

namespace CatchChallenger {
class ClientPlant : public QTimer
{
public:
    Tiled::MapObject * mapObject;
    quint8 x,y;
    quint8 plant_id;
    quint64 mature_at;
};

class Map_client : public Map
{
public:
    QList<Map_semi_teleport> teleport_semi;
    QList<Map_to_send::Map_Point> rescue_points;
    QList<Map_to_send::Map_Point> bot_spawn_points;

    Map_semi_border border_semi;

    Map_client();

    QList<ClientPlant *> plantList;
    QHash<QPair<quint8,quint8>,Bot> bots;
    QHash<QPair<quint8,quint8>,BotDisplay> botsDisplay;

    QMultiHash<QPair<quint8,quint8>,QPair<quint8,quint8> > botsFightTriggerExtra;//trigger line in front of bot fight

    QDomElement xmlRoot;
};
}

#endif // MAP_SERVER_H
