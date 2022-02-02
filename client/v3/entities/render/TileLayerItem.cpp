// Copyright 2021 CatchChallenger
#include "TileLayerItem.hpp"

#include <iostream>

TileLayerItem::TileLayerItem(Tiled::TileLayer *tileLayer,
                             Tiled::MapRenderer *renderer, Node *parent)
    : Node(parent), mTileLayer(tileLayer), mRenderer(renderer) {
  // setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
  node_type_ = "TileLayerItem";
}

TileLayerItem *TileLayerItem::Create(Tiled::TileLayer *tileLayer,
                                     Tiled::MapRenderer *renderer,
                                     Node *parent) {
  return new (std::nothrow) TileLayerItem(tileLayer, renderer, parent);
}

void TileLayerItem::Draw(QPainter *painter) {
  // TODO(lanstat): verify how works option
  // mRenderer->drawTileLayer(painter, mTileLayer, option->rect);
  std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< "123" << std::endl;
  mRenderer->drawTileLayer(painter, mTileLayer, BoundingRect());
}

QRectF TileLayerItem::BoundingRect() const {
  return mRenderer->boundingRect(mTileLayer->bounds());
}
