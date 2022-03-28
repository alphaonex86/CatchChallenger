// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_TEST_TEST_HPP_
#define CLIENT_V3_SCENES_TEST_TEST_HPP_

#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Button.hpp"

namespace Scenes {
class Test : public Scene {
 public:
  static Test *Create();
  ~Test();
  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;

 private:
  Test();
  Sprite *sprite_;
  Sprite *sprite_2;
  Sprite *sprite_3;
  Sprite *sprite_4;
  Sprite *sprite_5;
  UI::Label *label_;
  UI::Button *button_;

  void OnButtonClick(Node* node);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_TEST_TEST_HPP_
