#include "../../tiled/tiled_isometricrenderer.h"
#include "../../tiled/tiled_map.h"
#include "../../tiled/tiled_mapobject.h"
#include "../../tiled/tiled_mapreader.h"
#include "../../tiled/tiled_objectgroup.h"
#include "../../tiled/tiled_orthogonalrenderer.h"
#include "../../tiled/tiled_tilelayer.h"
#include "../../tiled/tiled_tileset.h"

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
