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
            qint32 x_offset;
        };
        struct Map_BorderContent_LeftRight
        {
            CommonMap *map;
            qint32 y_offset;
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
        quint32 x,y;
        CommonMap *map;
        MapCondition condition;
    };
    std::unordered_map<quint32,Teleporter> teleporter;//the int (x+y*width) is position

    std::string map_file;
    quint16 width;
    quint16 height;
    quint32 group;
    quint32 id;

    std::unordered_multimap<std::pair<uint8_t,uint8_t>,uint32_t, pairhash> shops;
    std::unordered_set<std::pair<quint8,quint8>,pairhash> learn;
    std::unordered_set<std::pair<quint8,quint8>,pairhash> heal;
    std::unordered_set<std::pair<quint8,quint8>,pairhash> market;
    std::unordered_map<std::pair<quint8,quint8>,std::string,pairhash> zonecapture;
    std::unordered_multimap<std::pair<quint8,quint8>,quint32,pairhash> botsFight;

    /*std::vector<MapMonster> grassMonster;
    std::vector<MapMonster> waterMonster;
    std::vector<MapMonster> caveMonster;*/

    std::unordered_multimap<std::pair<quint8,quint8>,quint32,pairhash> botsFightTrigger;//trigger line in front of bot fight

    static void removeParsedLayer(const ParsedLayer &parsed_layer);
};
}

#endif // MAP_H
