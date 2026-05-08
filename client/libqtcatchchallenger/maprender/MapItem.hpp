#include "MapObjectItem.hpp"
#include "MapVisualiserOrder.hpp"
#include "ObjectGroupItem.hpp"
#include "TileLayerItem.hpp"
#include <map.h>
#include "../libcatchchallenger/ClientStructures.hpp"

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
#include <QGraphicsObject>
#include <unordered_map>
#include <unordered_set>

class MapItem : public QGraphicsObject
{
    Q_OBJECT
public:
    MapItem(QGraphicsItem *parent = 0,const bool &useCache=true);
    void addMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex);
    bool haveMap(Tiled::Map *map);
    void removeMap(Tiled::Map *map);
    void setMapPosition(Tiled::Map *map, int16_t global_x, int16_t global_y);
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    //Tilesets currently safe to deref via raw `Tileset*` from a Tiled::Cell.
    //MapObjectItem::paint() validates cell.tileset() against this map to skip
    //paints whose cell points at a tileset that's been freed by an async map
    //unload / OtherPlayer turnover — accessing such a stale Tileset* would
    //crash inside Cell::tile() (Tileset::findTile reads mTilesById, a
    //red-black tree whose root is now freed memory).
    //
    //The value type is a SharedTileset (== QSharedPointer<Tileset>) so each
    //entry holds a strong ref: while a Tileset is in validTilesets_, the
    //refcount can't drop to zero, the underlying Tileset can't be freed,
    //and any cell whose raw `Tileset*` matches a registered key is safe.
    //Callers who own a SharedTileset can let go of their copy at any time;
    //the Tileset survives until validTilesets_.erase() is called for that
    //pointer (typically in MapItem::removeMap or remove_player_final).
    //
    //Why we don't store SharedTileset *only* and skip the raw key: lookups
    //come from `Tiled::Cell::tileset()` which returns a raw `Tileset*` —
    //we need to match by pointer identity, not by SharedPtr.
    static std::unordered_map<Tiled::Tileset *,Tiled::SharedTileset> validTilesets_;
private:
    std::unordered_map<Tiled::Map *,std::unordered_set<QGraphicsItem *> > displayed_layer;
    std::unordered_map<Tiled::Map *,std::unordered_set<Tiled::Tileset *> > tilesetsPerMap_;
    bool cache;
signals:
    void eventOnMap(CatchChallenger::MapEvent event,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,COORD_TYPE x,COORD_TYPE y);
};

#endif
