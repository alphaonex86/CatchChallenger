#include <tiled/isometricrenderer.h>
#include <tiled/map.h>
#include <tiled/mapobject.h>
#include <tiled/mapreader.h>
#include <tiled/objectgroup.h>
#include <tiled/orthogonalrenderer.h>
#include <tiled/tilelayer.h>
#include <tiled/tileset.h>

#ifndef TILELAYERITEM_H
#define TILELAYERITEM_H

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
 * Item that represents a tile layer.
 */
class TileLayerItem : public QGraphicsItem
{
public:
    TileLayerItem(Tiled::TileLayer *tileLayer, Tiled::MapRenderer *renderer, QGraphicsItem *parent = 0);
    QRectF boundingRect() const;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *);
private:
    Tiled::TileLayer *mTileLayer;
    Tiled::MapRenderer *mRenderer;
};

#endif
