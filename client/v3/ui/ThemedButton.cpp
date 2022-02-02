// Copyright 2021 CatchChallenger
#include "ThemedButton.hpp"

using UI::GreenButton;
using UI::YellowButton;

GreenButton::GreenButton(Node *parent)
    : Button(":/CC/images/interface/greenbutton.png", parent) {
  SetOutlineColor(QColor(44, 117, 0));
}

GreenButton *GreenButton::Create(Node *parent) {
  return new (std::nothrow) GreenButton(parent);
}

YellowButton::YellowButton(Node *parent)
    : Button(":/CC/images/interface/button.png", parent) {
}

YellowButton *YellowButton::Create(Node *parent) {
  return new (std::nothrow) YellowButton(parent);
}
