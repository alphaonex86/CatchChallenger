// Copyright 2021 CatchChallenger
#include "CraftingItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Label.hpp"
#include "../../../Constants.hpp"

using Scenes::CraftingItem;

CraftingItem::CraftingItem() : UI::SelectableItem() {
  SetSize(60, Constants::ItemSmallHeight());
  bg_disabled_ = QString(":/CC/images/interface/b1-red.png");
}

CraftingItem::~CraftingItem() {}

CraftingItem *CraftingItem::Create() {
  return new (std::nothrow) CraftingItem();
}

void CraftingItem::DrawContent(QPainter *painter) {
  auto item = QtDatapackClientLoader::GetInstance()->get_itemsExtra().at(item_id_);
  icon_ = QtDatapackClientLoader::GetInstance()->getItemExtra(item_id_).image;
  icon_ = icon_.scaledToHeight(Height() * 0.8);

  text_ = QString::fromStdString(item.name);
  description_ = QString::fromStdString(item.description);

  auto pix = Sprite::Create();
  pix->SetPixmap(icon_);
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

  delete pix;
  delete text;
}

void CraftingItem::OnResize() { ReDraw(); }

void CraftingItem::SetItemId(const uint16_t &item_id) { item_id_ = item_id; }

void CraftingItem::SetIcon(const QPixmap &pixmap) { icon_ = pixmap; }

void CraftingItem::SetName(const QString &text) { text_ = text; }

void CraftingItem::SetRecipe(const uint16_t &item_id) { recipe_ = item_id; }

QString CraftingItem::Name() const { return text_; }

QString CraftingItem::Description() const { return description_; }

uint16_t CraftingItem::ItemId() const { return item_id_; }

uint16_t CraftingItem::Recipe() const { return recipe_; }

QPixmap CraftingItem::Icon() const { return icon_; }
