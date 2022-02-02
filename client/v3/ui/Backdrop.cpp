// Copyright 2021 CatchChallenger
#include "Backdrop.hpp"

#include <iostream>
#include <QPainter>

using UI::Backdrop;

Backdrop::Backdrop(QColor color, const std::function<void(QPainter *)> &on_draw, Node *parent) : Node(parent) {
  node_type_ = __func__;
  color_ = color;
  on_draw_ = on_draw;
}

Backdrop::~Backdrop() {}

Backdrop *Backdrop::Create(QColor color, Node *parent) {
  Backdrop *instance = new (std::nothrow) Backdrop(color, nullptr, parent);
  return instance;
}

Backdrop *Backdrop::Create(const std::function<void(QPainter *)> &on_draw, Node *parent) {
  Backdrop *instance = new (std::nothrow) Backdrop(Qt::transparent, on_draw, parent);
  return instance;
}

void Backdrop::Draw(QPainter *painter) {
  painter->fillRect(0, 0, bounding_rect_.width(), bounding_rect_.height(),
                    color_);
  if (on_draw_) on_draw_(painter);
}

void Backdrop::SetOnDraw(const std::function<void (QPainter *)> &on_draw) {
  on_draw_ = on_draw;
}
