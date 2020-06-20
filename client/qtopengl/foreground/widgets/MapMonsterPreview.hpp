#ifndef MapMonsterPreview_H
#define MapMonsterPreview_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include "../../../../general/base/GeneralStructures.hpp"

class MapMonsterPreview : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    MapMonsterPreview(const CatchChallenger::PlayerMonster &m,QGraphicsItem * parent = 0);
    ~MapMonsterPreview();
    void regenCache();
    void mousePressEventXY(const QPointF &p,bool &pressValidated);
    void mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated);
    void setInDrag(bool drag);
    bool isPressed();
private:
    void setPressed(const bool &pressed);
private:
    CatchChallenger::PlayerMonster monster;
    bool pressed;
    bool m_drag;
signals:
    void clicked();
};

#endif // MapMonsterPreview_H
