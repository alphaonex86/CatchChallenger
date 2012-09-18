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
    int loopSize=childs.size();
    while(index<childs.size())
    {
        delete childs.at(index);
        index++;
    }
    QList<Tiled::MapObject *> objects=mObjectGroup->objects();
    loopSize=objects.size();
    index=0;
    while(index<loopSize)
    {
        MapObjectItem::objectLink[objects.at(index)]=new MapObjectItem(objects.at(index), this);
        index++;
    }
}

QRectF ObjectGroupItem::boundingRect() const
{
    return QRectF();
}

void ObjectGroupItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

bool ObjectGroupItem::isVisible() const
{
    return mObjectGroup->isVisible();
}

void ObjectGroupItem::addObject(Tiled::MapObject *object)
{
    if(MapObjectItem::objectLink.contains(object))
    {
        qDebug() << "Tiled object already present on the layer";
        return;
    }
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
