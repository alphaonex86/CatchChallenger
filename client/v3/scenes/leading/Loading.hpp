// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_LOADING_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_LOADING_HPP_

#include "../../action/RepeatForever.hpp"
#include "../../core/ProgressNotifier.hpp"
#include "../../core/Scene.hpp"
#include "../../core/Node.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Progressbar.hpp"

namespace Scenes {
class Loading : public Scene {
 public:
  static Loading *GetInstance();
  ~Loading();
  void Reset();
  void Progression(uint32_t size, uint32_t total);
  void SetText(QString text);
  void SetNotifier(ProgressNotifier *notifier);
  void OnEnter() override;
  void OnExit() override;

 protected:
  void CanBeChanged();
  void DataIsParsed();
  void UpdateProgression();

  void OnScreenResize() override;

 private:
  static Loading *instance_;
  Loading();
  Sprite *widget;
  Sprite *teacher;
  UI::Label *info;
  UI::Progressbar *progressbar;
  UI::Label *version;
  RepeatForever *timer_;
  uint8_t lastProgression;
  uint8_t timerProgression;
  bool doTheNext;
};
}  // namespace Scenes
#endif  // CLIENT_QTOPENGL_SCENES_LEADING_LOADING_HPP_
