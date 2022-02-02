// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_LEADING_DEBUG_HPP_
#define CLIENT_V3_SCENES_LEADING_DEBUG_HPP_

#include <vector>
#include <QTime>

#include "../../ui/Button.hpp"
#include "../../ui/Dialog.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Checkbox.hpp"
#include "../../ui/Column.hpp"
#include "../../action/RepeatForever.hpp"

namespace Scenes {
class Debug : public UI::Dialog {
 public:
  ~Debug();

  static Debug *Create();

  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;
  void Draw(QPainter *painter) override;

 private:
  UI::Label *debugText;
  bool debugIsShow;
  RepeatForever* updater_;

  Debug();
};
}  // namespace Scenes

#endif  // CLIENT_V3_SCENES_LEADING_DEBUG_HPP_
