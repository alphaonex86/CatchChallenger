// Copyright 2021 CatchChallenger
#include "ThemedButton.hpp"

#include <QPainter>
#include <iostream>
#include "../entities/PixmapUtils.hpp"

using UI::GreenButton;
using UI::YellowButton;
using UI::RedButton;

GreenButton::GreenButton(Node *parent)
    : Button(":/CC/images/interface/greenbutton.png", parent) {
  SetOutlineColor(QColor(44, 117, 0));
}

GreenButton *GreenButton::Create(Node *parent) {
  return new (std::nothrow) GreenButton(parent);
}

QPixmap GreenButton::ScaleBackground(QPixmap pixmap, qreal width,
                                     qreal height) {
  return PixmapUtils::Strech(pixmap, 25, 25, 25, width, height);
}

YellowButton::YellowButton(Node *parent)
    : Button(":/CC/images/interface/button.png", parent) {}

YellowButton *YellowButton::Create(Node *parent) {
  return new (std::nothrow) YellowButton(parent);
}

QPixmap YellowButton::ScaleBackground(QPixmap pixmap, qreal width,
                                     qreal height) {
  return PixmapUtils::Strech(pixmap, 25, 25, 25, width, height);
}

RedButton::RedButton(Node *parent)
    : Button(":/CC/images/interface/redbutton.png", parent) {}

RedButton *RedButton::Create(Node *parent) {
  return new (std::nothrow) RedButton(parent);
}

QPixmap RedButton::ScaleBackground(QPixmap pixmap, qreal width,
                                     qreal height) {
  return PixmapUtils::Strech(pixmap, 25, 25, 25, width, height);
}
