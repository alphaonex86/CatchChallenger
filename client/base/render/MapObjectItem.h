#include "libtiled/isometricrenderer.h"
#include "libtiled/map.h"
#include "libtiled/mapobject.h"
#include "libtiled/mapreader.h"
#include "libtiled/objectgroup.h"
#include "libtiled/orthogonalrenderer.h"
#include "libtiled/tilelayer.h"
#include "libtiled/tileset.h"

#ifndef MAPOBJECTITEM_H
#define MAPOBJECTITEM_H

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QSet>
#include <QMultiMap>
#include <QHash>

/**
 * Item that represents a map object.
 */
class MapObjectItem : public QGraphicsItem
{
public:
    MapObjectItem(Tiled::MapObject *mapObject,QGraphicsItem *parent = 0);
    QRectF boundingRect() const;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *);
private:
    Tiled::MapObject *mMapObject;
public:
    //interresting if have lot of object by group layer
    static QHash<Tiled::ObjectGroup *,Tiled::MapRenderer *> mRendererList;

    //the link tiled with viewer
    static QHash<Tiled::MapObject *,MapObjectItem *> objectLink;
};

#endif
