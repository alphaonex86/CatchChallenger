#ifndef CATCHCHALLENGER_QMAP_CLIENT_H
#define CATCHCHALLENGER_QMAP_CLIENT_H

#include "../../general/base/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/GeneralType.hpp"
#include "../libqtcatchchallenger/DisplayStructures.hpp"

#include "MapVisualiserOrder.hpp"

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
class ClientPlantWithTimer : public QTimer
{
public:
    Tiled::MapObject * mapObject;
    COORD_TYPE x,y;
    uint8_t plant_id;
    uint64_t mature_at;
};

class QMap_client
{
public:
    QMap_client();
public:
    static std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,QMap_client *> all_map;
    static std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,QMap_client *> old_all_map;
    static std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,uint8_t> old_all_map_time_count;
public:
    //moved into all_map above CATCHCHALLENGER_TYPE_MAPID mapIndex;//need keep global list and index to match file path always, to client and server speak about the same file, see std::vector<CatchChallenger::Map_client> DatapackClientLoader::mapList

    //std::unordered_map<std::pair<uint8_t,uint8_t>,CatchChallenger::Bot,pairhash> bots;-> to detect colision then in logical map just mark as colision to have same data into server and client
    std::unordered_map<std::pair<uint8_t,uint8_t>,CatchChallenger::BotDisplay,pairhash> botsDisplay;

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
    std::string visualType;
    std::string name;
    std::string zone;
};

}

#endif // QMAP_SERVER_H
