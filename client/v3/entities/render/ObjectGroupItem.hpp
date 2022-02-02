#ifndef OBJECTGROUPITEM_H
#define OBJECTGROUPITEM_H

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QHash>
#include <QKeyEvent>
#include <QMultiMap>
#include <QPainter>
#include <QRectF>
#include <QSet>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QWidget>
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

class ObjectGroupItem : public Node {
 public:
  static ObjectGroupItem* Create(Tiled::ObjectGroup *objectGroup, Node *parent = nullptr);
  virtual ~ObjectGroupItem();
  void Draw(QPainter *) override;
  bool isVisible() const;
  void addObject(Tiled::MapObject *object);
  void removeObject(Tiled::MapObject *object);

 public:
  Tiled::ObjectGroup *mObjectGroup;

  // the link tiled with viewer
  static std::unordered_map<Tiled::ObjectGroup *, ObjectGroupItem *>
      objectGroupLink;

 private:
  ObjectGroupItem(Tiled::ObjectGroup *objectGroup, Node *parent = nullptr);
};

#endif
