#ifndef LAYERITEM_H
#define LAYERITEM_H

#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <cmath>

#include "orthogonalrenderer.h"
#include "tilelayer.h"
#include "tile.h"

/**
 * Item that represents a tile layer.
 */
using namespace Tiled;

class TileLayerItem : public QGraphicsItem
{
public:
    TileLayerItem(TileLayer *tileLayer, QGraphicsItem *parent = 0);
    ~TileLayerItem();
    QRectF boundingRect() const;

    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *);
    QString Name();

private:
    TileLayer *mTileLayer;
};

#endif // LAYERITEM_H
