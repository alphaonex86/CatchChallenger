#ifndef MAPSERVERMINI_H
#define MAPSERVERMINI_H

#include <map>
#include <utility>
#include <vector>
#include <stdint.h>
#include <unordered_map>

#include <QIcon>
#include <QString>

#include "../../general/base/CommonMap.h"

class MapServerMini : public CatchChallenger::CommonMap
{
public:
    MapServerMini();
    bool preload_step1();
    bool preload_step2();
    void preload_step2_addNearTileToScanList(std::vector<std::pair<uint8_t,uint8_t> > &scanList, const uint8_t &x, const uint8_t &y);
    bool preload_step2b();
    bool preload_step2c();
    bool preload_step2z();

    std::map<std::pair<uint8_t,uint8_t>,CatchChallenger::Orientation/*,pairhash*/> rescue;
    //at last to improve the other variable cache
    struct ItemOnMap
    {
        uint16_t item;
        bool infinite;
    };
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//first is x,y, second is db code, item
    struct PlantOnMap
    {
    };
    std::map<std::pair<uint8_t,uint8_t>,PlantOnMap> plants;//position, plant id

    struct BlockObject{
        enum LinkType
        {
            ToTheTarget,
            BothDirection
        };
        std::unordered_map<BlockObject *,LinkType> links;
        MapServerMini * map;
        uint8_t id;

        //things into each zone
        bool rescue;
        std::vector<ItemOnMap> pointOnMap_Item;
        std::vector<uint32_t> shops;
        bool learn;
        bool heal;
        bool market;
        bool zonecapture;
        std::vector<uint32_t> botsFight;
        const CatchChallenger::MonstersCollisionValue *monstersCollisionValue;

        MapServerMini *bordertop;
        MapServerMini *borderright;
        MapServerMini *borderbottom;
        MapServerMini *borderleft;
        std::vector<Teleporter> teleporter_list;
    };
    struct MapParsedForBot{
        struct Layer{
            std::string text;
            std::string name;
            enum DestinationDisplay
            {
                All,
                OnlyGui
            };
            struct Content{
                QIcon icon;
                QString text;
                uint32_t mapId;
                DestinationDisplay destinationDisplay;
            };
            std::vector<Content> contentList;
            bool haveMonsterSet;
            BlockObject *blockObject;
        };
        uint8_t *map;//0x00 is not accessible, it's why don't have layer for it
        std::vector<Layer> layers;//layer 1=index 0, layer 2=index 1, ...
        std::string graphvizText;
    };
    std::vector<MapParsedForBot> step;
    uint8_t min_x,max_x,min_y,max_y;

    uint8_t *botLayerMask;

    bool addBlockLink(BlockObject &blockObjectFrom,BlockObject &blockObjectTo);
public:
    void displayConsoleMap(const MapParsedForBot &currentStep);
    bool mapIsValid(const MapParsedForBot &currentStep);
};

#endif // MAPSERVERMINI_H
