#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <vector>
#include <string>

#include "GeneralStructures.hpp"
#include "GeneralStructuresXml.hpp"
#include "CommonMap.hpp"

#ifdef CATCHCHALLENGER_CLASS_GATEWAY
#error This kind don't need reply on that's
#endif
#ifdef CATCHCHALLENGER_CLASS_LOGIN
#error This kind don't need reply on that's
#endif
#ifdef CATCHCHALLENGER_CLASS_MASTER
#error This kind don't need reply on that's
#endif

namespace CatchChallenger {
class Map_loader
{
public:
    explicit Map_loader();
    ~Map_loader();

    Map_to_send map_to_send;
    std::string errorString();
    bool tryLoadMap(const std::string &file, const bool &botIsNotWalkable=true);
    static void removeMapLayer(const ParsedLayer &parsed_layer);
    bool loadMonsterOnMapAndExtra(const std::string &file, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer,std::string &zoneName);
    static std::string resolvRelativeMap(const std::string &file, const std::string &link, const std::string &datapackPath=std::string());
    static tinyxml2::XMLElement * getXmlCondition(const std::string &fileName, const std::string &file, const uint16_t &conditionId);
    static MapCondition xmlConditionToMapCondition(const std::string &conditionFile,const tinyxml2::XMLElement * const item);
    std::vector<MapMonster> loadSpecificMonster(const std::string &fileName,const std::string &monsterType);
    static std::unordered_map<std::string/*file*/, std::unordered_map<uint16_t/*id*/,tinyxml2::XMLElement *> > teleportConditionsUnparsed;

    //for tiled
    #ifdef TILED_ZLIB
    static int32_t decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
    #endif

private:
    std::string error;
    std::unordered_map<std::string,uint8_t> zoneNumber;
};
}

#endif // MAP_LOADER_H
