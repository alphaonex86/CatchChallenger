// Copyright 2021 CatchChallenger
#include "BattleActions.hpp"

#include <iostream>

#include "../../action/CallFunc.hpp"
#include "../../action/MoveTo.hpp"

using Scenes::BattleActions;

Sequence *BattleActions::CreateThrowAction(Node *trap, Node *monster,
                                           std::function<void()> callback) {
  auto x = monster->Right() - monster->Width() / 2;
  auto y = monster->Bottom() - trap->Height();
  return Sequence::Create(
      MoveTo::Create(1000, x, monster->Y()), CallFunc::Create([=]() {
        std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                  << "achicado" << std::endl;
      }),
      MoveTo::Create(500, x, y), CallFunc::Create(callback), nullptr);
}
