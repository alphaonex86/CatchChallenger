// Copyright 2021 CatchChallenger
#include "RoundedButton.hpp"

#include <QEvent>
#include <QGraphicsWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <iostream>

#include "../core/AssetsLoader.hpp"
#include "../core/EventManager.hpp"

using UI::RoundedButton;

RoundedButton::RoundedButton(Node *parent) : Node(parent) {
  content_cache_ = nullptr;
  pressed_ = false;
  checkable_ = false;
  checked_ = false;
  has_border_ = false;

  start_inactive_ = Qt::white;
  start_active_ = Qt::black;
  end_inactive_ = Qt::white;
  end_active_ = Qt::black;
  end_percent_ = 0;

  bounding_rect_ = QRectF(0.0, 0.0, 200.0, 50.0);
}

void RoundedButton::SetSize(qreal w, qreal h) {
  if (bounding_rect_.width() == w && bounding_rect_.height() == h) return;
  Node::SetSize(w, h);
  ReDraw();
}

void RoundedButton::SetWidth(qreal w) {
  if (bounding_rect_.width() == w) return;
  Node::SetWidth(w);
  ReDraw();
}

void RoundedButton::SetPos(qreal x, qreal y) {
  if (bounding_rect_.x() == x && bounding_rect_.y() == y) return;
  Node::SetPos(x, y);
}

RoundedButton::~RoundedButton() { UnRegisterEvents(); }

void RoundedButton::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;

  qreal slope = bounding_rect_.height() / 2;

  QRadialGradient m_gradient(slope, bounding_rect_.height() / 2,
                             bounding_rect_.height() / 2);
  m_gradient.setColorAt(0.0, Qt::gray);
  m_gradient.setColorAt(1.0, Qt::transparent);

  painter->setRenderHint(QPainter::Antialiasing);
  painter->setBrush(m_gradient);
  painter->setPen(Qt::NoPen);

  painter->drawRect(0, 0, slope, bounding_rect_.height());

  QRadialGradient m_gradient2(bounding_rect_.width() - slope,
                              bounding_rect_.height() / 2,
                              bounding_rect_.height() / 2);
  m_gradient2.setColorAt(0.0, Qt::gray);
  m_gradient2.setColorAt(1.0, Qt::transparent);
  painter->setBrush(m_gradient2);
  painter->drawRect(bounding_rect_.width() - slope, 0, slope,
                    bounding_rect_.height());

  QLinearGradient gradient3(0, 0, 0, bounding_rect_.height());
  gradient3.setColorAt(0.0, Qt::transparent);
  gradient3.setColorAt(0.5, Qt::gray);
  gradient3.setColorAt(1.0, Qt::transparent);
  painter->setBrush(gradient3);
  painter->drawRect(slope, 0, bounding_rect_.width() - slope * 2,
                    bounding_rect_.height());

  if (checked_) {
    painter->setBrush(start_active_);
    if (has_border_) {
      QPen pen(border_color_, border_width_);
      painter->setPen(pen);
    }
  } else {
    painter->setBrush(start_inactive_);
  }
  qreal padding = 5;

  qreal inner_height = bounding_rect_.height() - padding * 2;
  qreal inner_diameter = slope * 2 - padding * 2;
  qreal inner_width = bounding_rect_.width() - slope * 2;

  QPainterPath base_path;
  base_path.moveTo(slope, padding);
  base_path.lineTo(slope + inner_width, padding);
  base_path.cubicTo(bounding_rect_.width(), padding, bounding_rect_.width(),
                    padding + inner_height, slope + inner_width,
                    inner_height + padding);
  base_path.lineTo(slope, inner_height + padding);
  base_path.cubicTo(0, inner_height + padding, 0, padding, slope, padding);

  painter->drawPath(base_path);

  if (end_percent_ > 0) {
    painter->setPen(Qt::NoPen);
    if (checked_) {
      painter->setBrush(end_active_);
    } else {
      painter->setBrush(end_inactive_);
    }

    qreal inner_end = bounding_rect_.width() * end_percent_;

    QPainterPath end_path;
    end_path.moveTo(slope + inner_width - inner_end, padding);
    end_path.lineTo(slope + inner_width, padding);
    end_path.cubicTo(bounding_rect_.width(), padding, bounding_rect_.width(),
                     padding + inner_height, slope + inner_width,
                     inner_height + padding);
    end_path.lineTo(
        slope + inner_width - inner_end - bounding_rect_.height() / 2,
        inner_height + padding);
    end_path.lineTo(slope + inner_width - inner_end, padding);
    painter->drawPath(end_path);
  }

  delete content_cache_;
  content_cache_ = nullptr;

  if (content_cache_ == nullptr) {
    content_cache_ = new QPixmap(Width() - slope * 2, inner_height);
    content_cache_->fill(Qt::transparent);
    QPainter *inner_painter = new QPainter(content_cache_);
    QColor color = checked_ ? Qt::white : Qt::black;
    QRectF boundary(slope, 0, bounding_rect_.width() - (slope * 2), inner_height);
    DrawContent(inner_painter, color, boundary);
    delete inner_painter;
  }
  painter->drawPixmap(slope, padding, *content_cache_);
}

void RoundedButton::SetPressed(const bool &pressed) {
  if (checkable_) {
    if (pressed_ == false && pressed == true && checked_ == false)
      checked_ = true;
    else if (pressed_ == false && pressed == true && checked_ == true)
      checked_ = false;
  } else {
    checked_ = pressed;
  }
  pressed_ = pressed;
  ReDraw();
}

void RoundedButton::MousePressEvent(const QPointF &p, bool &press_validated) {
  if (press_validated) return;
  if (pressed_) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(p)) {
    press_validated = true;
    SetPressed(true);
  }
}

void RoundedButton::MouseReleaseEvent(const QPointF &p, bool &prev_validated) {
  if (prev_validated) {
    SetPressed(false);
    return;
  }
  if (!pressed_) return;
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  SetPressed(false);
  if (!prev_validated && IsVisible()) {
    if (t.contains(p)) {
      if (on_click_) {
        on_click_(this);
      }
    }
  }
}

void RoundedButton::MouseMoveEvent(const QPointF &point) { (void)point; }

void RoundedButton::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void RoundedButton::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void RoundedButton::SetEnabled(bool enabled) {
  if (enabled == IsEnabled()) return;
  Node::SetEnabled(enabled);
  ReDraw();
}

void RoundedButton::SetCheckable(bool checkable) {
  checkable_ = checkable;
  ReDraw();
}

bool RoundedButton::IsChecked() const { return checked_; }

void RoundedButton::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void RoundedButton::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void RoundedButton::ReDrawContent() {
  if (content_cache_) {
    delete content_cache_;
  }
  content_cache_ = nullptr;
}

void RoundedButton::SetStart(QColor inactive, QColor active) {
  start_active_ = active;
  start_inactive_ = inactive;
}

void RoundedButton::SetEnd(qreal percent, QColor inactive, QColor active) {
  end_percent_ = percent;
  end_active_ = active;
  end_inactive_ = inactive;
}

void RoundedButton::SetBorder(qreal width, QColor color) {
  border_width_ = width;
  border_color_ = color;
  has_border_ = true;
}
