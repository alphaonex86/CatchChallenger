#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <vector>
#include <string>

#include "GeneralStructures.h"
#include "GeneralStructuresXml.h"
#include "CommonMap.h"

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QObject>
#endif

namespace CatchChallenger {
class Map_loader
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
#ifndef EPOLLCATCHCHALLENGERSERVER
Q_OBJECT
#endif
public:
    explicit Map_loader();
    ~Map_loader();

    Map_to_send map_to_send;
    std::string errorString();
    bool tryLoadMap(const std::string &file, const bool &botIsNotWalkable=true);
    static void removeMapLayer(const ParsedLayer &parsed_layer);
    bool loadMonsterMap(const std::string &file, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer);
    static std::string resolvRelativeMap(const std::string &file, const std::string &link, const std::string &datapackPath=std::string());
    static CATCHCHALLENGER_XMLELEMENT * getXmlCondition(const std::string &fileName, const std::string &file, const uint32_t &conditionId);
    static MapCondition xmlConditionToMapCondition(const std::string &conditionFile,const CATCHCHALLENGER_XMLELEMENT * const item);
    std::vector<MapMonster> loadSpecificMonster(const std::string &fileName,const std::string &monsterType);
    static std::unordered_map<std::string/*file*/, std::unordered_map<uint32_t/*id*/,CATCHCHALLENGER_XMLELEMENT *> > teleportConditionsUnparsed;
private:
    std::string error;
    std::unordered_map<std::string,uint8_t> zoneNumber;
};
}

#endif // MAP_LOADER_H
