// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_RENDER_TILELAYERITEM_HPP_
#define CLIENT_V3_ENTITIES_RENDER_TILELAYERITEM_HPP_

#include <QPainter>

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
 * Item that represents a tile layer.
 */
class TileLayerItem : public Node {
 public:
  static TileLayerItem *Create(Tiled::TileLayer *tileLayer,
                               Tiled::MapRenderer *renderer,
                               Node *parent = nullptr);
  void Draw(QPainter *painter) override;
  QRectF BoundingRect() const override;

 private:
  Tiled::TileLayer *mTileLayer;
  Tiled::MapRenderer *mRenderer;

  TileLayerItem(Tiled::TileLayer *tileLayer, Tiled::MapRenderer *renderer,
                Node *parent = nullptr);
};

#endif  // CLIENT_V3_ENTITIES_RENDER_TILELAYERITEM_HPP_
