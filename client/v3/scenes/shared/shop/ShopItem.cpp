// Copyright 2021 <CatchChallenger>
#include "ShopItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../../general/base/CommonDatapack.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ThemedButton.hpp"

using Scenes::ShopItem;

ShopItem::ShopItem(uint16_t object_id, uint32_t quantity, uint32_t price)
    : UI::SelectableItem() {
  object_id_ = object_id;
  quantity_ = quantity;
  price_ = price;
  SetSize(60, 60);
}

ShopItem::~ShopItem() {}

ShopItem *ShopItem::Create(uint16_t object_id, uint32_t quantity,
                           uint32_t price) {
  return new (std::nothrow) ShopItem(object_id, quantity, price);
}

void ShopItem::DrawContent(QPainter *painter) {
  auto item = QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(object_id_);
  QPixmap p =
      QtDatapackClientLoader::datapackLoader->getItemExtra(object_id_).image;
  p = p.scaledToHeight(32);

  name_ = item.name;

  auto pix = Sprite::Create();
  pix->SetPixmap(p);
  pix->SetPos(10, Height() / 2 - pix->Height() / 2);
  pix->Render(painter);

  auto text = UI::Label::Create(Qt::white, QColor(30, 25, 17));
  text->SetText(item.name);
  text->SetPixelSize(12);
  text->SetPos(pix->Width() + pix->X() + 10, 5);
  text->Render(painter);

  text->SetPos(pix->Width() + pix->X() + 10, text->Height());
  text->SetText(item.description);
  text->SetPixelSize(8);
  text->Render(painter);

  auto button = UI::GreenButton::Create();
  button->SetSize(60, 25);
  button->SetPixelSize(8);
  button->SetText(QStringLiteral("%1 $").arg(price_));
  button->SetPos(Width() - button->Width() - 10,
                 Height() / 2 - button->Height() / 2);
  button->Render(painter);

  delete pix;
  delete text;
  delete button;
}

void ShopItem::OnResize() { ReDraw(); }

std::string ShopItem::Name() { return name_; }

uint16_t ShopItem::ObjectId() { return object_id_; }

uint32_t ShopItem::Quantity() { return quantity_; }

uint32_t ShopItem::Price() { return price_; }