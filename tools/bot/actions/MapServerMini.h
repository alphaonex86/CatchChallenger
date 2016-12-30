#ifndef MAPSERVERMINI_H
#define MAPSERVERMINI_H

#include <map>
#include <utility>
#include <vector>
#include <stdint.h>
#include <unordered_map>

#include <QIcon>
#include <QString>
#include <QList>
#include <QColor>

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
    static QList<QColor> colorsList;

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
        enum LinkDirection
        {
            ToTheTarget,
            BothDirection
        };
        enum LinkType
        {
            SourceTeleporter,
            SourceTopMap,
            SourceRightMap,
            SourceBottomMap,
            SourceLeftMap,
            SourceInternalTopBlock,
            SourceInternalRightBlock,
            SourceInternalBottomBlock,
            SourceInternalLeftBlock
        };
        struct LinkInformation
        {
            LinkDirection direction;
            std::vector<LinkType> types;
        };
        std::unordered_map<BlockObject *,LinkInformation> links;
        MapServerMini * map;
        uint8_t id;

        //things into each zone
        std::vector<ItemOnMap> pointOnMap_Item;
        std::vector<uint32_t> botsFight;
        std::vector<uint32_t> shops;
        bool heal;
        const CatchChallenger::MonstersCollisionValue *monstersCollisionValue;

        bool rescue;
        bool learn;
        bool market;
        bool zonecapture;

        QColor color;
        void *layer;
    };
    struct MapParsedForBot{
        struct Layer{
            std::string text;
            std::string name;
            /*enum DestinationDisplay
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
            std::vector<Content> contentList;*/
            BlockObject *blockObject;
        };
        uint8_t *map;//0x00 is not accessible, it's why don't have layer for it
        std::vector<Layer> layers;//layer 1=index 0, layer 2=index 1, ...
    };
    struct BlockObjectPathFinding{
        std::vector<const BlockObject *> bestPath;
        uint32_t weight;
    };

    std::vector<MapParsedForBot> step;
    uint8_t min_x,max_x,min_y,max_y;

    uint8_t *botLayerMask;

    bool addBlockLink(BlockObject &blockObjectFrom,BlockObject &blockObjectTo,const BlockObject::LinkType &linkSourceFrom);
public:
    void displayConsoleMap(const MapParsedForBot &currentStep) const;
    bool mapIsValid(const MapParsedForBot &currentStep) const;
    void targetBlockList(const BlockObject * const currentNearBlock,std::unordered_map<const BlockObject *,BlockObjectPathFinding> &resolvedBlock,const unsigned int &depth=2) const;
    std::unordered_set<const MapServerMini *> getValidMaps(const unsigned int &depth=2) const;
    std::unordered_set<const BlockObject *> getAccessibleBlock(const std::unordered_set<const MapServerMini *> &validMaps,const BlockObject * const currentNearBlock) const;
    void resolvBlockPath(const BlockObject * blockToExplore,
                         std::unordered_map<const BlockObject *, BlockObjectPathFinding> &resolvedBlock,
                         const std::unordered_set<const BlockObject *> &accessibleBlock,
                         const std::vector<const BlockObject *> &previousBlock=std::vector<const BlockObject *>()
                         ) const;
    static uint32_t resolvBlockWeight(const BlockObject * const blockToExplore);
};

#endif // MAPSERVERMINI_H
