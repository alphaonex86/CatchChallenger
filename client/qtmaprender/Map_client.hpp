#ifndef CATCHCHALLENGER_MAP_CLIENT_H
#define CATCHCHALLENGER_MAP_CLIENT_H

#include "../../general/base/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/GeneralStructuresXml.hpp"
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
        uint16_t indexOfItemOnMap;//to see if the player have get it, DatapackClientLoader::std::unordered_map<QString,std::unordered_map<QPair<uint8_t,uint8_t>,uint16_t> > itemOnMap;
    };
    std::vector<CommonMap *> near_map;//only the border (left, right, top, bottom) AND them self
    std::vector<CommonMap *> linked_map;//not only the border, with tp, door, ...

    std::vector<Map_semi_teleport> teleport_semi;
    std::vector<std::string> teleport_condition_texts;
    std::vector<Map_to_send::Map_Point> rescue_points;
    std::vector<Map_to_send::Map_Point> bot_spawn_points;

    Map_semi_border border_semi;
    uint32_t group;

    Map_client();

    std::vector<ClientPlantWithTimer *> plantList;
    std::unordered_map<std::pair<uint8_t,uint8_t>,Bot,pairhash> bots;
    std::unordered_map<std::pair<uint8_t,uint8_t>,BotDisplay,pairhash> botsDisplay;

    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<std::pair<uint8_t,uint8_t> >,pairhash> botsFightTriggerExtra;//trigger line in front of bot fight, tigger x,y, bot x,y
    std::unordered_map<std::pair<uint8_t,uint8_t>,ItemOnMapForClient,pairhash> itemsOnMap;

    const tinyxml2::XMLElement * xmlRoot;
};
}

#endif // MAP_SERVER_H
