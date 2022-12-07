// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_PIXMAPUTILS_HPP_
#define CLIENT_V3_ENTITIES_PIXMAPUTILS_HPP_

#include "../core/Scene.hpp"
#include "CommonTypes.hpp"

class PixmapUtils {
 public:
  static QPixmap B64Png2Pixmap(const QString& b64);
  static QImage CropToContent(const QImage& buffer, QColor stop);
  static QPixmap Strech(QPixmap pixmap, unsigned int padding_x,
                        unsigned int padding_y, unsigned int border_size,
                        qreal width, qreal height);
};
#endif  // CLIENT_V3_ENTITIES_PIXMAPUTILS_HPP_
