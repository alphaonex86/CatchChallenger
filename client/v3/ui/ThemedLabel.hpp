// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_THEMEDLABEL_HPP_
#define CLIENT_V3_UI_THEMEDLABEL_HPP_

#include "Label.hpp"

namespace UI {
class BlueLabel {
 public:
  static Label *Create(Node *parent = nullptr);
};
}  // namespace UI
#endif  // CLIENT_V3_UI_THEMEDLABEL_HPP_
