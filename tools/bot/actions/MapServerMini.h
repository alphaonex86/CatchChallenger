#ifndef MAPSERVERMINI_H
#define MAPSERVERMINI_H

#include <map>
#include <utility>
#include <vector>
#include <stdint.h>
#include <unordered_map>

#include <QString>
#include <QList>
#include <QtGui/QColor>

#include "../../general/base/CommonMap.h"
#include "../../client/base/Api_protocol.h"

class MapServerMini : public CatchChallenger::CommonMap
{
public:
    MapServerMini();
    bool preload_other_pre();
    bool preload_step1();
    bool preload_step2();
    void preload_step2_addNearTileToScanList(std::vector<std::pair<uint8_t,uint8_t> > &scanList, const uint8_t &x, const uint8_t &y);
    bool preload_step2b();
    bool preload_step2c();
    bool preload_post_subdatapack();
    static QList<QColor> colorsList;

    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> botOnMap;
    std::map<std::pair<uint8_t,uint8_t>,CatchChallenger::Orientation/*,pairhash*/> rescue;
    //at last to improve the other variable cache
    struct ItemOnMap
    {
        uint16_t item;
        bool infinite;
        bool visible;
        uint16_t indexOfItemOnMap;//to see if the player have get it
    };
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//first is x,y, second is db code, item
    struct PlantOnMap
    {
    };
    std::map<std::pair<uint8_t,uint8_t>,PlantOnMap> plants;//position, plant id

    struct BlockObject{
        /*enum LinkDirection
        {
            ToTheTarget,
            BothDirection
        }; control by access to the other zone: link can have condition (teleporter with condition)*/
        enum LinkType
        {
            SourceNone,
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
        struct LinkPoint
        {
            LinkType type;
            bool inLedge;
            //point to go
            uint8_t x,y;
        };
        struct LinkCondition
        {
            std::vector<LinkPoint> points;
            CatchChallenger::MapCondition condition;
        };
        struct LinkInformation
        {
            std::vector<LinkCondition> linkConditions;
        };
        struct DestinationForPath
        {
            CatchChallenger::Orientation destination_orientation;
            uint8_t destination_x;
            uint8_t destination_y;
        };
        struct PathFindingCacheEntry
        {
            //input
            CatchChallenger::Orientation source_orientation;
            uint8_t source_x,source_y;
            std::vector<DestinationForPath> destinations;

            //output
            bool ok;
            unsigned int destinationIndexSelected;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;
        };

        std::vector<PathFindingCacheEntry> pathFindingCache;
        std::map<const BlockObject */*to where*/,LinkInformation/*how, if single way or both way*/> links;
        MapServerMini * map;
        uint8_t id;//the displayed value is id+1

        //things into each zone
        std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;
        std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> botsFight;
        std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> botsFightTrigger;//trigger line in front of bot fight
        std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> shops;
        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
        const CatchChallenger::MonstersCollisionValue *monstersCollisionValue;

        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> rescue;
        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> learn;
        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> market;
        std::unordered_map<std::pair<uint8_t,uint8_t>,std::string,pairhash> zonecapture;
        std::vector<std::pair<uint8_t,uint8_t> > block;//exclude special like teleporter

        QColor color;
        void *layer;
    };
    struct MapParsedForBot{
        struct Layer{
            std::string text;
            std::string name;
            BlockObject *blockObject;
        };
        uint16_t *map;//0x00 is not accessible, it's why don't have layer for it
        std::vector<Layer> layers;//layer 1=index 0, layer 2=index 1, ...
    };
    struct BlockObjectPathFinding{
        std::vector<const BlockObject *> bestPath;
        uint32_t weight;
    };

    std::vector<MapParsedForBot> step;
    uint8_t min_x,max_x,min_y,max_y;

    //uint8_t *botLayerMask;->directly mark as not walkable into the map loader

    bool addBlockLink(BlockObject &blockObjectFrom, BlockObject &blockObjectTo, const BlockObject::LinkType &linkSourceFrom, /*point to go:*/const uint8_t &x, const uint8_t &y, const CatchChallenger::MapCondition &condition);
    BlockObject::LinkCondition &searchConditionOrCreate(BlockObject::LinkInformation &links, const CatchChallenger::MapCondition &condition);
public:
    void displayConsoleMap(const MapParsedForBot &currentStep) const;
    bool mapIsValid(const MapParsedForBot &currentStep) const;
    void targetBlockList(const BlockObject * const currentNearBlock, std::unordered_map<const BlockObject *,BlockObjectPathFinding> &resolvedBlock, const unsigned int &depth=2,const CatchChallenger::Api_protocol *api=NULL) const;
    std::unordered_set<const MapServerMini *> getValidMaps(const unsigned int &depth=2) const;
    std::unordered_set<const BlockObject *> getAccessibleBlock(const std::unordered_set<const MapServerMini *> &validMaps, const BlockObject * const currentNearBlock,const CatchChallenger::Api_protocol *api=NULL) const;
    void resolvBlockPath(const BlockObject * blockToExplore,
                         std::unordered_map<const BlockObject *, BlockObjectPathFinding> &resolvedBlock,
                         const std::unordered_set<const BlockObject *> &accessibleBlock,
                         const std::vector<const BlockObject *> &previousBlock=std::vector<const BlockObject *>()
                         ) const;
    static uint32_t resolvBlockWeight(const BlockObject * const blockToExplore);
    static bool haveDirectReturnWithoutCondition(const BlockObject &source, const BlockObject &destination);
};

#endif // MAPSERVERMINI_H
