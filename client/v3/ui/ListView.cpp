// Copyright 2021 CatchChallenger
#include "ListView.hpp"

#include <QPainter>
#include <iostream>

#include "../core/EventManager.hpp"
#include "../core/Node.hpp"

using std::placeholders::_1;
using UI::ListView;

ListView::ListView(Node *parent) : Node(parent) {
  node_type_ = __func__;
  content_rect_ = QRectF(0.0, 0.0, 0.0, 0.0);
  slider_rect_ = QRectF(0.0, 0.0, 0.0, 0.0);
  show_slider_ = false;
  is_slider_pressed_ = false;
  is_horizontal_ = false;
  is_drawable_ = false;
  is_processing_resize_ = false;
  spacing_ = 0;
  ShouldCache(false);
}

ListView::~ListView() {
  UnRegisterEvents();
  auto items = items_;
  for (auto item : items) {
    delete item;
  }
  items_.clear();
}

ListView *ListView::Create(Node *parent) {
  ListView *instance = new (std::nothrow) ListView(parent);
  return instance;
}

void ListView::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;
  if (items_.size() == 0 || !is_drawable_ || content_.width() == 0) return;

  int x1 = 0;
  int y1 = 0;
  int x2 = x1 + bounding_rect_.width();
  int y2 = y1 + bounding_rect_.height();

  content_.fill(Qt::transparent);
  QPainter painter_c(&content_);
  if (painter_c.isActive()) {
    for (auto item : items_) {
      item->Render(&painter_c);
    }
    painter_c.end();
  }

  painter->drawPixmap(x1, y1, content_, content_rect_.x(), content_rect_.y(),
                      BoundingRect().width(), BoundingRect().height());

  if (show_slider_) {
    if (is_horizontal_) {
      painter->drawPixmap(x1, y2 - 22, slider_rail_);
    } else {
      painter->drawPixmap(x2 - 22, y1, slider_rail_);
    }
    painter->drawPixmap(slider_rect_.x(), slider_rect_.y(), slider_);
  }
}

void ListView::AddItem(Node *node) {
  node->SetParent(nullptr);
  node->SetOnResize(std::bind(&ListView::OnChildResize, this, _1));
  items_.push_back(node);
  ReCalculateSize();
  ReDraw();
}

void ListView::AddItems(std::vector<Node *> nodes) {
  for (auto node : nodes) {
    node->SetParent(nullptr);
    node->SetOnResize(std::bind(&ListView::OnChildResize, this, _1));
    items_.push_back(node);
  }
  ReCalculateSize();
  ReDraw();
}

Node *ListView::GetItem(uint8_t index) { return items_.at(index); }

void ListView::RemoveItem(uint8_t index) {
  items_.erase(items_.begin() + index);
  ReDraw();
}

void ListView::RemoveItem(Node *node) {
  uint8_t index = 0;
  for (auto item : items_) {
    if (item == node) {
      RemoveItem(index);
      break;
    }
    index++;
  }
}

Node *ListView::GetSelectedItem() { return selected_item_; }

std::vector<Node *> ListView::Items() { return items_; }

void ListView::SetSize(int width, int height) {
  Node::SetSize(width, height);
  if (width > 0 && height > 0) {
    is_drawable_ = true;
  }
  ReCalculateSize();
  ReDraw();
}

