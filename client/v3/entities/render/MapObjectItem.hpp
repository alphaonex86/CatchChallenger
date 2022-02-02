#ifndef MAPOBJECTITEM_H
#define MAPOBJECTITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRectF>
#include <unordered_map>

#include "../../core/Node.hpp"
#include "../../libraries/tiled/tiled_isometricrenderer.hpp"
#include "../../libraries/tiled/tiled_map.hpp"
#include "../../libraries/tiled/tiled_mapobject.hpp"
#include "../../libraries/tiled/tiled_mapreader.hpp"
#include "../../libraries/tiled/tiled_objectgroup.hpp"
#include "../../libraries/tiled/tiled_orthogonalrenderer.hpp"
#include "../../libraries/tiled/tiled_tilelayer.hpp"
#include "../../libraries/tiled/tiled_tileset.hpp"

/**
 * Item that represents a map object.
 */
class MapObjectItem : public Node {
 public:
  static MapObjectItem *Create(Tiled::MapObject *mapObject,
                               Node *parent = nullptr);
  // QRectF BoundingRect() const override;
  void Draw(QPainter *p) override;
  const Tiled::MapObject *getMapObject() const;

  static const void *playerObject;
  static qreal scale;
  static qreal x;
  static qreal y;

 private:
  Tiled::MapObject *mMapObject;
  QPixmap cache;

  MapObjectItem(Tiled::MapObject *mapObject, Node *parent = nullptr);

 public:
  // interresting if have lot of object by group layer
  static std::unordered_map<Tiled::ObjectGroup *, Tiled::MapRenderer *>
      mRendererList;

  // the link tiled with viewer
  static std::unordered_map<Tiled::MapObject *, MapObjectItem *> objectLink;
};

#endif
