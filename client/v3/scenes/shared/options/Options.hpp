// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_OPTIONS_OPTIONS_HPP_
#define CLIENT_V3_SCENES_SHARED_OPTIONS_OPTIONS_HPP_

#include <string>
#include <vector>

#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Combo.hpp"
#include "../../ui/Dialog.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Slider.hpp"

namespace Scenes {
class Options : public UI::Dialog {
 public:
  static Options *Create();
  ~Options();
  void OnScreenResize() override;
  void OnEnter() override;
  void OnExit() override;

 private:
  UI::Label *volumeText;
  UI::Slider *volumeSlider;

  UI::Label *productKeyText;
  UI::Input *productKeyInput;
  UI::Button *buy;

  UI::Label *languagesText;
  UI::Combo *languagesList;

  QString previousKey;

  Options();
  void volumeSliderChange();
  void productKeyChange();
  void languagesChange(int index);
  void openBuy();

  void newLanguage();
};
}  // namespace Scenes

#endif  // CLIENT_V3_SCENES_SHARED_OPTIONS_OPTIONS_HPP_
