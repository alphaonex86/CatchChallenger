// Copyright 2021 CatchChallenger
#include "InventoryItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../Constants.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Label.hpp"

using Scenes::InventoryItem;

InventoryItem::InventoryItem() {
  SetSize(Constants::ButtonMediumHeight(), Constants::ButtonMediumHeight());
  is_selected_ = false;
}

InventoryItem *InventoryItem::Create() {
  return new (std::nothrow) InventoryItem();
}

void InventoryItem::SetPixmap(const QPixmap pixmap) { pixmap_ = pixmap; }

void InventoryItem::SetText(const QString text) { text_ = text; }

QString InventoryItem::Text() const { return text_; }

void InventoryItem::SetSelected(bool selected) {
  is_selected_ = selected;
  ReDraw();
}

bool InventoryItem::IsSelected() { return is_selected_; }

void InventoryItem::Draw(QPainter *painter) {
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

void InventoryItem::MousePressEvent(const QPointF &point,
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

void InventoryItem::MouseReleaseEvent(const QPointF &point,
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
