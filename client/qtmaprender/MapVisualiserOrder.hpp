#ifndef MapVisualiserOrder_H
#define MapVisualiserOrder_H

#include <QSet>
#include <QString>
#include <QHash>
#include <QRegularExpression>

#include <map.h>
#include <maprenderer.h>
#include <mapobject.h>
#include <objectgroup.h>
#include <tile.h>

#include "MapDoor.hpp"
#include "Map_client.hpp"
#include "TriggerAnimation.hpp"

class Map_full;

class MapVisualiserOrder
{
public:
    struct Map_animation_object
    {
        Tiled::MapObject * animatedObject;
    };
    struct Map_animation
    {
        int minId;
        int maxId;
        std::vector<Map_animation_object> animatedObjectList;
    };
    struct TriggerAnimationContent
    {
        Tiled::Tile* objectTile;
        Tiled::Tile* objectTileOver;
        uint8_t framesCountEnter;
        uint16_t msEnter;
        uint8_t framesCountLeave;
        uint16_t msLeave;
        uint8_t framesCountAgain;
        uint16_t msAgain;
        bool over;
    };
    std::unordered_map<Tiled::Tile *,TriggerAnimationContent> tileToTriggerAnimationContent;

    explicit MapVisualiserOrder();
    ~MapVisualiserOrder();
    static void layerChangeLevelAndTagsChange(Map_full *tempMapObject, bool hideTheDoors=false);
protected:
    static QRegularExpression regexMs;
    static QRegularExpression regexFrames;
    static QRegularExpression regexTrigger;
    static QRegularExpression regexTriggerAgain;
};

class Map_full
{
public:
    Map_full();
public:
    CatchChallenger::Map_client logicalMap;
    std::shared_ptr<Tiled::Map> tiledMap;
    Tiled::MapRenderer * tiledRender;
    Tiled::ObjectGroup * objectGroup;
    std::unordered_map<uint16_t/*ms*/,std::unordered_map<int/*minId*/,MapVisualiserOrder::Map_animation> > animatedObject;
    int objectGroupIndex;
    int relative_x,relative_y;//needed for the async load
    int relative_x_pixel,relative_y_pixel;
    bool displayed;
    std::unordered_map<std::pair<uint8_t,uint8_t>,MapDoor*,pairhash> doors;
    std::unordered_map<std::pair<uint8_t,uint8_t>,TriggerAnimation*,pairhash> triggerAnimations;
    std::string visualType;
    std::string name;
    std::string zone;
};

#endif // MapVisualiserOrder_H
