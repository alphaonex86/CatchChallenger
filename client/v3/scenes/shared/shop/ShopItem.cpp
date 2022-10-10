// Copyright 2021 <CatchChallenger>
#include "ShopItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../../general/base/CommonDatapack.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ThemedButton.hpp"
#include "../../../Constants.hpp"

using Scenes::ShopItem;

ShopItem::ShopItem(uint16_t object_id, uint32_t quantity, uint32_t price)
    : UI::SelectableItem() {
  object_id_ = object_id;
  quantity_ = quantity;
  price_ = price;
  SetSize(60, Constants::ButtonSmallHeight());
}

ShopItem::~ShopItem() {}

ShopItem *ShopItem::Create(uint16_t object_id, uint32_t quantity,
                           uint32_t price) {
  return new (std::nothrow) ShopItem(object_id, quantity, price);
}

void ShopItem::DrawContent(QPainter *painter) {
  auto item = QtDatapackClientLoader::GetInstance()->get_itemsExtra().at(object_id_);
  QPixmap p =
      QtDatapackClientLoader::GetInstance()->getItemExtra(object_id_).image;
  p = p.scaledToHeight(Height() / 2);

  name_ = item.name;

  auto pix = Sprite::Create();
  pix->SetPixmap(p);
  pix->SetPos(10, Height() / 2 - pix->Height() / 2);
  pix->Render(painter);

  auto text = UI::Label::Create(Qt::white, QColor(30, 25, 17));
  text->SetText(item.name);
  text->SetPixelSize(Constants::TextMediumSize());
  text->SetPos(pix->Width() + pix->X() + 10, 5);
  text->Render(painter);

  text->SetPos(pix->Width() + pix->X() + 10, text->Height());
  text->SetText(item.description);
  text->SetPixelSize(Constants::TextSmallSize());
  text->Render(painter);

  auto button = UI::GreenButton::Create();
  button->SetSize(UI::Button::kRectSmall);
  button->SetHeight(Height() * 0.75);
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
