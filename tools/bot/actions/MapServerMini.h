#ifndef MAPSERVERMINI_H
#define MAPSERVERMINI_H

#include <map>
#include <utility>
#include <vector>
#include <stdint.h>

#include "../../general/base/CommonMap.h"

class MapServerMini : public CatchChallenger::CommonMap
{
public:
    MapServerMini();
    bool preload_step1();

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
    };
    MapParsedForBot step1;
};

#endif // MAPSERVERMINI_H
