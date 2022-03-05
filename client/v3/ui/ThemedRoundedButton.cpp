// Copyright 2021 CatchChallenger
#include "ThemedRoundedButton.hpp"

#include <QPainter>
#include <iostream>

using UI::TextRoundedButton;

TextRoundedButton::TextRoundedButton(Node *parent) : RoundedButton(parent) {
  font_ = new QFont();
  font_->setFamily("Roboto Condensed");
  font_->setPixelSize(16);
  text_ = QString();

  font_icon_ = new QFont();
  font_icon_->setFamily("icomoon");
  font_icon_->setPixelSize(56);
}

TextRoundedButton *TextRoundedButton::Create(Node *parent) {
  return new (std::nothrow) TextRoundedButton(parent);
}

void TextRoundedButton::DrawContent(QPainter *painter, QColor color,
                                    QRectF boundary) {
  painter->setPen(color);
  painter->setFont(*font_);
  painter->drawText(0, 25, text_);

  painter->setPen(icon_color_);
  painter->setFont(*font_icon_);
  painter->drawText(boundary.width() - 55, 50, icon_);
}

void TextRoundedButton::SetText(const QString &text) {
  text_ = text;
  ReDraw();
}

void TextRoundedButton::SetIcon(const QString &icon, QColor color) {
  icon_ = icon;
  icon_color_ = color;
  ReDraw();
}
