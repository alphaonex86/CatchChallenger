// Copyright 2022 CatchChallenger
#include "PixmapUtils.hpp"

#include <QPainter>
#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../Constants.hpp"

QPixmap PixmapUtils::B64Png2Pixmap(const QString &b64) {
  QByteArray by = QByteArray::fromBase64(b64.toUtf8());
  QImage image = QImage::fromData(by, "PNG");
  return QPixmap::fromImage(image);
}

QImage PixmapUtils::CropToContent(const QImage &buffer, QColor stop) {
  int pivot = 0;
  int end = buffer.height() - 1;
  QColor aux;
  do {
    pivot++;
    aux = buffer.pixel(10, pivot);
  } while (aux != stop && pivot < end);
  return buffer.copy(0, 0, buffer.width(), pivot);
}

QPixmap PixmapUtils::Strech(QPixmap pixmap, unsigned int padding_x, unsigned int padding_y,
               unsigned int border_size, qreal width, qreal height) {
  QPixmap background = pixmap.copy();
  QPixmap tr, tm, tl, mr, mm, ml, br, bm, bl;
  tl = background.copy(0, 0, padding_x, padding_y);
  tm = background.copy(padding_x, 0, background.width() - padding_x * 2,
                       padding_y);
  tr = background.copy(background.width() - padding_x, 0, padding_x, padding_y);
  ml = background.copy(0, padding_y, padding_x,
                       background.height() - padding_y * 2);
  mm = background.copy(padding_x, padding_y, background.width() - padding_x * 2,
                       background.height() - padding_y * 2);
  mr = background.copy(background.width() - padding_x, padding_y, padding_x,
                       background.height() - padding_y * 2);
  bl =
      background.copy(0, background.height() - padding_y, padding_x, padding_y);
  bm = background.copy(padding_x, background.height() - padding_y,
                       background.width() - padding_x * 2, padding_y);
  br = background.copy(background.width() - padding_x,
                       background.height() - padding_y, padding_x, padding_y);

  int scaled_x = 0;
  int scaled_y = 0;

  if (padding_x > padding_y) {
    scaled_x = (padding_x / padding_y) * border_size;
    scaled_y = border_size;
  } else {
    scaled_y = (padding_y / padding_x) * border_size;
    scaled_x = border_size;
  }

  tl = tl.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  tm = tm.scaled(width - scaled_x * 2, scaled_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  tr = tr.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  ml = ml.scaled(scaled_x, height - scaled_y * 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  mm = mm.scaled(width - scaled_x * 2, height - scaled_y * 2,
                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  mr = mr.scaled(scaled_x, height - scaled_y * 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  bl = bl.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  bm = bm.scaled(width - scaled_x * 2, scaled_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  br = br.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

  auto tmp = QPixmap(width, height);
  tmp.fill(Qt::transparent);

  auto painter = new QPainter(&tmp);
  painter->drawPixmap(0, 0, tl);
  painter->drawPixmap(scaled_x, 0, tm);
  painter->drawPixmap(width - scaled_x, 0, tr);

  painter->drawPixmap(0, scaled_y, ml);
  painter->drawPixmap(scaled_x, scaled_y, mm);
  painter->drawPixmap(width - scaled_x, scaled_y, mr);

  painter->drawPixmap(0, height - scaled_y, bl);
  painter->drawPixmap(scaled_x, height - scaled_y, bm);
  painter->drawPixmap(width - scaled_x, height - scaled_y, br);
  painter->end();

  delete painter;
  return tmp;
}
