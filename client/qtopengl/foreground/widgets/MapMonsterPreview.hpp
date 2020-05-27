#ifndef MapMonsterPreview_H
#define MapMonsterPreview_H

#include <QGraphicsPixmapItem>
#include "../../../../general/base/GeneralStructures.hpp"

class MapMonsterPreview : public QGraphicsPixmapItem
{
public:
    MapMonsterPreview(const CatchChallenger::PlayerMonster &m,QGraphicsItem * parent = 0);
    ~MapMonsterPreview();
    void regenCache();
private:
    CatchChallenger::PlayerMonster monster;
};

#endif // MapMonsterPreview_H
