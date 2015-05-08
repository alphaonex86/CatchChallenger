#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <QObject>
#include <QStringList>
#include <QString>
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
    QString errorString();
    bool tryLoadMap(const QString &fileName);
    static void removeMapLayer(const ParsedLayer &parsed_layer);
    bool loadMonsterMap(const QString &fileName,QList<QString> detectedMonsterCollisionMonsterType,QList<QString> detectedMonsterCollisionLayer);
    static QString resolvRelativeMap(const QString &fileName,const QString &link,const QString &datapackPath=QString());
    static QDomElement getXmlCondition(const QString &fileName,const QString &conditionFile,const quint32 &conditionId);
    static MapCondition xmlConditionToMapCondition(const QString &conditionFile, const QDomElement &item);
    QList<MapMonster> loadSpecificMonster(const QString &fileName,const QString &monsterType);
    static QHash<QString/*file*/, QHash<quint32/*id*/,QDomElement> > teleportConditionsUnparsed;
private:
    QByteArray decompress(const QByteArray &data, int expectedSize);
    QString error;
    QHash<QString,quint8> zoneNumber;

    static const QString text_map;
    static const QString text_width;
    static const QString text_height;
    static const QString text_properties;
    static const QString text_property;
    static const QString text_name;
    static const QString text_value;
    static const QString text_objectgroup;
    static const QString text_Moving;
    static const QString text_object;
    static const QString text_x;
    static const QString text_y;
    static const QString text_type;
    static const QString text_borderleft;
    static const QString text_borderright;
    static const QString text_bordertop;
    static const QString text_borderbottom;
    static const QString text_teleport_on_push;
    static const QString text_teleport_on_it;
    static const QString text_door;
    static const QString text_condition_file;
    static const QString text_condition_id;
    static const QString text_rescue;
    static const QString text_bot_spawn;
    static const QString text_Object;
    static const QString text_bot;
    static const QString text_skin;
    static const QString text_lookAt;
    static const QString text_bottom;
    static const QString text_file;
    static const QString text_id;
    static const QString text_layer;
    static const QString text_encoding;
    static const QString text_compression;
    static const QString text_base64;
    static const QString text_Walkable;
    static const QString text_Collisions;
    static const QString text_Water;
    static const QString text_Grass;
    static const QString text_Dirt;
    static const QString text_LedgesRight;
    static const QString text_LedgesLeft;
    static const QString text_LedgesBottom;
    static const QString text_LedgesDown;
    static const QString text_LedgesTop;
    static const QString text_LedgesUp;
    static const QString text_grass;
    static const QString text_monster;
    static const QString text_minLevel;
    static const QString text_maxLevel;
    static const QString text_level;
    static const QString text_luck;
    static const QString text_water;
    static const QString text_cave;
    static const QString text_condition;
    static const QString text_quest;
    static const QString text_item;
    static const QString text_fightBot;
    static const QString text_clan;
    static const QString text_dottmx;
    static const QString text_dotxml;
    static const QString text_slash;
    static const QString text_percent;
    static const QString text_data;
    static const QString text_dotcomma;
    static const QString text_false;
    static const QString text_true;
    static const QString text_visible;
    static const QString text_infinite;
    static const QString text_tileheight;
    static const QString text_tilewidth;
};
}

#endif // MAP_LOADER_H
