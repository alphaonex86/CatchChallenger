#include "MapObjectItem.hpp"
#include "../background/CCMap.hpp"
#include <iostream>

std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> MapObjectItem::mRendererList;
std::unordered_map<Tiled::MapObject *,MapObjectItem *> MapObjectItem::objectLink;
const void * MapObjectItem::playerObject=nullptr;
qreal MapObjectItem::scale=1.0;
qreal MapObjectItem::x=0.0;
qreal MapObjectItem::y=0.0;

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

void MapObjectItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    /// \warning performance problem here
    if(!mMapObject->isVisible())
        return;
    //if anymore into a group
    if(mMapObject->objectGroup()==NULL)
        return;
    /*int w=child->boundingRect().width();
    int h=child->boundingRect().height();
    MapObjectItem::playerObject=mapController.getPlayerMapObject();
    if(x<=widget->width() && y<=widget->height())// && (x+w)>=0 && (y+h)>=0*/
    qreal x=(mMapObject->x()*16+this->x)*this->scale;//convert to pixel
    qreal y=((mMapObject->y()-1)*16+this->y)*this->scale;
    qreal w=16*this->scale;
    qreal h=32*this->scale;
    QRectF objectSpace(x,y,w,h);
    //only draw in visible space
    if(objectSpace.x()<=widget->width() && objectSpace.y()<=widget->height() &&
        (objectSpace.x()+objectSpace.width())>=0.0 && (objectSpace.y()+objectSpace.height())>=0.0)
    {
        Tiled::MapRenderer *r=mRendererList.at(mMapObject->objectGroup());
        r->drawMapObject(p, mMapObject, Qt::transparent);
    }

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

const Tiled::MapObject * MapObjectItem::getMapObject() const
{
    return mMapObject;
}
