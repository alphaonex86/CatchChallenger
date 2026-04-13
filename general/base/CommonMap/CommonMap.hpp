#ifndef CATCHCHALLENGER_MAP_H
#define CATCHCHALLENGER_MAP_H

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "../GeneralStructures.hpp"
#include "../cpp11addition.hpp"
#include "../lib.h"
#include "Teleporter.hpp"
#include "BaseMap.hpp"
#include "Map_Border.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "../../tinyXML2/tinyxml2.hpp"
#endif

namespace CatchChallenger {

struct UnknownMovingEntry {
    std::string type;
    uint32_t object_x;
    uint32_t object_y;
    std::unordered_map<std::string,std::string> property_text;
};
struct UnknownObjectEntry {
    std::string type;
    uint32_t object_x;
    uint32_t object_y;
    std::unordered_map<std::string,std::string> property_text;
};
struct UnknownBotStepEntry {
    uint32_t object_x;
    uint32_t object_y;
    #ifndef CATCHCHALLENGER_NOXML
    const tinyxml2::XMLElement *step;
    #else
    const void *step;
    #endif
};

//the only visible map is loaded on client, all map on server
class DLL_PUBLIC CommonMap : public BaseMap
{
public:
    Map_Border border;

    /* CATCHCHALLENGER_TYPE_MAPID id; enable this generate crash because generate inconsistency between this var and the map list index
     * on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id
     * on server is QtDatapackClientLoader::datapackLoader->getMap(current_map)
     * if you have map object then you can look how was get to have the pointer */

    /* see flat_simplified_map
     * after resolution the index is position (x+y*width)
     * not optimal, but memory safe, simply 2 cache miss max
     */
    /* 0 walkable: index = 0 for monster is used into cave
     * 254 not walkable
     * 253 ParsedLayerLedges_LedgesBottom
     * 252 ParsedLayerLedges_LedgesTop
     * 251 ParsedLayerLedges_LedgesRight
     * 250 ParsedLayerLedges_LedgesLeft
     * 249 dirt
     * 200 - 248 reserved
     * 0 cave def
     * 1-199 monster def and condition */
    /* can't be pointer, server can have unique pointer, but client need another pointer or pointer mulitple in case of multi-bots
     * can be just uint32_t offset the map list, but by map */
    std::vector<uint8_t> flat_simplified_map;

    std::vector<Teleporter> teleporters;

    //buffer unknown entries for later processing by server
    std::vector<UnknownMovingEntry> unknownMovingBuffer;
    std::vector<UnknownObjectEntry> unknownObjectBuffer;
    #ifndef CATCHCHALLENGER_NOXML
    std::vector<UnknownBotStepEntry> unknownBotStepBuffer;
    #endif

    //extra parse function, buffer unknown data for later processing
    bool parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text);
    bool parseUnknownObject(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text);
    #ifndef CATCHCHALLENGER_NOXML
    bool parseUnknownBotStep(uint32_t object_x,uint32_t object_y,const tinyxml2::XMLElement *step);
    #endif

};
}

#endif // MAP_H
