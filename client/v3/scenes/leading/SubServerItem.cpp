// Copyright 2021 CatchChallenger
#include "SubServerItem.hpp"

#include <iostream>

#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"

using Scenes::SubServerItem;

SubServerItem::SubServerItem() : UI::SelectableItem() { SetSize(200, 70); }

SubServerItem *SubServerItem::Create() {
  return new (std::nothrow) SubServerItem();
}

void SubServerItem::DrawContent(QPainter *painter) {
  auto bounding = BoundingRect();
  auto image = Sprite::Create();
  qreal x_pos = 0;
  if (!icon_.isEmpty()) {
    image->SetPixmap(icon_);
    image->SetSize(40, 27);
    image->SetPos(30, bounding.height() / 2 - image->Height() / 2);
    x_pos = image->X() + image->Width();
    image->Render(painter);
  }

  if (!stat_.isEmpty()) {
    image->SetPixmap(stat_);
    image->SetSize(30, 30);
    image->SetPos(bounding.width() - image->Width() - 30,
                  bounding.height() / 2 - image->Height() / 2);
    image->Render(painter);
  }

  if (!star_.isEmpty()) {
    image->SetPixmap(star_);
    image->SetSize(30, 30);
    image->SetPos(x_pos + 10,
                  bounding.height() / 2 - image->Height() / 2);
    image->Render(painter);
  }

  auto text = UI::Label::Create(QColor(255, 255, 255), QColor(30, 25, 17));
  text->SetWidth(bounding.width());
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

void SubServerItem::OnResize() { ReDraw(); }

void SubServerItem::SetIcon(const QString &icon) {
  icon_ = icon;
  ReDrawContent();
}

void SubServerItem::SetTitle(const QString &text) {
  title_ = text;
  ReDrawContent();
}

void SubServerItem::SetSubTitle(const QString &text) {
  subtitle_ = text;
  ReDrawContent();
}

void SubServerItem::SetStat(const QString &text) {
  stat_ = text;
  ReDrawContent();
}

void SubServerItem::SetStar(const QString &text) {
  star_ = text;
  ReDrawContent();
}
