#ifndef CATCHCHALLENGER_GENERAL_STRUCTURES_XML_H
#define CATCHCHALLENGER_GENERAL_STRUCTURES_XML_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QDomElement>
#include <QMultiHash>

#include "GeneralType.h"

#define COORD_TYPE quint8
#define SIMPLIFIED_PLAYER_ID_TYPE quint16
#define CLAN_ID_TYPE quint32
#define SPEED_TYPE quint8

namespace CatchChallenger {

struct Map_semi_teleport
{
    COORD_TYPE source_x,source_y;
    COORD_TYPE destination_x,destination_y;
    QString map;
    QDomElement conditionUnparsed;
    MapCondition condition;
};

struct Map_to_send
{
    Map_semi_border border;
    //QStringList other_map;//border and not

    //quint32 because the format allow it, checked into tryLoadMap()
    quint32 width;
    quint32 height;

    QHash<QString,QVariant> property;

    ParsedLayer parsed_layer;

    QList<Map_semi_teleport> teleport;

    struct Map_Point
    {
        COORD_TYPE x,y;
    };
    QList<Map_Point> rescue_points;

    struct Bot_Semi
    {
        Map_Point point;
        QString file;
        quint16 id;
        QHash<QString,QVariant> property_text;
    };
    QList<Bot_Semi> bots;

    struct ItemOnMap_Semi
    {
        Map_Point point;
        CATCHCHALLENGER_TYPE_ITEM item;
        bool visible;
        bool infinite;
    };
    QList<ItemOnMap_Semi> items;//list to keep to keep the order to do the indexOfItemOnMap to send to player, use less bandwith due to send quint8 not map,x,y
    //used only on server
    struct DirtOnMap_Semi
    {
        Map_Point point;
    };
    QList<DirtOnMap_Semi> dirts;//list to keep to keep the order to do the indexOfDirtsOnMap to send to player, use less bandwith due to send quint8 not map,x,y

    quint8 *monstersCollisionMap;
    QList<MonstersCollisionValue> monstersCollisionList;

    QDomElement xmlRoot;
};

//permanent bot on client, temp to parse on the server
struct Bot
{
    QHash<quint8,QDomElement> step;
    QHash<QString,QString> properties;
    quint32 botId;//id need be unique for the quests, then 32Bits
    QString skin;
    QString name;
};

}
#endif // STRUCTURES_GENERAL_H
