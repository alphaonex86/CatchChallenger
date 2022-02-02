#ifndef MapVisualiserOrder_H
#define MapVisualiserOrder_H

#include <QSet>
#include <QString>
#include <QHash>
#include <QRegularExpression>

#include "../../libraries/tiled/tiled_isometricrenderer.hpp"
#include "../../libraries/tiled/tiled_map.hpp"
#include "../../libraries/tiled/tiled_mapobject.hpp"
#include "../../libraries/tiled/tiled_mapreader.hpp"
#include "../../libraries/tiled/tiled_objectgroup.hpp"
#include "../../libraries/tiled/tiled_orthogonalrenderer.hpp"
#include "../../libraries/tiled/tiled_tilelayer.hpp"
#include "../../libraries/tiled/tiled_tileset.hpp"
#include "../../libraries/tiled/tiled_tile.hpp"

#include "../../../general/base/GeneralStructures.hpp"
#include "../../../general/base/CommonMap.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../Map_client.hpp"
#include "../DisplayStructures.hpp"
#include "../../../general/base/Map_loader.hpp"
#include "MapDoor.hpp"
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

    static std::string text_blockedtext;
    static std::string text_en;
    static std::string text_lang;
    static std::string text_Dyna_management;
    static std::string text_Moving;
    static std::string text_door;
    static std::string text_Object;
    static std::string text_bot;
    static std::string text_bots;
    static std::string text_WalkBehind;
    static std::string text_Collisions;
    static std::string text_Grass;
    static std::string text_animation;
    static std::string text_dotcomma;
    static std::string text_ms;
    static std::string text_frames;
    static std::string text_map;
    static std::string text_objectgroup;
    static std::string text_name;
    static std::string text_object;
    static std::string text_type;
    static std::string text_x;
    static std::string text_y;
    static std::string text_botfight;
    static std::string text_property;
    static std::string text_value;
    static std::string text_file;
    static std::string text_id;
    static std::string text_slash;
    static std::string text_dotxml;
    static std::string text_dottmx;
    static std::string text_properties;
    static std::string text_shop;
    static std::string text_learn;
    static std::string text_heal;
    static std::string text_fight;
    static std::string text_zonecapture;
    static std::string text_market;
    static std::string text_zone;
    static std::string text_fightid;
    static std::string text_randomoffset;
    static std::string text_visible;
    static std::string text_true;
    static std::string text_false;
    static std::string text_trigger;
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
    Tiled::Map * tiledMap;
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
