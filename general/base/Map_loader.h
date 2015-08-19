#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <QObject>
#include <vector>
#include <string>
#include <QDomElement>

#include "DebugClass.h"
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
    std::basic_string<char> errorString();
    bool tryLoadMap(const std::basic_string<char> &fileName);
    static void removeMapLayer(const ParsedLayer &parsed_layer);
    bool loadMonsterMap(const std::basic_string<char> &fileName,std::vector<std::basic_string<char>> detectedMonsterCollisionMonsterType,std::vector<std::basic_string<char>> detectedMonsterCollisionLayer);
    static std::basic_string<char> resolvRelativeMap(const std::basic_string<char> &fileName,const std::basic_string<char> &link,const std::basic_string<char> &datapackPath=std::basic_string<char>());
    static QDomElement getXmlCondition(const std::basic_string<char> &fileName,const std::basic_string<char> &conditionFile,const quint32 &conditionId);
    static MapCondition xmlConditionToMapCondition(const std::basic_string<char> &conditionFile, const QDomElement &item);
    std::vector<MapMonster> loadSpecificMonster(const std::basic_string<char> &fileName,const std::basic_string<char> &monsterType);
    static std::unordered_map<std::basic_string<char>/*file*/, std::unordered_map<quint32/*id*/,QDomElement> > teleportConditionsUnparsed;
private:
    QByteArray decompress(const QByteArray &data, int expectedSize);
    std::basic_string<char> error;
    std::unordered_map<std::basic_string<char>,quint8> zoneNumber;

    static const std::basic_string<char> text_map;
    static const std::basic_string<char> text_width;
    static const std::basic_string<char> text_height;
    static const std::basic_string<char> text_properties;
    static const std::basic_string<char> text_property;
    static const std::basic_string<char> text_name;
    static const std::basic_string<char> text_value;
    static const std::basic_string<char> text_objectgroup;
    static const std::basic_string<char> text_Moving;
    static const std::basic_string<char> text_object;
    static const std::basic_string<char> text_x;
    static const std::basic_string<char> text_y;
    static const std::basic_string<char> text_type;
    static const std::basic_string<char> text_borderleft;
    static const std::basic_string<char> text_borderright;
    static const std::basic_string<char> text_bordertop;
    static const std::basic_string<char> text_borderbottom;
    static const std::basic_string<char> text_teleport_on_push;
    static const std::basic_string<char> text_teleport_on_it;
    static const std::basic_string<char> text_door;
    static const std::basic_string<char> text_condition_file;
    static const std::basic_string<char> text_condition_id;
    static const std::basic_string<char> text_rescue;
    static const std::basic_string<char> text_bot_spawn;
    static const std::basic_string<char> text_Object;
    static const std::basic_string<char> text_bot;
    static const std::basic_string<char> text_skin;
    static const std::basic_string<char> text_lookAt;
    static const std::basic_string<char> text_bottom;
    static const std::basic_string<char> text_file;
    static const std::basic_string<char> text_id;
    static const std::basic_string<char> text_layer;
    static const std::basic_string<char> text_encoding;
    static const std::basic_string<char> text_compression;
    static const std::basic_string<char> text_base64;
    static const std::basic_string<char> text_Walkable;
    static const std::basic_string<char> text_Collisions;
    static const std::basic_string<char> text_Water;
    static const std::basic_string<char> text_Grass;
    static const std::basic_string<char> text_Dirt;
    static const std::basic_string<char> text_LedgesRight;
    static const std::basic_string<char> text_LedgesLeft;
    static const std::basic_string<char> text_LedgesBottom;
    static const std::basic_string<char> text_LedgesDown;
    static const std::basic_string<char> text_LedgesTop;
    static const std::basic_string<char> text_LedgesUp;
    static const std::basic_string<char> text_grass;
    static const std::basic_string<char> text_monster;
    static const std::basic_string<char> text_minLevel;
    static const std::basic_string<char> text_maxLevel;
    static const std::basic_string<char> text_level;
    static const std::basic_string<char> text_luck;
    static const std::basic_string<char> text_water;
    static const std::basic_string<char> text_cave;
    static const std::basic_string<char> text_condition;
    static const std::basic_string<char> text_quest;
    static const std::basic_string<char> text_item;
    static const std::basic_string<char> text_fightBot;
    static const std::basic_string<char> text_clan;
    static const std::basic_string<char> text_dottmx;
    static const std::basic_string<char> text_dotxml;
    static const std::basic_string<char> text_slash;
    static const std::basic_string<char> text_percent;
    static const std::basic_string<char> text_data;
    static const std::basic_string<char> text_dotcomma;
    static const std::basic_string<char> text_false;
    static const std::basic_string<char> text_true;
    static const std::basic_string<char> text_visible;
    static const std::basic_string<char> text_infinite;
    static const std::basic_string<char> text_tileheight;
    static const std::basic_string<char> text_tilewidth;
};
}

#endif // MAP_LOADER_H
