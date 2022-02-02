#include "ObjectGroupItem.hpp"

#include <QtCore/qmath.h>

#include <QDebug>
#include <iostream>

#include "MapObjectItem.hpp"

std::unordered_map<Tiled::ObjectGroup *, ObjectGroupItem *>
    ObjectGroupItem::objectGroupLink;

ObjectGroupItem::ObjectGroupItem(Tiled::ObjectGroup *objectGroup, Node *parent)
    : Node(parent), mObjectGroup(objectGroup) {
  node_type_ = "ObjectGroupItem";
  // setFlag(QGraphicsItem::ItemHasNoContents);

  // Create a child item for each object
  QList<Tiled::MapObject *> objects = mObjectGroup->objects();
  int index = 0;
  while (index < objects.size()) {
    MapObjectItem *mapObjectItem =
        MapObjectItem::Create(objects.at(index), this);
    mapObjectItem->SetZValue(qCeil(objects.at(index)->y()));
    MapObjectItem::objectLink[objects.at(index)] = mapObjectItem;
    index++;
  }
}

ObjectGroupItem *ObjectGroupItem::Create(Tiled::ObjectGroup *objectGroup,
                                         Node *parent) {
  return new (std::nothrow) ObjectGroupItem(objectGroup, parent);
}

ObjectGroupItem::~ObjectGroupItem() {
  {
    QList<Tiled::MapObject *> objects = mObjectGroup->objects();
    int index = 0;
    while (index < objects.size()) {
      if (MapObjectItem::objectLink.find(objects.at(index)) ==
          MapObjectItem::objectLink.cend())
        qDebug() << "The tiled object not exist on this layer (destructor)";
      else
        MapObjectItem::objectLink.erase(objects.at(index));
      index++;
    }
  }
}

void ObjectGroupItem::Draw(QPainter *) {}

bool ObjectGroupItem::isVisible() const { return mObjectGroup->isVisible(); }

void ObjectGroupItem::addObject(Tiled::MapObject *object) {
  if (MapObjectItem::objectLink.find(object) !=
      MapObjectItem::objectLink.cend()) {
    qDebug() << "Tiled object already present on the layer:" << object;
    return;
  }
  MapObjectItem::objectLink[object] = MapObjectItem::Create(object, this);

  mObjectGroup->addObject(object);
}

void ObjectGroupItem::removeObject(Tiled::MapObject *object) {
  if (MapObjectItem::objectLink.find(object) ==
      MapObjectItem::objectLink.cend()) {
    qDebug() << "The tiled object not exist on this layer";
#ifndef CATCHCHALLENGER_EXTRA_CHECK
    return;
#endif
  } else {
    delete MapObjectItem::objectLink[object];
    MapObjectItem::objectLink.erase(object);
  }
  mObjectGroup->removeObject(object);
}
