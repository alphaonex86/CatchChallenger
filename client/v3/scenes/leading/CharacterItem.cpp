// Copyright 2021 CatchChallenger
#include "CharacterItem.hpp"

#include <iostream>

#include "../../Constants.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"

using Scenes::CharacterItem;

CharacterItem::CharacterItem() : UI::SelectableItem() {
  SetSize(200, Constants::ItemMediumHeight());
}

CharacterItem *CharacterItem::Create() {
  return new (std::nothrow) CharacterItem();
}

void CharacterItem::DrawContent(QPainter *painter) {
  auto image = Sprite::Create(icon_);
  auto icon_height = Height() * 0.9;
  auto icon_width = image->Width() / icon_height * image->Width();
  image->SetSize(icon_width, icon_height);
  image->SetPos(Height() / 2 - icon_height / 2, 0);
  image->Render(painter);

  auto text = UI::Label::Create(QColor(255, 255, 255), QColor(30, 25, 17));
  text->SetWidth(bounding_rect_.width());
  text->SetAlignment(Qt::AlignCenter);
  text->SetText(title_);
  text->SetY(Constants::ItemSmallSpacing());
  text->Render(painter);

  text->SetY(text->Height());
  text->SetPixelSize(Constants::TextSmallSize());
  text->SetText(subtitle_);
  text->Render(painter);

  delete text;
  delete image;
}

void CharacterItem::OnResize() { ReDraw(); }

void CharacterItem::SetIcon(const QString &icon) {
  icon_ = icon;
  ReDrawContent();
}

void CharacterItem::SetTitle(const QString &text) {
  title_ = text;
  ReDrawContent();
}

void CharacterItem::SetSubTitle(const QString &text) {
  subtitle_ = text;
  ReDrawContent();
}
