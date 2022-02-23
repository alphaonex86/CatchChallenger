// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_BATTLE_BATTLEACTIONS_HPP_
#define CLIENT_V3_SCENES_BATTLE_BATTLEACTIONS_HPP_

#include "../../action/Sequence.hpp"
#include "../../core/Node.hpp"

namespace Scenes {
class BattleActions {
 public:
  static Sequence* CreateThrowAction(Node* trap, Node* monster,
                                     std::function<void()> callback);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_BATTLEACTIONS_HPP_
