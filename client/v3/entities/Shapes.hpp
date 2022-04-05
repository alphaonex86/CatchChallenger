// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_SHAPES_HPP_
#define CLIENT_V3_ENTITIES_SHAPES_HPP_

#include "../core/Scene.hpp"

class Shapes {
 public:
  static QPainterPath DrawDiamond(const QRectF &rect, qreal displacement);
};
#endif  // CLIENT_V3_ENTITIES_SHAPES_HPP_
