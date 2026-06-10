#include <unordered_map>
#include <mapobject.h>

#include <maprenderer.h>

#include <objectgroup.h>

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

    //the server runs the player visibility cache without coherency to improve
    //performance: two players can carry the same simplified id, and the
    //resulting load/unload sequences can leave a MapObject not (or no longer)
    //linked to its item. The Z fix is visual-only: silently skip in that case
    //instead of objectLink.at() throwing (which would crash the client).
    static void setZValueIfLinked(Tiled::MapObject *object,const qreal &z);
};

#endif
