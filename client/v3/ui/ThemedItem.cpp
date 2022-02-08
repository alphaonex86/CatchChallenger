// Copyright 2021 CatchChallenger
#include "ThemedItem.hpp"

#include "../core/Sprite.hpp"
#include "ThemedLabel.hpp"

using UI::DotItem;
using UI::IconItem;

DotItem::DotItem(): UI::SelectableItem() {
  bg_selected_ = QString();
  text_size_ = 10;
  SetHeight(30);
}

DotItem* DotItem::Create() {
  return new (std::nothrow)DotItem();
}

void DotItem::OnResize() {
  ReDraw();
  ReDrawContent();
}

void DotItem::SetText(const QString &text) {
  text_ = text;
}

void DotItem::DrawContent(QPainter *painter) {
  Sprite *dot = Sprite::Create(":/CC/images/interface/dot.png");
  dot->SetSize(12, 12);

  auto *label = UI::BlueLabel::Create();
  label->SetText(text_);
  label->SetPixelSize(text_size_);

  dot->SetPos(5, Height() / 2 - dot->Height() / 2);
  label->SetPos(30, Height() /2 - label->Height() / 2);

  dot->Render(painter);
  label->Render(painter);

  delete dot;
  delete label;
}

void DotItem::SetPixelSize(uint8_t size) {
 text_size_ = size;
}

IconItem::IconItem(): UI::SelectableItem() {
  text_size_ = 10;
  SetHeight(60);
}

IconItem* IconItem::Create() {
  return new (std::nothrow)IconItem();
}

void IconItem::OnResize() {
  ReDraw();
  ReDrawContent();
}

void IconItem::SetText(const QString &text) {
  text_ = text;
}

void IconItem::DrawContent(QPainter *painter) {
  Sprite *icon = Sprite::Create();
  icon->SetSize(30, 30);
  icon->SetPixmap(icon_);

  auto *label = UI::BlueLabel::Create();
  label->SetText(text_);
  label->SetPixelSize(text_size_);

  icon->SetPos(5, Height() / 2 - icon->Height() / 2);
  label->SetPos(icon->Width() + 10, Height() /2 - label->Height() / 2);

  icon->Render(painter);
  label->Render(painter);

  delete icon;
  delete label;
}

void IconItem::SetIcon(const QPixmap &icon) {
  icon_ = icon;
}

void IconItem::SetPixelSize(uint8_t size) {
 text_size_ = size;
}
