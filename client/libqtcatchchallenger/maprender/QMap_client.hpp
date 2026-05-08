#ifndef CATCHCHALLENGER_QMAP_CLIENT_H
#define CATCHCHALLENGER_QMAP_CLIENT_H

#include "../../general/base/CommonMap/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/GeneralType.hpp"
#include "../libqtcatchchallenger/DisplayStructures.hpp"

#include "MapVisualiserOrder.hpp"
#include "ClientPlantWithTimer.hpp"

#include <QString>
#include <QList>
#include <QHash>
#include <QTimer>

#include <map.h>
#include <maprenderer.h>
#include <mapobject.h>
#include <objectgroup.h>
#include <tile.h>

namespace Tiled {
class MapObject;
}

namespace CatchChallenger {
class QMap_client
{
public:
    QMap_client();
public:
    static std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,QMap_client *> all_map;
    static std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,QMap_client *> old_all_map;
    static std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,uint8_t> old_all_map_time_count;
public:
    //std::unordered_map<std::pair<uint8_t,uint8_t>,CatchChallenger::Bot,pairhash> bots;-> to detect colision then in logical map just mark as colision to have same data into server and client
    std::unordered_map<std::pair<uint8_t,uint8_t>,CatchChallenger::BotDisplay,pairhash> botsDisplay;
    // Pending-bot table populated by MapVisualiserOrder when it strips
    // type="bot" MapObjects from the visual layer.  Values carry the
    // skin / lookAt / id properties read off the MapObject before the
    // wrapper is destroyed.  MapVisualiser::asyncDetectBorder iterates
    // this AFTER the map is added to the scene, calling
    // loadBotOnTheMap() per entry to attach a real sprite (the
    // earlier "directly populate botsDisplay" approach didn't work
    // because loadBotOnTheMap early-returns when botsDisplay already
    // contains the (x,y) key — so we'd block our own re-attach).
    struct PendingBot
    {
        std::string skin;
        std::string lookAt;
        CATCHCHALLENGER_TYPE_BOTID botId;
    };
    std::unordered_map<std::pair<uint8_t,uint8_t>,PendingBot,pairhash> pendingBots;
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::pair<uint8_t,uint8_t>,pairhash> botsFightTriggerExtra;//trigger x,y -> bot display x,y
    std::vector<std::string> teleport_condition_texts;

    std::shared_ptr<Tiled::Map> tiledMap;
    Tiled::MapRenderer * tiledRender;
    Tiled::ObjectGroup * objectGroup;
    std::unordered_map<uint16_t/*ms*/,std::unordered_map<int/*minId*/,MapVisualiserOrder::Map_animation> > animatedObject;
    int objectGroupIndex;
    int relative_x,relative_y;//needed for the async load
    int relative_x_pixel,relative_y_pixel;
    bool displayed;
    std::unordered_map<std::pair<COORD_TYPE,COORD_TYPE>,MapDoor*,pairhash> doors;
    std::unordered_map<std::pair<COORD_TYPE,COORD_TYPE>,TriggerAnimation*,pairhash> triggerAnimations;
    std::unordered_map<std::pair<COORD_TYPE,COORD_TYPE>,ClientPlantWithTimer *,pairhash> Qplants;//get x,y via Player_private_and_public_informations_Map, this is paralelle structure to store visual part like QTimer and tile
    std::string visualType;
    std::string name;
    std::string zone;
    std::string backgroundsound;
};

}

#endif // QMAP_SERVER_H