void ListView::ReCalculateSize() {
  if (!is_drawable_) return;
  if (!is_horizontal_) {
    content_rect_ = QRectF(0.0, 0.0, Width() - 22, 0.0);

    if (items_.size() == 0) return;
    int x = 0;
    int y = 0;
    for (auto item : items_) {
      item->SetX(x);
      item->SetY(y);
      content_rect_.setHeight(item->Height() + y);
      y = y + item->Height() + spacing_;
    }

    show_slider_ = content_rect_.height() > Height();

    if (!show_slider_) {
      content_rect_.setWidth(Width());
    }

    for (auto item : items_) {
      item->SetWidth(content_rect_.width());
    }

    content_ = QPixmap(content_rect_.width(), content_rect_.height());
    content_scale_ = content_rect_.height() / BoundingRect().height();

    if (show_slider_) {
      int slider_height = (BoundingRect().height() / content_rect_.height()) *
                          BoundingRect().height();

      auto tmp = QPixmap(":/CC/images/interface/sliderBarV.png");
      auto top = tmp.copy(0, 0, 22, 5);
      auto bottom = tmp.copy(0, 38, 22, 5);
      auto middle = tmp.copy(0, 5, 22, 5);
      middle = middle.scaled(22, Height() - 10);

      slider_rail_ = QPixmap(22, Height());
      slider_rail_.fill(Qt::transparent);

      auto painter = new QPainter(&slider_rail_);
      painter->drawPixmap(0, 0, top);
      painter->drawPixmap(0, 5, middle);
      painter->drawPixmap(0, Height() - 5, bottom);
      painter->end();
      delete painter;

      slider_rect_ = QRectF(BoundingRect().width() - 22, 0, 22, slider_height);

      slider_ = QPixmap(22, slider_height);
      slider_.fill(Qt::transparent);

      painter = new QPainter(&slider_);

      tmp = QPixmap(":/CC/images/interface/sliderV.png");
      top = tmp.copy(0, 0, 22, 18);
      bottom = tmp.copy(0, 19, 22, 18);
      middle = tmp.copy(0, 18, 22, 1);

      if (slider_height <= 36) {
        painter->drawPixmap(0, 0, top.scaled(22, slider_height / 2));
        painter->drawPixmap(0, slider_height / 2,
                            bottom.scaled(22, slider_height / 2));
      } else {
        painter->drawPixmap(0, 0, top);
        painter->drawPixmap(0, 18, middle.scaled(22, slider_height - 36));
        painter->drawPixmap(0, slider_height - 18, bottom);
      }
      painter->end();
      delete painter;
    }
  } else {
    content_rect_ = QRectF(0.0, 0.0, 0.0, Height() - 22);

    if (items_.size() == 0) return;
    int x = 0;
    int y = 0;
    for (auto item : items_) {
      item->SetX(x);
      item->SetY(y);
      content_rect_.setWidth(item->Width() + x);
      x = x + item->Width();
    }

    show_slider_ = content_rect_.width() > Width();

    if (!show_slider_) {
      content_rect_.setWidth(Width());
    }

    content_ = QPixmap(content_rect_.width(), content_rect_.height());
    content_scale_ = content_rect_.width() / BoundingRect().width();

    if (show_slider_) {
      int slider_width = (BoundingRect().width() / content_rect_.width()) *
                         BoundingRect().width();

      auto tmp = QPixmap(":/CC/images/interface/sliderBarV.png");
      auto top = tmp.copy(0, 0, 22, 5);
      auto bottom = tmp.copy(0, 38, 22, 5);
      auto middle = tmp.copy(0, 5, 22, 5);
      middle = middle.scaled(22, Height() - 10);

      slider_rail_ = QPixmap(22, Height());
      slider_rail_.fill(Qt::transparent);

      auto painter = new QPainter(&slider_rail_);
      painter->drawPixmap(0, 0, top);
      painter->drawPixmap(0, 5, middle);
      painter->drawPixmap(0, Height() - 5, bottom);
      painter->rotate(90);
      painter->end();
      delete painter;

      slider_rect_ = QRectF(BoundingRect().x(),
                            BoundingRect().y() + BoundingRect().height() - 22,
                            slider_width, 22);

      slider_ = QPixmap(slider_width, 22);
      slider_.fill(Qt::transparent);

      painter = new QPainter(&slider_);

      tmp = QPixmap(":/CC/images/interface/sliderV.png");
      top = tmp.copy(0, 0, 22, 18);
      bottom = tmp.copy(0, 19, 22, 18);
      middle = tmp.copy(0, 18, 22, 1);

      if (slider_width <= 36) {
        painter->drawPixmap(0, 0, top.scaled(22, slider_width / 2));
        painter->drawPixmap(0, slider_width / 2,
                            bottom.scaled(22, slider_width / 2));
      } else {
        painter->drawPixmap(0, 0, top);
        painter->drawPixmap(0, 18, middle.scaled(22, slider_width - 36));
        painter->drawPixmap(0, slider_width - 18, bottom);
      }
      painter->rotate(90);
      painter->end();
      delete painter;
    }
  }
}

void ListView::MousePressEvent(const QPointF &point, bool &press_validated) {
  if (press_validated) return;

  // Verify if point is in listview, if true calculate the relative position
  // of the point in the content
  const QRectF &b2 = BoundingRect();
  const QRectF &t2 = MapRectToScene(b2);
  if (t2.contains(point)) {
    press_validated = true;
    is_slider_pressed_ = true;
    last_point_ = point;
    initial_point_ = point;
  }
}

void ListView::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  is_slider_pressed_ = false;
  // Verify if point is in listview, if true calculate the relative position
  // of the point in the content
  const QRectF &b2 = BoundingRect();
  const QRectF &t2 = MapRectToScene(b2);
  if (t2.contains(point)) {
    if ((point.x() > initial_point_.x() - 10 &&
         point.x() < initial_point_.x() + 10) &&
        (point.y() > initial_point_.y() - 10 &&
         point.y() < initial_point_.y() + 10)) {
    } else {
      return;
    }
    QPointF auxiliar = QPointF(content_rect_.x() + point.x() - t2.x(),
                               content_rect_.y() + point.y() - t2.y());
    auto items = items_;
    has_dirty_items_ = false;
    for (auto item : items) {
      if (has_dirty_items_) {
        break;
      }
      bool prevent_default = false;
      item->MousePressEvent(auxiliar, prevent_default);
      item->MouseReleaseEvent(auxiliar, prev_validated);

      if (prev_validated) {
        ReDraw();
      }
    }
  }
}

void ListView::MouseMoveEvent(const QPointF &point) {
  if (is_slider_pressed_) {
    int delta = last_point_.y() - point.y();
    last_point_ = point;

    if (delta == 0) {
      return;
    }

    qreal tmp = content_rect_.y() + delta * content_scale_;
    int end = (int)(content_rect_.height() - bounding_rect_.height());

    if (end < 0) {
      return;
    }

    if (tmp < 0) {
      tmp = 0;
      delta = 0;
    } else if (tmp > end) {
      tmp = end;
      delta = 0;
    }
    content_rect_.moveTop(tmp);
    slider_rect_.moveTop(slider_rect_.y() + delta);
    ReDraw();
  }
}

void ListView::Clear() {
  has_dirty_items_ = true;
  auto items = items_;
  items_.clear();
  for (std::vector<Node *>::iterator it = items.begin(); it != items.end();
       ++it) {
    delete *it;
  }
  ReDraw();
}

void ListView::SetHorizontal(bool horizontal) {
  if (is_horizontal_ == horizontal) return;
  is_horizontal_ = horizontal;
  ReCalculateSize();
  ReDraw();
}

uint8_t ListView::Count() { return items_.size(); }

void ListView::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void ListView::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void ListView::SetItemSpacing(int value) {
  spacing_ = value;
  ReCalculateSize();
}

void ListView::OnChildResize(Node *node) {
  if (is_processing_resize_) return;
  is_processing_resize_ = true;
  ReCalculateSize();
  is_processing_resize_ = false;
}
