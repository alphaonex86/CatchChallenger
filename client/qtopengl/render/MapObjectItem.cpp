#include "MapObjectItem.hpp"
#include "../background/CCMap.hpp"
#include <iostream>

std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> MapObjectItem::mRendererList;
std::unordered_map<Tiled::MapObject *,MapObjectItem *> MapObjectItem::objectLink;

MapObjectItem::MapObjectItem(Tiled::MapObject *mapObject,
              QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mMapObject(mapObject)
{
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);//just change direction without move do bug
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

QRectF MapObjectItem::boundingRect() const
{
    return QRectF();
    Tiled::ObjectGroup * objectGroup=mMapObject->objectGroup();
    if(mRendererList.find(objectGroup)!=mRendererList.cend())
        return mRendererList.at(objectGroup)->boundingRect(mMapObject);
    else
        return QRectF();
}

void MapObjectItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    /// \warning performance problem here
    if(!mMapObject->isVisible())
        return;
    //if anymore into a group
    if(mMapObject->objectGroup()==NULL)
        return;

    Tiled::MapRenderer *r=mRendererList.at(mMapObject->objectGroup());
    r->drawMapObject(p, mMapObject, Qt::transparent);


    /*if(!this->cache.isNull())
    {
        p->drawPixmap(0,0,this->cache.width(),this->cache.height(),this->cache);
        return;
    }
    QImage cache(QSize(64*16,64*16),QImage::Format_ARGB32_Premultiplied);
    cache.fill(Qt::transparent);
    QPainter painter(&cache);

    if(!mMapObject->isVisible())
        return;
    //if anymore into a group
    if(mMapObject->objectGroup()==NULL)
        return;

    Tiled::MapRenderer *r=mRendererList.at(mMapObject->objectGroup());
    r->drawMapObject(&painter, mMapObject, Qt::transparent);

    this->cache=QPixmap::fromImage(cache);
    p->drawPixmap(0,0,this->cache.width(),this->cache.height(),this->cache);*/
}
