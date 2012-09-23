#ifndef OBJECTITEM_H
#define OBJECTITEM_H

#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QGraphicsScene>

#include "orthogonalrenderer.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "tile.h"
#include "gamedata.h"

using namespace Tiled;

class ObjectItem : public QGraphicsItem
{
public:
        ObjectItem(MapObject *mapObject,QGraphicsItem *parent = 0);
        ~ObjectItem();
        QRectF boundingRect() const;


        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

    private:
        MapObject *mMapObject;

};

#endif // OBJECTITEM_H
