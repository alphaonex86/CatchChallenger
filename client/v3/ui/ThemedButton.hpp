// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_THEMEDBUTTON_HPP_
#define CLIENT_V3_UI_THEMEDBUTTON_HPP_

#include "Button.hpp"

namespace UI {
class GreenButton : public UI::Button {
 public:
  static GreenButton *Create(Node *parent = nullptr);

 protected:
  QPixmap ScaleBackground(QPixmap pixmap, qreal width, qreal height) override;

 private:
  GreenButton(Node *parent = nullptr);
};
class YellowButton : public UI::Button {
 public:
  static YellowButton *Create(Node *parent = nullptr);

 protected:
  QPixmap ScaleBackground(QPixmap pixmap, qreal width, qreal height) override;

 private:
  YellowButton(Node *parent = nullptr);
};
}  // namespace UI
#endif  // CLIENT_V3_UI_THEMEDBUTTON_HPP_
