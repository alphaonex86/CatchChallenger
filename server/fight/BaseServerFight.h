#include "../base/ServerStructures.h"

#ifndef CATCHCHALLENGER_BASESERVERFIGHT_H
#define CATCHCHALLENGER_BASESERVERFIGHT_H

namespace CatchChallenger {
class BaseServerFight
{
protected:
    virtual void load_monsters_max_id();
    virtual void preload_monsters_drops();
    virtual void check_monsters_map();
    virtual void unload_monsters_drops();

    QHash<quint32,MonsterDrops> loadMonsterDrop(const QString &file, QHash<quint32,Item> items,const QHash<quint32,Monster> &monsters);
protected:
    static QString text_list;
    static QString text_id;
    static QString text_monster;
    static QString text_monsters;
    static QString text_drops;
    static QString text_drop;
    static QString text_item;
    static QString text_quantity_min;
    static QString text_quantity_max;
    static QString text_luck;
    static QString text_percent;
};
}

#endif
