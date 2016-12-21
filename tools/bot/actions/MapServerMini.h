#ifndef MAPSERVERMINI_H
#define MAPSERVERMINI_H

#include <map>
#include <utility>
#include <vector>
#include <stdint.h>
#include <unordered_map>

#include "../../general/base/CommonMap.h"

class MapServerMini : public CatchChallenger::CommonMap
{
public:
    MapServerMini();
    bool preload_step1();
    bool preload_step2();
    void preload_step2_addNearTileToScanList(std::vector<std::pair<uint8_t,uint8_t> > &scanList, const uint8_t &x, const uint8_t &y);

    std::map<std::pair<uint8_t,uint8_t>,CatchChallenger::Orientation/*,pairhash*/> rescue;
    int reverse_db_id;

    //at last to improve the other variable cache
    struct ItemOnMap
    {
        uint16_t pointOnMapDbCode;

        uint16_t item;
        bool infinite;
    };
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//first is x,y, second is db code, item
    struct PlantOnMap
    {
        uint16_t pointOnMapDbCode;
    };
    std::map<std::pair<uint8_t,uint8_t>,PlantOnMap> plants;//position, plant id


    struct MapParsedForBot{
        struct Layer{
            std::string text;
            std::string name;
        };
        uint8_t *map;//0x00 is not accessible, it's why don't have layer for it
        std::vector<Layer> layers;//layer 1=index 0, layer 2=index 1, ...
        std::string graphvizText;
    };
    std::vector<MapParsedForBot> step;
    uint8_t min_x,max_x,min_y,max_y;

    uint8_t *botLayerMask;
    struct BlockObject{
        std::string text;
        std::string name;
        enum LinkType
        {
            ToTheTarget,
            BothDirection
        };
        std::unordered_map<BlockObject *,LinkType> links;
    };
    std::vector<BlockObject> blockList;

    bool addBlockLink(BlockObject &blockObjectFrom,BlockObject &blockObjectTo);
};

#endif // MAPSERVERMINI_H
