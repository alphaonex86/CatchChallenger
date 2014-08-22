#ifndef CATCHCHALLENGER_MAP_CLIENT_H
#define CATCHCHALLENGER_MAP_CLIENT_H

#include "../../general/base/CommonMap.h"
#include "../../general/base/GeneralStructures.h"
#include "../tiled/tiled_mapobject.h"
#include "ClientStructures.h"
#include "DisplayStructures.h"

#include <QString>
#include <QList>
#include <QHash>
#include <QTimer>
#include <QDomElement>

namespace CatchChallenger {
class ClientPlantWithTimer : public QTimer
{
public:
    Tiled::MapObject * mapObject;
    quint8 x,y;
    quint8 plant_id;
    quint64 mature_at;
};

class Map_client : public CommonMap
{
public:
    struct ItemOnMapForClient
    {
        Tiled::MapObject* tileObject;
        quint32 item;
        bool infinite;
        quint8 indexOfItemOnMap;
    };

    QList<Map_semi_teleport> teleport_semi;
    QStringList teleport_condition_texts;
    QList<Map_to_send::Map_Point> rescue_points;
    QList<Map_to_send::Map_Point> bot_spawn_points;

    Map_semi_border border_semi;

    Map_client();

    QList<ClientPlantWithTimer *> plantList;
    QHash<QPair<quint8,quint8>,Bot> bots;
    QHash<QPair<quint8,quint8>,BotDisplay> botsDisplay;

    QMultiHash<QPair<quint8,quint8>,QPair<quint8,quint8> > botsFightTriggerExtra;//trigger line in front of bot fight
    QHash<QPair<quint8,quint8>,ItemOnMapForClient> itemsOnMap;

    QDomElement xmlRoot;
};
}

#endif // MAP_SERVER_H
