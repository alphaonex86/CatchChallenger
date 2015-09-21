#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <QObject>
#include <vector>
#include <string>
#include <QDomElement>

#include "GeneralStructures.h"
#include "GeneralStructuresXml.h"
#include "CommonMap.h"

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
    bool tryLoadMap(const std::string &fileName);
    static void removeMapLayer(const ParsedLayer &parsed_layer);
    bool loadMonsterMap(const std::string &fileName,std::vector<std::string> detectedMonsterCollisionMonsterType,std::vector<std::string> detectedMonsterCollisionLayer);
    static std::string resolvRelativeMap(const std::string &fileName,const std::string &link,const std::string &datapackPath=std::string());
    static QDomElement getXmlCondition(const std::string &fileName,const std::string &conditionFile,const uint32_t &conditionId);
    static MapCondition xmlConditionToMapCondition(const std::string &conditionFile, const QDomElement &item);
    std::vector<MapMonster> loadSpecificMonster(const std::string &fileName,const std::string &monsterType);
    static std::unordered_map<std::string/*file*/, std::unordered_map<uint32_t/*id*/,QDomElement> > teleportConditionsUnparsed;
private:
    static uint32_t decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &outputSize);
    std::string error;
    std::unordered_map<std::string,uint8_t> zoneNumber;

    static const std::string text_map;
    static const std::string text_width;
    static const std::string text_height;
    static const std::string text_properties;
    static const std::string text_property;
    static const std::string text_name;
    static const std::string text_value;
    static const std::string text_objectgroup;
    static const std::string text_Moving;
    static const std::string text_object;
    static const std::string text_x;
    static const std::string text_y;
    static const std::string text_type;
    static const std::string text_borderleft;
    static const std::string text_borderright;
    static const std::string text_bordertop;
    static const std::string text_borderbottom;
    static const std::string text_teleport_on_push;
    static const std::string text_teleport_on_it;
    static const std::string text_door;
    static const std::string text_condition_file;
    static const std::string text_condition_id;
    static const std::string text_rescue;
    static const std::string text_bot_spawn;
    static const std::string text_Object;
    static const std::string text_bot;
    static const std::string text_skin;
    static const std::string text_lookAt;
    static const std::string text_bottom;
    static const std::string text_file;
    static const std::string text_id;
    static const std::string text_layer;
    static const std::string text_encoding;
    static const std::string text_compression;
    static const std::string text_base64;
    static const std::string text_Walkable;
    static const std::string text_Collisions;
    static const std::string text_Water;
    static const std::string text_Grass;
    static const std::string text_Dirt;
    static const std::string text_LedgesRight;
    static const std::string text_LedgesLeft;
    static const std::string text_LedgesBottom;
    static const std::string text_LedgesDown;
    static const std::string text_LedgesTop;
    static const std::string text_LedgesUp;
    static const std::string text_grass;
    static const std::string text_monster;
    static const std::string text_minLevel;
    static const std::string text_maxLevel;
    static const std::string text_level;
    static const std::string text_luck;
    static const std::string text_water;
    static const std::string text_cave;
    static const std::string text_condition;
    static const std::string text_quest;
    static const std::string text_item;
    static const std::string text_fightBot;
    static const std::string text_clan;
    static const std::string text_dottmx;
    static const std::string text_dotxml;
    static const std::string text_slash;
    static const std::string text_percent;
    static const std::string text_data;
    static const std::string text_dotcomma;
    static const std::string text_false;
    static const std::string text_true;
    static const std::string text_visible;
    static const std::string text_infinite;
    static const std::string text_tileheight;
    static const std::string text_tilewidth;
};
}

#endif // MAP_LOADER_H
