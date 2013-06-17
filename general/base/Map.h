#ifndef CATCHCHALLENGER_MAP_H
#define CATCHCHALLENGER_MAP_H

#include <QString>
#include <QHash>
#include <QList>
#include <QByteArray>

#include "GeneralStructures.h"

namespace CatchChallenger {
class Map
{
public:
    //the index is position (x+y*width)
    ParsedLayer parsed_layer;

    struct Map_Border
    {
        struct Map_BorderContent_TopBottom
        {
            Map *map;
            qint32 x_offset;
        };
        struct Map_BorderContent_LeftRight
        {
            Map *map;
            qint32 y_offset;
        };
        Map_BorderContent_TopBottom top;
        Map_BorderContent_TopBottom bottom;
        Map_BorderContent_LeftRight left;
        Map_BorderContent_LeftRight right;
    };
    Map_Border border;

    QList<Map *> near_map;//not only the border
    struct Teleporter
    {
        quint32 x,y;
        Map *map;
    };
    QHash<quint32,Teleporter> teleporter;//the int (x+y*width) is position

    QString map_file;
    quint16 width;
    quint16 height;
    quint32 group;
    quint32 id;

    QList<MapMonster> grassMonster;
    QList<MapMonster> waterMonster;
    QList<MapMonster> caveMonster;
    QMultiHash<QPair<quint8,quint8>,quint32> botsFightTrigger;//trigger line in front of bot fight

    static void removeParsedLayer(const ParsedLayer &parsed_layer);
};
}

#endif // MAP_H
