// Copyright 2021 CatchChallenger
#include "CharacterItem.hpp"

#include <iostream>

#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"

using Scenes::CharacterItem;

CharacterItem::CharacterItem()
    : UI::SelectableItem() {
  SetSize(200, 70);
}

CharacterItem *CharacterItem::Create() {
  return new (std::nothrow) CharacterItem();
}

void CharacterItem::DrawContent(QPainter *painter) {
  auto image = Sprite::Create(icon_);
  image->SetPos(10, 0);
  image->SetSize(image->Width() - 10, 60);
  image->Render(painter);

  auto text = UI::Label::Create(QColor(255, 255, 255), QColor(30, 25, 17));
  text->SetWidth(bounding_rect_.width());
  text->SetAlignment(Qt::AlignCenter);
  text->SetText(title_);
  text->SetY(5);
  text->Render(painter);

  text->SetY(text->Height());
  text->SetPixelSize(10);
  text->SetText(subtitle_);
  text->Render(painter);

  delete text;
  delete image;
}

void CharacterItem::OnResize() {
  ReDraw();
}

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
