// Copyright 2021 CatchChallenger
#include "IndustryItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/Label.hpp"

using Scenes::IndustryItem;

IndustryItem::IndustryItem(uint16_t object_id, uint32_t quantity,
                           uint32_t price)
    : UI::SelectableItem() {
  object_id_ = object_id;
  quantity_ = quantity;
  price_ = price;
  SetSize(60, 60);
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
  if (QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(itemId) !=
      QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend()) {
    icon_ = QtDatapackClientLoader::datapackLoader->getItemExtra(itemId).image;
  } else {
    icon_ = QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
  }
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << quantity_ << std::endl;
  auto pix = Sprite::Create();
  pix->SetPixmap(icon_);
  pix->SetPos(10, Height() / 2 - pix->Height() / 2);
  pix->Render(painter);

  auto text = UI::Label::Create(Qt::white, QColor(30, 25, 17));
  text->SetText(text_);
  text->SetPixelSize(12);
  text->SetPos(pix->Width() + pix->X() + 10, 5);
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