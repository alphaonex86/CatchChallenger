// Copyright 2021 <CatchChallenger>
#include "SelectableItem.hpp"

#include <QPainter>
#include <iostream>

#include "../core/EventManager.hpp"
#include "../core/Sprite.hpp"

using UI::SelectableItem;

SelectableItem::SelectableItem() : Node() {
  content_cache_ = nullptr;
  bg_unselected_ = QString(":/CC/images/interface/b1-trimmed.png");
  bg_selected_ = QString(":/CC/images/interface/b1-green.png");
  bg_disabled_ = QString(":/CC/images/interface/b1-gray.png");
  bg_danger_ = QString(":/CC/images/interface/b1-red.png");

  strech_x_ = 20;
  strech_y_ = 18;
  border_size_ = 10;
  is_selected_ = false;
  is_disabled_ = false;
  is_danger_ = false;
}

SelectableItem::~SelectableItem() {
  UnRegisterEvents();
  if (content_cache_) {
    delete content_cache_;
  }
  content_cache_ = nullptr;
}

void SelectableItem::Draw(QPainter *painter) {
  if (!bg_selected_.isEmpty()) {
    Sprite *background;
    if (is_disabled_) {
      background = Sprite::Create(bg_disabled_);
    } else {
      if (is_selected_) {
        background = Sprite::Create(bg_selected_);
      } else {
        if (is_danger_) {
          background = Sprite::Create(bg_danger_);
        } else {
          background = Sprite::Create(bg_unselected_);
        }
      }
    }
    background->Strech(strech_x_, strech_y_, border_size_, Width(), Height());
    background->Render(painter);

    delete background;
  }

  if (content_cache_ == nullptr) {
    content_cache_ = new QPixmap(Width(), Height());
    content_cache_->fill(Qt::transparent);
    QPainter *inner_painter = new QPainter(content_cache_);
    DrawContent(inner_painter);
    delete inner_painter;
  }
  painter->drawPixmap(0, 0, *content_cache_);
}

void SelectableItem::ReDrawContent() {
  if (content_cache_) {
    delete content_cache_;
  }
  content_cache_ = nullptr;
}

void SelectableItem::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void SelectableItem::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void SelectableItem::MousePressEvent(const QPointF &point,
                                     bool &press_validated) {
  if (press_validated) {
    if (is_selected_) {
      is_selected_ = false;
      ReDraw();
    }
    return;
  }
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void SelectableItem::MouseReleaseEvent(const QPointF &point,
                                       bool &prev_validated) {
  if (prev_validated) {
    if (is_selected_) {
      is_selected_ = false;
      ReDraw();
    }
    return;
  }

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point) && (!is_disabled_ && !is_danger_)) {
      is_selected_ = true;
      prev_validated = true;
      ReDraw();
      if (on_click_) {
        on_click_(this);
      }
    }
  }
}

void SelectableItem::SetSelected(bool is_selected) {
  is_selected_ = is_selected;
  ReDraw();
}

void SelectableItem::SetDisabled(bool is_disabled) {
  is_disabled_ = is_disabled;
  ReDraw();
}

void SelectableItem::SetDanger(bool is_danger) {
  is_danger_ = is_danger;
  ReDraw();
}
