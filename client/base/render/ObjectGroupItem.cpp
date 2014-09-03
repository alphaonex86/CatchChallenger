#include "ObjectGroupItem.h"
#include "MapObjectItem.h"

#include <QDebug>
#include <QtCore/qmath.h>

QHash<Tiled::ObjectGroup *,ObjectGroupItem *> ObjectGroupItem::objectGroupLink;

ObjectGroupItem::ObjectGroupItem(Tiled::ObjectGroup *objectGroup,
                QGraphicsItem *parent)
    : QGraphicsItem(parent),
      mObjectGroup(objectGroup)
{
    setFlag(QGraphicsItem::ItemHasNoContents);

    // Create a child item for each object
    QList<Tiled::MapObject *> objects=mObjectGroup->objects();
    const int &loopSize=objects.size();
    int index=0;
    while(index<loopSize)
    {
        MapObjectItem *mapObjectItem=new MapObjectItem(objects.at(index), this);
        mapObjectItem->setZValue(qCeil(objects.at(index)->y()));
        MapObjectItem::objectLink[objects.at(index)]=mapObjectItem;
        index++;
    }
}

ObjectGroupItem::~ObjectGroupItem()
{
    {
        QList<Tiled::MapObject *> objects=mObjectGroup->objects();
        const int &loopSize=objects.size();
        int index=0;
        while(index<loopSize)
        {
            if(!MapObjectItem::objectLink.contains(objects.at(index)))
                qDebug() << "The tiled object not exist on this layer (destructor)";
            else
                MapObjectItem::objectLink.remove(objects.at(index));
            index++;
        }
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
        qDebug() << "Tiled object already present on the layer:" << object;
        return;
    }
    MapObjectItem::objectLink[object]=new MapObjectItem(object, this);

    mObjectGroup->addObject(object);
}

void ObjectGroupItem::removeObject(Tiled::MapObject *object)
{
    if(!MapObjectItem::objectLink.contains(object))
    {
        qDebug() << "The tiled object not exist on this layer";
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        return;
        #endif
    }
    else
    {
        delete MapObjectItem::objectLink[object];
        MapObjectItem::objectLink.remove(object);
    }
    mObjectGroup->removeObject(object);
}
