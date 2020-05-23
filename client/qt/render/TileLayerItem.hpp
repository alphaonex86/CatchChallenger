#include "../tiled/tiled_isometricrenderer.hpp"
#include "../tiled/tiled_map.hpp"
#include "../tiled/tiled_mapobject.hpp"
#include "../tiled/tiled_mapreader.hpp"
#include "../tiled/tiled_objectgroup.hpp"
#include "../tiled/tiled_orthogonalrenderer.hpp"
#include "../tiled/tiled_tilelayer.hpp"
#include "../tiled/tiled_tileset.hpp"

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
