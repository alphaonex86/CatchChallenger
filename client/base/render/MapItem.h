#include "MapObjectItem.h"
#include "ObjectGroupItem.h"
#include "TileLayerItem.h"

#include "libtiled/isometricrenderer.h"
#include "libtiled/map.h"
#include "libtiled/mapobject.h"
#include "libtiled/mapreader.h"
#include "libtiled/objectgroup.h"
#include "libtiled/orthogonalrenderer.h"
#include "libtiled/tilelayer.h"
#include "libtiled/tileset.h"

#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QSet>
#include <QPair>
#include <QMultiMap>
#include <QHash>

class MapItem : public QGraphicsItem
{
public:
    MapItem(QGraphicsItem *parent = 0,const bool &useCache=true);
    void addMap(Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex);
    bool haveMap(Tiled::Map *map);
    void removeMap(Tiled::Map *map);
    void setMapPosition(Tiled::Map *map, qint16 x, qint16 y);
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
private:
    QMultiMap<Tiled::Map *,QGraphicsItem *> displayed_layer;
    bool cache;
};

#endif
