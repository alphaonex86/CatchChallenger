// Copyright 2021 CatchChallenger
#include "IndustryItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/Label.hpp"
#include "../../../Constants.hpp"

using Scenes::IndustryItem;

IndustryItem::IndustryItem(uint16_t object_id, uint32_t quantity,
                           uint32_t price)
    : UI::SelectableItem() {
  object_id_ = object_id;
  quantity_ = quantity;
  price_ = price;
  SetSize(60, Constants::ItemSmallHeight());
}

IndustryItem::~IndustryItem() {}

IndustryItem *IndustryItem::Create(uint16_t object_id, uint32_t quantity,
                                   uint32_t price) {
  return new (std::nothrow) IndustryItem(object_id, quantity, price);
}

IndustryItem *IndustryItem::Create(
    CatchChallenger::ItemToSellOrBuy item) {
  return IndustryItem::Create(item.object, item.quantity, item.price);
}

void IndustryItem::DrawContent(QPainter *painter) {
  const auto &playerInformations =
      PlayerInfo::GetInstance()->GetInformationRO();
  if (quantity_ > 1) {
    text_ = QStringLiteral("%1 at %2$").arg(quantity_).arg(price_);
  } else {
    text_ = QStringLiteral("%1$").arg(price_);
  }
  const uint16_t &itemId = object_id_;
  if (QtDatapackClientLoader::GetInstance()->get_itemsExtra().find(itemId) !=
      QtDatapackClientLoader::GetInstance()->get_itemsExtra().cend()) {
    icon_ = QtDatapackClientLoader::GetInstance()->getItemExtra(itemId).image;
  } else {
    icon_ = QtDatapackClientLoader::GetInstance()->defaultInventoryImage();
  }
  auto pix = Sprite::Create();
  pix->SetPixmap(icon_);
  pix->SetTransformationMode(Qt::FastTransformation);
  pix->ScaledToHeight(Height() * 0.75);
  pix->SetPos(10, Height() / 2 - pix->Height() / 2);
  pix->Render(painter);

  auto text = UI::Label::Create(Qt::white, QColor(30, 25, 17));
  text->SetText(text_);
  text->SetPixelSize(Constants::TextSmallSize());
  text->SetPos(pix->Right() + 10, 5);
  text->Render(painter);

  delete pix;
  delete text;
}

void IndustryItem::OnResize() { ReDraw(); }

uint16_t IndustryItem::ObjectId() { return object_id_; }

uint32_t IndustryItem::Quantity() { return quantity_; }

uint32_t IndustryItem::Price() { return price_; }

void IndustryItem::SetText(const QString &text) { text_ = text; }

void IndustryItem::SetIcon(const QPixmap &icon) { icon_ = icon; }

void IndustryItem::SetQuantity(uint32_t quantity) {
  quantity_ = quantity;
  ReDrawContent();
  ReDraw();
}

void IndustryItem::SetPrice(uint32_t price) {
  price_ = price;
  ReDrawContent();
  ReDraw();
}
