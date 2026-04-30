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
    //per-map tilesets currently held alive by addMap (released in removeMap).
    //MapObjectItem::paint() validates cell.tileset() against this set to skip
    //paints whose cell points at a tileset that's been freed by an async map
    //unload — accessing such a stale Tileset* would crash inside Cell::tile().
    static std::unordered_set<Tiled::Tileset *> validTilesets_;
private:
    std::unordered_map<Tiled::Map *,std::unordered_set<QGraphicsItem *> > displayed_layer;
    std::unordered_map<Tiled::Map *,std::unordered_set<Tiled::Tileset *> > tilesetsPerMap_;
    bool cache;
signals:
    void eventOnMap(CatchChallenger::MapEvent event,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,COORD_TYPE x,COORD_TYPE y);
};

#endif
