#include "MapObjectItem.hpp"
#include <libtiled/objectgroup.h>

std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> MapObjectItem::mRendererList;
std::unordered_map<Tiled::MapObject *,MapObjectItem *> MapObjectItem::objectLink;

MapObjectItem::MapObjectItem(Tiled::MapObject *mapObject,
              QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mMapObject(mapObject)
{
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);just change direction without move do bug
}

QRectF MapObjectItem::boundingRect() const
{
    Tiled::ObjectGroup * objectGroup=mMapObject->objectGroup();
    if(mRendererList.find(objectGroup)!=mRendererList.cend())
        return mRendererList.at(objectGroup)->boundingRect(mMapObject);
    else
        return QRectF();
}

void MapObjectItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(!mMapObject->isVisible())
        return;
    //if anymore into a group
    if(mMapObject->objectGroup()==NULL)
        return;

    const QColor &color = mMapObject->objectGroup()->color();
    mRendererList.at(mMapObject->objectGroup())->drawMapObject(p, mMapObject,
                             color.isValid() ? color : Qt::transparent);
}
