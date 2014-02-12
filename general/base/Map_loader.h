#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QDomElement>

#include "DebugClass.h"
#include "GeneralStructures.h"
#include "CommonMap.h"

namespace CatchChallenger {
class Map_loader : public QObject
{
    Q_OBJECT
public:
    explicit Map_loader();
    ~Map_loader();

    Map_to_send map_to_send;
    QString errorString();
    bool tryLoadMap(const QString &fileName);
    bool loadMonsterMap(const QString &fileName,QList<QString> detectedMonsterCollisionMonsterType,QList<QString> detectedMonsterCollisionLayer);
    static QString resolvRelativeMap(const QString &fileName,const QString &link,const QString &datapackPath=QStringLiteral(""));
    static QDomElement getXmlCondition(const QString &fileName,const QString &conditionFile,const quint32 &conditionId);
    static MapCondition xmlConditionToMapCondition(const QString &conditionFile, const QDomElement &item);
private:
    QByteArray decompress(const QByteArray &data, int expectedSize);
    QString error;
    QHash<QString,quint8> zoneNumber;

    static QString text_map;
    static QString text_width;
    static QString text_height;
    static QString text_properties;
    static QString text_property;
    static QString text_name;
    static QString text_value;
    static QString text_objectgroup;
    static QString text_Moving;
    static QString text_object;
    static QString text_x;
    static QString text_y;
    static QString text_type;
    static QString text_borderleft;
    static QString text_borderright;
    static QString text_bordertop;
    static QString text_borderbottom;
    static QString text_teleport_on_push;
    static QString text_teleport_on_it;
    static QString text_door;
    static QString text_condition_file;
    static QString text_condition_id;
    static QString text_rescue;
    static QString text_bot_spawn;
    static QString text_Object;
    static QString text_bot;
    static QString text_skin;
    static QString text_lookAt;
    static QString text_bottom;
    static QString text_file;
    static QString text_id;
    static QString text_layer;
    static QString text_encoding;
    static QString text_compression;
    static QString text_base64;
    static QString text_Walkable;
    static QString text_Collisions;
    static QString text_Water;
    static QString text_Grass;
    static QString text_Dirt;
    static QString text_LedgesRight;
    static QString text_LedgesLeft;
    static QString text_LedgesBottom;
    static QString text_LedgesDown;
    static QString text_LedgesTop;
    static QString text_LedgesUp;
    static QString text_grass;
    static QString text_monster;
    static QString text_minLevel;
    static QString text_maxLevel;
    static QString text_level;
    static QString text_luck;
    static QString text_water;
    static QString text_cave;
    static QString text_condition;
    static QString text_quest;
    static QString text_item;
    static QString text_fightBot;
    static QString text_clan;
    static QString text_dottmx;
    static QString text_slash;
    static QString text_percent;
    static QString text_data;
};
}

#endif // MAP_LOADER_H
