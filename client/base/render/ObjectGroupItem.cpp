#include "ObjectGroupItem.h"
#include "MapObjectItem.h"

#include <QDebug>

QHash<Tiled::ObjectGroup *,ObjectGroupItem *> ObjectGroupItem::objectGroupLink;

ObjectGroupItem::ObjectGroupItem(Tiled::ObjectGroup *objectGroup,
                QGraphicsItem *parent)
    : QGraphicsItem(parent),
      mObjectGroup(objectGroup)
{
    setFlag(QGraphicsItem::ItemHasNoContents);

    // Create a child item for each object
    QList<QGraphicsItem *> childs=childItems();
    int index=0;
    while(index<childs.size())
    {
        delete childs.at(index);
        index++;
    }
    foreach (Tiled::MapObject *object, mObjectGroup->objects())
        MapObjectItem::objectLink[object]=new MapObjectItem(object, this);
}

QRectF ObjectGroupItem::boundingRect() const
{
    return QRectF();
}

void ObjectGroupItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

void ObjectGroupItem::addObject(Tiled::MapObject *object)
{
    if(!MapObjectItem::objectLink.contains(object))
        qDebug() << "Tiled object already present on the layer";
    MapObjectItem::objectLink[object]=new MapObjectItem(object, this);

    mObjectGroup->addObject(object);
}

void ObjectGroupItem::removeObject(Tiled::MapObject *object)
{
    if(!MapObjectItem::objectLink.contains(object))
        qDebug() << "The tiled object not exist on this layer";
    else
    {
        delete MapObjectItem::objectLink[object];
        MapObjectItem::objectLink.remove(object);
    }

    mObjectGroup->removeObject(object);
}
