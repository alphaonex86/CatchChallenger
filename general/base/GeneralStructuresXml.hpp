#ifndef CATCHCHALLENGER_GENERAL_STRUCTURES_XML_H
#define CATCHCHALLENGER_GENERAL_STRUCTURES_XML_H

#include <vector>
#include <string>
#include <unordered_map>

#include "../tinyXML2/tinyxml2.hpp"
#include "GeneralType.hpp"
#include "GeneralStructures.hpp"
#include "GeneralVariable.hpp"

namespace CatchChallenger {

struct Map_semi_teleport
{
    COORD_TYPE source_x,source_y;
    COORD_TYPE destination_x,destination_y;
    std::string map;
    const tinyxml2::XMLElement * conditionUnparsed;
    MapCondition condition;
};

struct Map_to_send
{
    Map_semi_border border;
    //std::stringList other_map;//border and not

    //uint32_t because the format allow it, checked into tryLoadMap()
    uint32_t width;
    uint32_t height;
    std::string zoneName;

    std::unordered_map<std::string,std::string> property;

    ParsedLayer parsed_layer;

    std::vector<Map_semi_teleport> teleport;

    struct Map_Point
    {
        COORD_TYPE x,y;
    };
    std::vector<Map_Point> rescue_points;

    struct Bot_Semi
    {
        Map_Point point;
        std::string file;
        uint16_t id;
        std::unordered_map<std::string,std::string> property_text;
    };
    std::vector<Bot_Semi> bots;

    struct ItemOnMap_Semi
    {
        Map_Point point;
        CATCHCHALLENGER_TYPE_ITEM item;
        bool visible;
        bool infinite;
    };
    std::vector<ItemOnMap_Semi> items;//list to keep to keep the order to do the indexOfItemOnMap to send to player, use less bandwith due to send uint8_t not map,x,y
    //used only on server
    struct DirtOnMap_Semi
    {
        Map_Point point;
    };
    std::vector<DirtOnMap_Semi> dirts;//list to keep to keep the order to do the indexOfDirtsOnMap to send to player, use less bandwith due to send uint8_t not map,x,y

    uint8_t *monstersCollisionMap;
    std::vector<MonstersCollisionValue> monstersCollisionList;

    const tinyxml2::XMLElement * xmlRoot;
};

//permanent bot on client, temp to parse on the server
struct Bot
{
    std::unordered_map<uint8_t,const tinyxml2::XMLElement *> step;
    std::unordered_map<std::string,std::string> properties;
    uint16_t botId;//id need be unique for the quests, then 32Bits
    std::string skin;
    std::string name;
};

}
#endif // STRUCTURES_GENERAL_H
