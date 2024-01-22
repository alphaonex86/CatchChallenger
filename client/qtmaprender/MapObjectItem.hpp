#include <unordered_map>
#include <libtiled/mapobject.h>
#include <libtiled/maprenderer.h>

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
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *);
private:
    Tiled::MapObject *mMapObject;
public:
    //interresting if have lot of object by group layer
    static std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> mRendererList;

    //the link tiled with viewer
    static std::unordered_map<Tiled::MapObject *,MapObjectItem *> objectLink;
};

#endif
