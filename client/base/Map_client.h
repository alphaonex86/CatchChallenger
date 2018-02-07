#ifndef CATCHCHALLENGER_MAP_CLIENT_H
#define CATCHCHALLENGER_MAP_CLIENT_H

#include "../../general/base/CommonMap.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralStructuresXml.h"
#include "../../general/base/GeneralVariable.h"
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
    uint8_t x,y;
    uint8_t plant_id;
    uint64_t mature_at;
};

class Map_client : public CommonMap
{
public:
    struct ItemOnMapForClient
    {
        Tiled::MapObject* tileObject;
        uint16_t item;
        bool infinite;
        uint8_t indexOfItemOnMap;//to see if the player have get it
    };

    QList<Map_semi_teleport> teleport_semi;
    QStringList teleport_condition_texts;
    QList<Map_to_send::Map_Point> rescue_points;
    QList<Map_to_send::Map_Point> bot_spawn_points;

    Map_semi_border border_semi;

    Map_client();

    QList<ClientPlantWithTimer *> plantList;
    QHash<QPair<uint8_t,uint8_t>,Bot> bots;
    QHash<QPair<uint8_t,uint8_t>,BotDisplay> botsDisplay;

    QMultiHash<QPair<uint8_t,uint8_t>,QPair<uint8_t,uint8_t> > botsFightTriggerExtra;//trigger line in front of bot fight, tigger x,y, bot x,y
    QHash<QPair<uint8_t,uint8_t>,ItemOnMapForClient> itemsOnMap;

    const CATCHCHALLENGER_XMLELEMENT * xmlRoot;
};
}

#endif // MAP_SERVER_H
