// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_LEADING_LEADING_HPP_
#define CLIENT_V3_SCENES_LEADING_LEADING_HPP_

#include "../../core/StackedScene.hpp"
#include "../ParallaxForest.hpp"
#include "Menu.hpp"

namespace Scenes {
class Leading : public StackedScene {
 public:
  static Leading* Create();
  ~Leading();

 private:
  Leading();

  ParallaxForest* background_;
  Menu* menu_;
  Options* options_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_LEADING_LEADING_HPP_
