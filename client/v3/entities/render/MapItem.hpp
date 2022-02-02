// Copyright 2021 CatchChallenger
#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QHash>
#include <QKeyEvent>
#include <QMultiMap>
#include <QObject>
#include <QPainter>
#include <QPair>
#include <QRectF>
#include <QSet>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QWidget>
#include <unordered_map>
#include <unordered_set>

#include "../../core/Node.hpp"
#include "../../libraries/tiled/tiled_isometricrenderer.hpp"
#include "../../libraries/tiled/tiled_map.hpp"
#include "../../libraries/tiled/tiled_mapobject.hpp"
#include "../../libraries/tiled/tiled_mapreader.hpp"
#include "../../libraries/tiled/tiled_objectgroup.hpp"
#include "../../libraries/tiled/tiled_orthogonalrenderer.hpp"
#include "../../libraries/tiled/tiled_tilelayer.hpp"
#include "../../libraries/tiled/tiled_tileset.hpp"
#include "MapObjectItem.hpp"
#include "MapVisualiserThread.hpp"
#include "ObjectGroupItem.hpp"
#include "TileLayerItem.hpp"

class MapItem : public QObject, public Node {
  Q_OBJECT
 public:
  static MapItem *Create(Node *parent = nullptr);
  void addMap(Map_full *tempMapObject, Tiled::Map *map,
              Tiled::MapRenderer *renderer, const int &playerLayerIndex);
  bool haveMap(Tiled::Map *map);
  void removeMap(Tiled::Map *map);
  void setMapPosition(Tiled::Map *map, int16_t x, int16_t y);
  void Draw(QPainter *) override;

 private:
  std::unordered_map<Tiled::Map *, std::unordered_set<Node *> > displayed_layer;

  MapItem(Node *parent = nullptr);
 signals:
  void eventOnMap(CatchChallenger::MapEvent event, Map_full *tempMapObject,
                  uint8_t x, uint8_t y);
};

#endif
