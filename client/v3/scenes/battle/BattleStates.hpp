// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_BATTLE_BATTLESTATES_HPP_
#define CLIENT_V3_SCENES_BATTLE_BATTLESTATES_HPP_

#include "../../base/ConnectionManager.hpp"

namespace Scenes {
class Battle;
class BattleContext;

class BattleState {
 public:
  virtual void Handle(BattleContext *context, Battle *battle) = 0;
};

class WildPresentationState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class CallMonsterState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
  void LoadPlayerMonster(Battle *battle,
                         CatchChallenger::PlayerMonster *fight_monster);
};

class UserInputState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class BattleBehaviorState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class BattleCycleState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class EscapeState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class UseItemState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class UseTrapState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class FightState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class EndState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class ChooseMonsterState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class MonsterDeadState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class UpdateMonsterStatState : public BattleState {
 public:
  void Handle(BattleContext *context, Battle *battle) override;
};

class BattleContext {
 public:
  static BattleContext *Create(Battle *battle);

  enum StartState { kStartState_Wild, kStartState_PVN, kStartState_PVP };

  void SetState(BattleState *state);
  void SetTransition(bool is_transition);
  void Handle();
  void Restart(StartState start);

 private:
  BattleState *state_;
  Battle *battle_;
  bool is_transition_;

  BattleContext(Battle *battle);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_BATTLESTATES_HPP_
