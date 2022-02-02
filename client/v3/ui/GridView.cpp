// Copyright 2021 CatchChallenger
#include "GridView.hpp"

#include <QPainter>
#include <iostream>

using std::placeholders::_1;
using UI::GridView;

GridView::GridView(Node *parent) : ListView(parent) {
  node_type_ = __func__;
  item_size_ = QSize(48, 48);
}

GridView *GridView::Create(Node *parent) {
  return new (std::nothrow) GridView(parent);
}

void GridView::SetItemSize(qreal width, qreal height) {
  item_size_.setWidth(width);
  item_size_.setHeight(height);
  GridView::ReCalculateSize();
}

void GridView::ReCalculateSize() {
  if (!is_drawable_) return;
  const auto item_width = item_size_.width();
  const auto item_height = item_size_.height();

  if (!is_horizontal_) {
    content_rect_ = QRectF(0.0, 0.0, Width() - 22, item_height);

    if (items_.size() == 0) return;
    for (auto item : items_) {
      if (item->Width() == item_width && item->Height() == item_height) {
        continue;
      }
      item->SetSize(item_width, item_height);
    }
    int x = 0;
    int y = 0;
    for (auto item : items_) {
      if (x + item_size_.width() > Width()) {
        x = 0;
        content_rect_.setHeight(item_height + y);
        y = y + item_height + spacing_;
      }
      item->SetPos(x, y);

      x = x + item->Width() + spacing_;
    }
    content_rect_.setHeight(item_height + y);

    show_slider_ = content_rect_.height() > Height();

    if (!show_slider_) {
      content_rect_.setWidth(Width());
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
