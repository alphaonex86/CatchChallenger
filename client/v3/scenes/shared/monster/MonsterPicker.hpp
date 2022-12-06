// Copyright 2022 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERPICKER_HPP_
#define CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERPICKER_HPP_

#include "../../../ui/LinkedDialog.hpp"
#include "MonsterBag.hpp"

namespace Scenes {
class MonsterPicker : public UI::LinkedDialog {
 public:
  ~MonsterPicker();
  static MonsterPicker *Create();

  void Show(std::function<void(uint8_t)> on_select);

 private:
  MonsterPicker();

  MonsterBag *bag_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERPICKER_HPP_
