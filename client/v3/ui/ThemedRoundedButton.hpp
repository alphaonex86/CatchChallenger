// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_THEMEDROUNDEDBUTTON_HPP_
#define CLIENT_V3_UI_THEMEDROUNDEDBUTTON_HPP_

#include "RoundedButton.hpp"

namespace UI {
class TextRoundedButton : public UI::RoundedButton {
 public:
  static TextRoundedButton *Create(Node *parent = nullptr);
  void DrawContent(QPainter *painter, QColor color, QRectF boundary) override;
  void SetText(const QString &text);
  void SetIcon(const QString &icon, QColor color);

 private:
  QString text_;
  QString icon_;
  QFont *font_;
  QFont *font_icon_;
  QColor icon_color_;

  TextRoundedButton(Node *parent = nullptr);
};
}  // namespace UI
#endif  // CLIENT_V3_UI_THEMEDROUNDEDBUTTON_HPP_
