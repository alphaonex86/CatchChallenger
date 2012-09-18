#include "MapObjectItem.h"

QHash<Tiled::ObjectGroup *,Tiled::MapRenderer *> MapObjectItem::mRendererList;
QHash<Tiled::MapObject *,MapObjectItem *> MapObjectItem::objectLink;

MapObjectItem::MapObjectItem(Tiled::MapObject *mapObject,
              QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mMapObject(mapObject)
{
}

QRectF MapObjectItem::boundingRect() const
{
    Tiled::ObjectGroup * objectGroup=mMapObject->objectGroup();
    if(mRendererList.contains(objectGroup))
        return mRendererList[objectGroup]->boundingRect(mMapObject);
    else
        return QRectF();
}

void MapObjectItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(!mMapObject->isVisible())
        return;
    const QColor &color = mMapObject->objectGroup()->color();
    mRendererList[mMapObject->objectGroup()]->drawMapObject(p, mMapObject,
                             color.isValid() ? color : Qt::darkGray);
}
