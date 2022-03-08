// Copyright 2021 CatchChallenger
#include "MessageBar.hpp"

#include <QPainter>
#include <iostream>

#include "../core/SceneManager.hpp"

using UI::MessageBar;

MessageBar::MessageBar(Node *parent) : Node(parent) {
  node_type_ = __func__;
  on_hide_ = nullptr;
  show_next_ = false;

  font_ = new QFont();
  font_->setFamily("Roboto Condensed");
  font_->setPixelSize(30);

  font_icon_ = new QFont();
  font_icon_->setFamily("icomoon");
  font_icon_->setPixelSize(56);

  auto scene_width = SceneManager::GetInstance()->width();
  auto scene_height = SceneManager::GetInstance()->height();

  SetSize(scene_width, scene_height * 0.2);
  SetPos(0, scene_height - Height() - 30);
  SetZValue(99);
}

MessageBar::~MessageBar() {}

MessageBar *MessageBar::Create(Node *parent) {
  MessageBar *instance = new (std::nothrow) MessageBar(parent);
  return instance;
}

void MessageBar::Draw(QPainter *painter) {
  painter->fillRect(0, 0, Width(), Height(), QColor(51, 51, 51));

  painter->setBrush(QColor(38, 38, 38));
  painter->setPen(QColor(38, 38, 38));

  qreal width_1 = Width() * 0.08;
  qreal width_2 = width_1 * 0.25;

  QPainterPath path_1;
  path_1.moveTo(0, 0);
  path_1.lineTo(width_1, 0);
  path_1.lineTo(width_2, Height());
  path_1.lineTo(0, Height());
  path_1.lineTo(0, 0);
  painter->drawPath(path_1);

  QPainterPath path_2;
  path_2.moveTo(Width(), 0);
  path_2.lineTo(Width() - width_2, 0);
  path_2.lineTo(Width() - width_1, Height());
  path_2.lineTo(Width(), Height());
  path_2.lineTo(Width(), 0);
  painter->drawPath(path_2);

  painter->setPen(Qt::white);
  painter->setFont(*font_);
  painter->drawText(
      QRectF(width_1 + 20, 20, Width() - width_1 * 2, Height() - 40), 0,
      current_message_);

  if (show_next_) {
    painter->setFont(*font_icon_);
    painter->drawText(Width() - 50, Height() - 20, "v");
  }
}

void MessageBar::SetOnHide(const std::function<void()> &on_hide) {
  on_hide_ = on_hide;
}

void MessageBar::Hide() {
  SetVisible(false);
  if (on_hide_) {
    on_hide_();
  }
}

void MessageBar::Show() {
  SetVisible(true);
  UpdateCurrentMessage();
}

void MessageBar::SetMessages(const std::vector<std::string> &messages) {
  messages_ = messages;
}

void MessageBar::SetMessage(const std::string &message) {
  messages_.clear();
  messages_.push_back(message);
}

void MessageBar::UpdateCurrentMessage() {
  if (messages_.empty()) {
    Hide();
    return;
  }

  current_message_ = QString::fromStdString(messages_.front());
  messages_.erase(messages_.begin());
  ReDraw();
}

void MessageBar::MousePressEvent(const QPointF &p, bool &press_validated) {
  if (press_validated) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(p)) {
    press_validated = true;
  }
}

void MessageBar::MouseReleaseEvent(const QPointF &p, bool &prev_validated) {
  if (prev_validated) {
    return;
  }
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(p)) {
      if (show_next_) {
        UpdateCurrentMessage();
      }
    }
  }
}

void MessageBar::MouseMoveEvent(const QPointF &point) { (void)point; }

void MessageBar::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void MessageBar::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void MessageBar::ShowNext(bool next) {
  show_next_ = next;
}
