// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_MAP_HPP_
#define CLIENT_V3_SCENES_MAP_MAP_HPP_

#include "../../core/StackedScene.hpp"
#include "CCMap.hpp"
#include "OverMapLogic.hpp"

namespace Scenes {
class Map : public StackedScene {
 public:
  static Map* Create();
  ~Map();

 private:
  Map();

  CCMap* background_;
  OverMapLogic* overmap_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_MAP_HPP_
