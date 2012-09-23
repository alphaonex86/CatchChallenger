#include "../../general/libtiled/isometricrenderer.h"
#include "../../general/libtiled/map.h"
#include "../../general/libtiled/mapobject.h"
#include "../../general/libtiled/mapreader.h"
#include "../../general/libtiled/objectgroup.h"
#include "../../general/libtiled/orthogonalrenderer.h"
#include "../../general/libtiled/tilelayer.h"
#include "../../general/libtiled/tileset.h"

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
