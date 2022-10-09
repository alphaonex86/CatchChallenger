// Copyright 2021 CatchChallenger
#include "Checkbox.hpp"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <iostream>

#include "../Constants.hpp"
#include "../core/AssetsLoader.hpp"
#include "../core/EventManager.hpp"
#include "../core/Sprite.hpp"

using UI::Checkbox;

Checkbox::Checkbox(Node *parent) : Node(parent) {
  checked_ = false;
  SetSize(kMedium);
}

Checkbox *Checkbox::Create(Node *parent) {
  Checkbox *instance = new (std::nothrow) Checkbox(parent);
  return instance;
}

Checkbox::~Checkbox() {}

void Checkbox::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;

  auto background = Sprite::Create(":/CC/images/interface/input.png");

  background->Strech(14, 11, 10, Width(), Height());
  background->Render(painter);

  delete background;
  if (checked_) {
    auto check = AssetsLoader::GetInstance()
                     ->GetImage(":/CC/images/interface/check.png")
                     ->scaledToHeight(Height() * 0.7, Qt::SmoothTransformation);
    painter->drawPixmap((Width() / 2) - (check.width() / 2),
                        (Height() / 2) - (check.height() / 2), check);
  }
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
      checked_ = !checked_;
      if (on_click_) {
        on_click_(this);
      }
      ReDraw();
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

void Checkbox::SetSize(const CheckSize &size) {
  QSizeF buttonSize;

  switch (size) {
    case kLarge:
      buttonSize = Constants::ButtonLargeSize();
      break;
    case kMedium:
      buttonSize = Constants::ButtonMediumSize();
      break;
    case kSmall:
      buttonSize = Constants::ButtonSmallSize();
      break;
  }

  Node::SetSize(buttonSize.height(), buttonSize.height());
}
