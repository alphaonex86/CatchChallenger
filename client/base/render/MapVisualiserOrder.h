#ifndef MapVisualiserOrder_H
#define MapVisualiserOrder_H

#include <QSet>
#include <QString>
#include <QHash>
#include <QRegularExpression>

#include "../../tiled/tiled_isometricrenderer.h"
#include "../../tiled/tiled_map.h"
#include "../../tiled/tiled_mapobject.h"
#include "../../tiled/tiled_mapreader.h"
#include "../../tiled/tiled_objectgroup.h"
#include "../../tiled/tiled_orthogonalrenderer.h"
#include "../../tiled/tiled_tilelayer.h"
#include "../../tiled/tiled_tileset.h"
#include "../../tiled/tiled_tile.h"

#include "../../../general/base/GeneralStructures.h"
#include "../../../general/base/CommonMap.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../client/base/Map_client.h"
#include "../../../client/base/DisplayStructures.h"
#include "../../../general/base/Map_loader.h"
#include "../interface/MapDoor.h"
#include "../interface/TriggerAnimation.h"

class MapVisualiserOrder
{
public:
    struct Map_animation_object
    {
        quint8 randomOffset;
        Tiled::MapObject * animatedObject;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        int minId;
        int maxId;
        #endif
    };
    struct Map_animation
    {
        quint8 count;
        quint8 frameCountTotal;
        QList<Map_animation_object> animatedObjectList;
    };
    struct Map_full
    {
        CatchChallenger::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
        QHash<quint16,Map_animation> animatedObject;
        int objectGroupIndex;
        int relative_x,relative_y;//needed for the async load
        int relative_x_pixel,relative_y_pixel;
        bool displayed;
        QHash<QPair<quint8,quint8>,MapDoor*> doors;
        QHash<QPair<quint8,quint8>,TriggerAnimation*> triggerAnimations;
        QString visualType;
        QString name;
        QString zone;
    };
    struct TriggerAnimationContent
    {
        Tiled::Tile* objectTile;
        Tiled::Tile* objectTileOver;
        quint8 framesCountEnter;
        quint16 msEnter;
        quint8 framesCountLeave;
        quint16 msLeave;
        quint8 framesCountAgain;
        quint16 msAgain;
        bool over;
    };
    QHash<Tiled::Tile *,TriggerAnimationContent> tileToTriggerAnimationContent;

    explicit MapVisualiserOrder();
    ~MapVisualiserOrder();
    static void layerChangeLevelAndTagsChange(Map_full *tempMapObject, bool hideTheDoors=false);

    static QString text_blockedtext;
    static QString text_en;
    static QString text_lang;
    static QString text_Dyna_management;
    static QString text_Moving;
    static QString text_door;
    static QString text_Object;
    static QString text_bot;
    static QString text_bots;
    static QString text_WalkBehind;
    static QString text_Collisions;
    static QString text_Grass;
    static QString text_animation;
    static QString text_dotcomma;
    static QString text_ms;
    static QString text_frames;
    static QString text_map;
    static QString text_objectgroup;
    static QString text_name;
    static QString text_object;
    static QString text_type;
    static QString text_x;
    static QString text_y;
    static QString text_botfight;
    static QString text_property;
    static QString text_value;
    static QString text_file;
    static QString text_id;
    static QString text_slash;
    static QString text_dotxml;
    static QString text_dottmx;
    static QString text_step;
    static QString text_properties;
    static QString text_shop;
    static QString text_learn;
    static QString text_heal;
    static QString text_fight;
    static QString text_zonecapture;
    static QString text_market;
    static QString text_zone;
    static QString text_fightid;
    static QString text_randomoffset;
    static QString text_visible;
    static QString text_true;
    static QString text_false;
    static QString text_trigger;
protected:
    static QRegularExpression regexMs;
    static QRegularExpression regexFrames;
    static QRegularExpression regexTrigger;
    static QRegularExpression regexTriggerAgain;
};

#endif // MapVisualiserOrder_H
