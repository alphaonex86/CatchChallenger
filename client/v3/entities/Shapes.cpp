// Copyright 2021 CatchChallenger
#include "Shapes.hpp"

#include <iostream>

QPainterPath Shapes::DrawDiamond(const QRectF &rect, qreal displacement) {
  QPainterPath base_path;
  base_path.moveTo(displacement, rect.y());
  base_path.lineTo(rect.width(), rect.y());
  base_path.lineTo(rect.width() - displacement, rect.height());
  base_path.lineTo(rect.x(), rect.height());
  base_path.lineTo(displacement, rect.y());
  return base_path;
}
