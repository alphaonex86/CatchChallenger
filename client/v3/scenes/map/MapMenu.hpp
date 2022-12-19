// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_MAPMENU_HPP_
#define CLIENT_V3_SCENES_MAP_MAPMENU_HPP_

#include "../../scenes/shared/options/Options.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Column.hpp"
#include "../../ui/Dialog.hpp"

namespace Scenes {
class MapMenu : public UI::Dialog {
 public:
  ~MapMenu();

  static MapMenu *Create();

  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;

 private:
  UI::Button *go_to_main_;
  UI::Button *exit_;
  UI::Button *options_;
  UI::Button *open_to_lan_;
  Options *options_dialog_;
  UI::Column *buttons_;

  MapMenu();
};
}  // namespace Scenes

#endif  // CLIENT_V3_SCENES_MAP_MAPMENU_HPP_
