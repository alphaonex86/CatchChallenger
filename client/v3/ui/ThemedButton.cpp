// Copyright 2021 CatchChallenger
#include "ThemedButton.hpp"

#include <QPainter>
#include <iostream>

using UI::GreenButton;
using UI::YellowButton;

QPixmap Strech(QPixmap pixmap, unsigned int padding_x, unsigned int padding_y,
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

GreenButton::GreenButton(Node *parent)
    : Button(":/CC/images/interface/greenbutton.png", parent) {
  SetOutlineColor(QColor(44, 117, 0));
}

GreenButton *GreenButton::Create(Node *parent) {
  return new (std::nothrow) GreenButton(parent);
}

QPixmap GreenButton::ScaleBackground(QPixmap pixmap, qreal width,
                                     qreal height) {
  return Strech(pixmap, 25, 25, 25, width, height);
}

YellowButton::YellowButton(Node *parent)
    : Button(":/CC/images/interface/button.png", parent) {}

YellowButton *YellowButton::Create(Node *parent) {
  return new (std::nothrow) YellowButton(parent);
}

QPixmap YellowButton::ScaleBackground(QPixmap pixmap, qreal width,
                                     qreal height) {
  return Strech(pixmap, 25, 25, 25, width, height);
}
