#ifndef CATCHCHALLENGER_MAP_H
#define CATCHCHALLENGER_MAP_H

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "GeneralStructures.hpp"

namespace CatchChallenger {
class CommonMap
{
public:
    struct Map_Border
    {
        struct Map_BorderContent_TopBottom
        {
            CommonMap *map;
            int16_t x_offset;//the max map size is 255, then offset have range: -255 to 255, see Map_loader::tryLoadMap() check size
        };
        struct Map_BorderContent_LeftRight
        {
            CommonMap *map;
            int16_t y_offset;//the max map size is 255, then offset have range: -255 to 255, see Map_loader::tryLoadMap() check size
        };
        Map_BorderContent_TopBottom top;
        Map_BorderContent_TopBottom bottom;
        Map_BorderContent_LeftRight left;
        Map_BorderContent_LeftRight right;
    };
    struct Teleporter
    {
        uint8_t source_x,source_y;/*source*/
        uint8_t destination_x,destination_y;/*destination*/
        CommonMap *map;
        MapCondition condition;
    };

    Map_Border border;

    Teleporter* teleporter;//for very small list < 20 teleporter, it's this structure the more fast, code not ready for more than 127
    uint8_t teleporter_list_size;

    std::string map_file;
    uint8_t width;//why uint16_t if a map is not allowed be more than 255?
    uint8_t height;//why uint16_t if a map is not allowed be more than 255?
    //uint32_t group;
    uint32_t id;//on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id and find the right pointer

    //the index is position (x+y*width)
    ParsedLayer parsed_layer;

    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> learn;
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> market;
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::string,pairhash> zonecapture;
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> botsFight;
    std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> botsFightTrigger;//trigger line in front of bot fight

    static void removeParsedLayer(const ParsedLayer &parsed_layer);
};
}

#endif // MAP_H
