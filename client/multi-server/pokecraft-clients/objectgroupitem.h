#ifndef OBJECTGROUPITEM_H
#define OBJECTGROUPITEM_H

#include <QGraphicsItem>

#include "objectgroup.h"
#include "objectitem.h"

using namespace Tiled;

class ObjectGroupItem : public QGraphicsItem
{
public:
    ObjectGroupItem(ObjectGroup *objectGroup,QGraphicsItem *parent = 0);
    ~ObjectGroupItem(){foreach(QGraphicsItem *item,this->childItems()){this->scene()->removeItem(item);delete item;}}
    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

#endif // OBJECTGROUPITEM_H
