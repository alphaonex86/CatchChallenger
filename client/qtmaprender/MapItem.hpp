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
    void addMap(Map_full * tempMapObject,Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex);
    bool haveMap(Tiled::Map *map);
    void removeMap(Tiled::Map *map);
    void setMapPosition(Tiled::Map *map, int16_t x, int16_t y);
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
private:
    std::unordered_map<Tiled::Map *,std::unordered_set<QGraphicsItem *> > displayed_layer;
    bool cache;
signals:
    void eventOnMap(CatchChallenger::MapEvent event,Map_full * tempMapObject,uint8_t x,uint8_t y);
};

#endif
