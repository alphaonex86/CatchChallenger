// Copyright 2021 CatchChallenger
#include "ThemedLabel.hpp"

using UI::BlueLabel;
using UI::Label;

Label *BlueLabel::Create(Node *parent) {
  return Label::Create(QColor(255, 255, 255), QColor(0, 13, 40), parent);
}
