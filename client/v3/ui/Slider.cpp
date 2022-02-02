// Copyright 2021 CatchChallenger
#include "Slider.hpp"

#include <iostream>

#include "../core/EventManager.hpp"

using UI::Slider;

Slider::Slider(Node *parent) : Node(parent) {
  on_value_change_ = nullptr;
  slider_ = Sprite::Create(this);
  slider_->SetPixmap(":/CC/images/interface/sliderH.png");
}

Slider::~Slider() {}

Slider *Slider::Create(Node *parent) {
  return new (std::nothrow) Slider(parent);
}

void Slider::SetValue(qreal value) {
  value_ = value;
  ReCalculateSliderPosition();
}

void Slider::SetMaximum(qreal value) {
  maximum_ = value;
  ReCalculateSliderPosition();
}

void Slider::MousePressEvent(const QPointF &point, bool &press_validated) {
  if (press_validated) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void Slider::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  if (prev_validated) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      qreal percent = (point.x() - X()) / (Width() - slider_->Width());
      value_ = percent * maximum_;
      if (on_value_change_) {
        on_value_change_(value_);
      }
      
      ReCalculateSliderPosition();
    }
  }
}

void Slider::MouseMoveEvent(const QPointF &point) {}

void Slider::Draw(QPainter *painter) {
  auto sprite = Sprite::Create(":/CC/images/interface/sliderBarH-trimmed.png");
  sprite->Strech(7, 5, 7, Width(), 15);
  sprite->SetY(Height() / 2 - sprite->Height() / 2);
  sprite->Render(painter);
  delete sprite;
}

void Slider::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Slider::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void Slider::SetOnValueChange(std::function<void(qreal)> callback) {
  on_value_change_ = callback;
}

qreal Slider::Value() { return value_; }

void Slider::OnResize() {
  ReCalculateSliderPosition();
  ReDraw();
}

void Slider::ReCalculateSliderPosition() {
  slider_->SetY(Height() / 2 - slider_->Height() / 2);
  qreal width = Width() - slider_->Width();
  qreal tmp = (value_ / maximum_) * width;
  slider_->SetX(tmp);
}
