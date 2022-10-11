// Copyright 2021 CatchChallenger
#include "WarehouseItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Label.hpp"
#include "../../../Constants.hpp"

using Scenes::WarehouseItem;

WarehouseItem::WarehouseItem() {
  SetSize(Constants::ButtonSmallHeight(), Constants::ButtonSmallHeight());
  is_selected_ = false;
}

WarehouseItem *WarehouseItem::Create() {
  return new (std::nothrow) WarehouseItem();
}

void WarehouseItem::SetPixmap(const QPixmap pixmap) {
  pixmap_ = pixmap.scaledToHeight(Height());
}

void WarehouseItem::SetText(const QString text) { text_ = text; }

QString WarehouseItem::Text() const { return text_; }

void WarehouseItem::SetSelected(bool selected) {
  is_selected_ = selected;
  ReDraw();
}

bool WarehouseItem::IsSelected() { return is_selected_; }

void WarehouseItem::Draw(QPainter *painter) {
  const auto size = BoundingRect();
  QPixmap background = *AssetsLoader::GetInstance()->GetImage(
      ":/CC/images/interface/green-square.png");
  if (is_selected_) {
    painter->drawPixmap(0, 0, size.width(), size.height(), background, 0, 0,
                        114, 117);
  } else {
    painter->drawPixmap(0, 0, size.width(), size.height(), background, 0, 117,
                        114, 117);
  }
  painter->drawPixmap(0, 0, pixmap_);

  auto text = UI::Label::Create();
  text->SetPixelSize(Constants::TextSmallSize());
  text->SetWidth(size.width());
  text->SetAlignment(Qt::AlignCenter);
  text->SetText(text_);
  text->Render(painter);

  delete text;
}

void WarehouseItem::MousePressEvent(const QPointF &point,
                                    bool &press_validated) {
  if (press_validated) {
    is_selected_ = false;
    ReDraw();
    return;
  }
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void WarehouseItem::MouseReleaseEvent(const QPointF &point,
                                      bool &prev_validated) {
  if (prev_validated) {
    is_selected_ = false;
    ReDraw();
    return;
  }

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      is_selected_ = true;
      ReDraw();
      if (on_click_) {
        on_click_(this);
      }
      prev_validated = true;
    }
  }
}
