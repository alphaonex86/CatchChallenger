#include "../tiled/tiled_isometricrenderer.hpp"
#include "../tiled/tiled_map.hpp"
#include "../tiled/tiled_mapobject.hpp"
#include "../tiled/tiled_mapreader.hpp"
#include "../tiled/tiled_objectgroup.hpp"
#include "../tiled/tiled_orthogonalrenderer.hpp"
#include "../tiled/tiled_tilelayer.hpp"
#include "../tiled/tiled_tileset.hpp"

#ifndef MAPOBJECTITEM_H
#define MAPOBJECTITEM_H

#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <unordered_map>

/**
 * Item that represents a map object.
 */
class MapObjectItem : public QGraphicsItem
{
public:
    MapObjectItem(Tiled::MapObject *mapObject,QGraphicsItem *parent = 0);
    QRectF boundingRect() const;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget);
    const Tiled::MapObject * getMapObject() const;

    static const void * playerObject;
    static qreal scale;
    static qreal x;
    static qreal y;
private:
    Tiled::MapObject *mMapObject;
    QPixmap cache;
public:
    //interresting if have lot of object by group layer
    static std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> mRendererList;

    //the link tiled with viewer
    static std::unordered_map<Tiled::MapObject *,MapObjectItem *> objectLink;
};

#endif
