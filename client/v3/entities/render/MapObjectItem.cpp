#include "MapObjectItem.hpp"

#include <iostream>

#include "../../core/SceneManager.hpp"

std::unordered_map<Tiled::ObjectGroup *, Tiled::MapRenderer *>
    MapObjectItem::mRendererList;
std::unordered_map<Tiled::MapObject *, MapObjectItem *>
    MapObjectItem::objectLink;
const void *MapObjectItem::playerObject = nullptr;
qreal MapObjectItem::scale = 1.0;
qreal MapObjectItem::x = 0.0;
qreal MapObjectItem::y = 0.0;

MapObjectItem::MapObjectItem(Tiled::MapObject *mapObject, Node *parent)
    : Node(parent), mMapObject(mapObject) {
  node_type_ = "MapObjectItem";
  // setCacheMode(QGraphicsItem::ItemCoordinateCache);//just change direction
  // without move do bug
  // setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

MapObjectItem *MapObjectItem::Create(Tiled::MapObject *mapObject,
                                     Node *parent) {
  return new (std::nothrow) MapObjectItem(mapObject, parent);
}

void MapObjectItem::Draw(QPainter *p) {
  /// \warning performance problem here
  if (!mMapObject->isVisible()) return;
  // if anymore into a group
  if (mMapObject->objectGroup() == NULL) return;
  qreal x = (mMapObject->x() * 16 + this->x) * this->scale;  // convert to pixel
  qreal y = ((mMapObject->y() - 1) * 16 + this->y) * this->scale;
  qreal w = 16 * this->scale;
  qreal h = 32 * this->scale;
  QRectF objectSpace(x, y, w, h);

  auto scene = SceneManager::GetInstance();

  // only draw in visible space
  if (objectSpace.x() <= scene->width() &&
      objectSpace.y() <= scene->height() &&
      (objectSpace.x() + objectSpace.width()) >= 0.0 &&
      (objectSpace.y() + objectSpace.height()) >= 0.0) {
    Tiled::MapRenderer *r = mRendererList.at(mMapObject->objectGroup());
    r->drawMapObject(p, mMapObject, Qt::transparent);
  }
}

const Tiled::MapObject *MapObjectItem::getMapObject() const {
  return mMapObject;
}
