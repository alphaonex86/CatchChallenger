#include "libtiled/isometricrenderer.h"
#include "libtiled/map.h"
#include "libtiled/mapobject.h"
#include "libtiled/mapreader.h"
#include "libtiled/objectgroup.h"
#include "libtiled/orthogonalrenderer.h"
#include "libtiled/tilelayer.h"
#include "libtiled/tileset.h"

#ifndef OBJECTGROUPITEM_H
#define OBJECTGROUPITEM_H

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

class ObjectGroupItem : public QGraphicsItem
{
public:
    ObjectGroupItem(Tiled::ObjectGroup *objectGroup,QGraphicsItem *parent = 0);
    ~ObjectGroupItem();
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    bool isVisible() const;
    void addObject(Tiled::MapObject *object);
    void removeObject(Tiled::MapObject *object);
public:
    Tiled::ObjectGroup *mObjectGroup;

    //the link tiled with viewer
    static QHash<Tiled::ObjectGroup *,ObjectGroupItem *> objectGroupLink;
};

#endif
