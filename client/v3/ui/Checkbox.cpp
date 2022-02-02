// Copyright 2021 CatchChallenger
#include "Checkbox.hpp"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <iostream>

#include "../core/AssetsLoader.hpp"
#include "../core/EventManager.hpp"

using UI::Checkbox;

Checkbox::Checkbox(Node *parent) : Node(parent) {
  checked_ = false;
}

Checkbox *Checkbox::Create(Node *parent) {
  Checkbox *instance = new (std::nothrow) Checkbox(parent);
  return instance;
}

Checkbox::~Checkbox() {}

void Checkbox::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;
}

void Checkbox::MousePressEvent(const QPointF &p, bool &press_validated) {
  if (press_validated) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(p)) {
    press_validated = true;
  }
}

void Checkbox::MouseReleaseEvent(const QPointF &p, bool &prev_validated) {
  if (prev_validated) {
    return;
  }
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(p)) {
      if (on_click_) {
        on_click_(this);
      }
    }
  }
}

void Checkbox::MouseMoveEvent(const QPointF &point) { (void)point; }

void Checkbox::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Checkbox::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Checkbox::SetChecked(bool checked) {
  checked_ = checked;
  ReDraw();
}

bool Checkbox::IsChecked() const { return checked_; }

void Checkbox::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Checkbox::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}
