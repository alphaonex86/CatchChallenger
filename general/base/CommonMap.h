#ifndef CATCHCHALLENGER_MAP_H
#define CATCHCHALLENGER_MAP_H

#include <string>
#include <unordered_map>
#include <vector>
#include <vector>
#include <utility>
#include <QByteArray>

#include "GeneralStructures.h"

namespace CatchChallenger {
class CommonMap
{
public:
    //the index is position (x+y*width)
    ParsedLayer parsed_layer;

    struct Map_Border
    {
        struct Map_BorderContent_TopBottom
        {
            CommonMap *map;
            int32_t x_offset;
        };
        struct Map_BorderContent_LeftRight
        {
            CommonMap *map;
            int32_t y_offset;
        };
        Map_BorderContent_TopBottom top;
        Map_BorderContent_TopBottom bottom;
        Map_BorderContent_LeftRight left;
        Map_BorderContent_LeftRight right;
    };
    Map_Border border;

    std::vector<CommonMap *> near_map,border_map;//not only the border
    struct Teleporter
    {
        uint32_t x,y;
        CommonMap *map;
        MapCondition condition;
    };
    std::unordered_map<uint32_t,Teleporter> teleporter;//the int (x+y*width) is position

    std::string map_file;
    uint16_t width;
    uint16_t height;
    uint32_t group;
    uint32_t id;

    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint32_t>, pairhash> shops;
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> learn;
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> market;
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::string,pairhash> zonecapture;
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint32_t>,pairhash> botsFight;

    /*std::vector<MapMonster> grassMonster;
    std::vector<MapMonster> waterMonster;
    std::vector<MapMonster> caveMonster;*/

    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint32_t>,pairhash> botsFightTrigger;//trigger line in front of bot fight

    static void removeParsedLayer(const ParsedLayer &parsed_layer);
};
}

#endif // MAP_H
