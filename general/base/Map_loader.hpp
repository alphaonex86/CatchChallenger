#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <vector>
#include <string>

#include "GeneralStructures.hpp"
#include "CommonMap.hpp"
#include "lib.h"
#include "../tinyXML2/tinyxml2.hpp"

#ifdef CATCHCHALLENGER_CLASS_GATEWAY
#error This kind don't need reply on that's, not need load the map content
#endif
#ifdef CATCHCHALLENGER_CLASS_LOGIN
#error This kind don't need reply on that's, not need load the map content
#endif
#ifdef CATCHCHALLENGER_CLASS_MASTER
#error This kind don't need reply on that's, not need load the map content
#endif

namespace CatchChallenger {
struct Map_semi_border_content_top_bottom
{
    std::string fileName;
    int16_t x_offset;//can be negative, it's an offset!
};
struct Map_semi_border_content_left_right
{
    std::string fileName;
    int16_t y_offset;//can be negative, it's an offset!
};
struct Map_semi_border
{
    Map_semi_border_content_top_bottom top;
    Map_semi_border_content_top_bottom bottom;
    Map_semi_border_content_left_right left;
    Map_semi_border_content_left_right right;
};
struct Map_semi_teleport
{
    COORD_TYPE source_x,source_y;
    COORD_TYPE destination_x,destination_y;
    std::string map;
    MapCondition condition;
};

class DLL_PUBLIC Map_to_send : public BaseMap
{
public:
    Map_semi_border border;
    //std::stringList other_map;//border and not

    //all the variables after have to resolv map/zone
    std::string zoneName;
    std::string file;
    //std::vector<uint8_t> simplifiedMap; into CommonMap::flat_simplified_map and monstersCollisionMap?

    std::unordered_map<std::string,std::string> property;

    std::vector<Map_semi_teleport> teleport;

    //load just after the xml extra file
    struct Bot_Semi
    {
        std::pair<uint8_t,uint8_t> point;
        uint8_t id;
        std::unordered_map<std::string,std::string> property_text;
        std::unordered_map<uint8_t,const tinyxml2::XMLElement *> steps;
    };
    std::vector<Bot_Semi> bots;//to pass pos to xml file, used into colision too
    struct ItemOnMap_Semi
    {
        bool infinite;
        bool visible;
        CATCHCHALLENGER_TYPE_ITEM item;
        std::pair<uint8_t,uint8_t> point;
    };
    std::vector<ItemOnMap_Semi> items;//used into colision

    //loaded into CommonMap::flat_simplified_map and BaseMap::ParsedLayer
    uint8_t *monstersCollisionMap;

    const tinyxml2::XMLElement * xmlRoot;
};

struct Map_semi
{
    //conversion x,y to position: x+y*width
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    std::string file;
    Map_semi_border border;
    Map_to_send old_map;
};

class DLL_PUBLIC Map_loader
{
public:
    explicit Map_loader();
    ~Map_loader();

    Map_to_send map_to_send;
    std::string errorString();
    template<class MapType>
    bool tryLoadMap(const std::string &file, MapType *mapFinal, const bool &botIsNotWalkable);
    bool loadExtraXml(const std::string &file, std::vector<Map_to_send::Bot_Semi> &botslist, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer,std::string &zoneName);
    static std::string resolvRelativeMap(const std::string &file, const std::string &link, const std::string &datapackPath=std::string());
    static MapCondition xmlConditionToMapCondition(const std::string &conditionFile,const tinyxml2::XMLElement * const item);
    std::vector<MapMonster> loadSpecificMonster(const std::string &fileName,const std::string &monsterType);
    static std::unordered_map<std::string/*file*/, std::unordered_map<uint16_t/*id*/,tinyxml2::XMLElement *> > teleportConditionsUnparsed;

    template<class MapType>
    static void loadAllMapsAndLink(const std::string &datapack_mapPath, std::vector<Map_semi> &semi_loaded_map,std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);

    //for tiled
    #ifdef TILED_ZLIB
    static int32_t decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
    #endif

private:
    std::string error;
    std::unordered_map<std::string,uint16_t> zoneNumber;//sub zone part into same map, NOT linked with zone/
};
}

#endif // MAP_LOADER_H
