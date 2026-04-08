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
#include "TriggerAnimation.hpp"
#include "../libqtcatchchallenger/DisplayStructures.hpp"
#include "../libcatchchallenger/ClientStructures.hpp"

namespace CatchChallenger { class QMap_client; }
using CatchChallenger::QMap_client;

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
    static void layerChangeLevelAndTagsChange(QMap_client *tempMapObject, bool hideTheDoors=false);
protected:
    static QRegularExpression regexMs;
    static QRegularExpression regexFrames;
    static QRegularExpression regexTrigger;
    static QRegularExpression regexTriggerAgain;
};

#endif // MapVisualiserOrder_H
