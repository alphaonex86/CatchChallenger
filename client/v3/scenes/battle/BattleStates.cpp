// Copyright 2021 CatchChallenger
#include "BattleStates.hpp"

#include <QObject>
#include <iostream>

#include "../../../general/base/GeneralStructures.hpp"
#include "../../action/Animation.hpp"
#include "../../action/Blink.hpp"
#include "../../action/CallFunc.hpp"
#include "../../action/MoveTo.hpp"
#include "../../action/Sequence.hpp"
#include "../../entities/Utils.hpp"
#include "ActionBar.hpp"
#include "Battle.hpp"

using Scenes::ActionBar;
using Scenes::Battle;
using Scenes::BattleBehaviorState;
using Scenes::BattleContext;
using Scenes::BattleCycleState;
using Scenes::BattleState;
using Scenes::CallMonsterState;
using Scenes::ChooseMonsterState;
using Scenes::EndState;
using Scenes::EscapeState;
using Scenes::FightState;
using Scenes::MonsterDeadState;
using Scenes::UpdateMonsterStatState;
using Scenes::UseItemState;
using Scenes::UserInputState;
using Scenes::UseTrapState;
using Scenes::WildPresentationState;

BattleContext::BattleContext(Battle *battle) {
  battle_ = battle;
  state_ = nullptr;
}

BattleContext *BattleContext::Create(Battle *battle) {
  return new (std::nothrow) BattleContext(battle);
}

void BattleContext::SetState(BattleState *state) { state_ = state; }

void BattleContext::SetTransition(bool is_transition) {
  is_transition_ = is_transition;
}

void BattleContext::Handle() {
  is_transition_ = false;
  auto state = state_;
  state->Handle(this, battle_);
  delete state;

  if (is_transition_) {
    Handle();
  }
}

void BattleContext::Restart(StartState state) {
  if (state_ != nullptr) {
    delete state_;
    state_ = nullptr;
  }
  switch (state) {
    case kStartState_Wild:
      state_ = new WildPresentationState();
      break;
    case kStartState_PVN:
      state_ = new WildPresentationState();
      break;
    case kStartState_PVP:
      state_ = new WildPresentationState();
      break;
  }
  Handle();
}

void WildPresentationState::Handle(BattleContext *context, Battle *battle) {
  battle->action_bar_->CanRun(true);
  battle->ConfigureFighters();
  battle->ConfigureCommons();
  battle->action_bar_->SetVisible(false);
  battle->player_status_->SetVisible(false);

  if (!battle->client_->isInFight()) {
    // emit error("is in fight but without monster");
    // return false;
  }
  battle->ConfigureBattleground();

  battle->init_other_monster_display();
  CatchChallenger::PublicPlayerMonster *otherMonster =
      battle->client_->getOtherMonster();
  if (otherMonster == NULL) {
    // emit error("NULL pointer for other monster at wildFightCollision()");
    // return false;
  }
  battle->battleStep = Battle::BattleStep_PresentationMonster;
  battle->battleType = Battle::BattleType_Wild;

  battle->player_->SetPos(battle->player_out_);
  battle->enemy_->SetPos(battle->enemy_in_);

  battle->PrintError(
      __FILE__, __LINE__,
      QStringLiteral("You are in front of monster id: %1 level: %2 with hp: %3")
          .arg(otherMonster->monster)
          .arg(otherMonster->level)
          .arg(otherMonster->hp));

  battle->ShowStatusMessage(
      QStringLiteral("Wild %1 appeared")
          .arg(QString::fromStdString(
              QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                  .at(otherMonster->monster)
                  .name)),
      true, false);

  context->SetState(new CallMonsterState());
}

void CallMonsterState::Handle(BattleContext *context, Battle *battle) {
  switch (battle->battleStep) {
    case Battle::BattleStep_Presentation:
      if (battle->client_->isInFightWithWild()) {
        battle->player_->RunAction(battle->player_exit_);
      } else {
        battle->player_->RunAction(battle->both_exit_);
      }
      break;
    case Battle::BattleStep_PresentationMonster:
    default:
      LoadPlayerMonster(battle, nullptr);
      auto monster = battle->client_->getCurrentMonster();
      battle->player_->SetPixmap(QtDatapackClientLoader::datapackLoader
                                     ->getMonsterExtra(monster->monster)
                                     .back.scaled(400, 400));
      battle->action_bar_->SetMonster(monster);
      // battle->UpdateAttackSkills();
      battle->player_->RunAction(
          Sequence::Create(MoveTo::Create(1500, battle->player_in_),
                           CallFunc::Create([context]() { context->Handle(); }),
                           nullptr),
          true);

      if (monster != NULL) {
        battle->ShowStatusMessage(
            QObject::tr("Protect me %1!")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                        .at(monster->monster)
                        .name)),
            false, true);
      } else {
        qDebug() << "OnActionNextClick(): NULL pointer for "
                    "current monster";
        battle->ShowStatusMessage(
            QObject::tr("Protect me %1!").arg("(Unknow monster)"), false, true);
      }
      break;
  }

  context->SetState(new UserInputState());
}

void CallMonsterState::LoadPlayerMonster(
    Battle *battle, CatchChallenger::PlayerMonster *fight_monster) {
  if (fight_monster == nullptr) {
    fight_monster = battle->client_->getCurrentMonster();
  }
  battle->player_status_->SetMonster(fight_monster);
}

void UserInputState::Handle(BattleContext *context, Battle *battle) {
  battle->player_status_->SetVisible(true);
  battle->message_bar_->SetVisible(false);
  battle->action_bar_->SetVisible(true);

  context->SetState(new BattleBehaviorState());
}

void BattleBehaviorState::Handle(BattleContext *context, Battle *battle) {
  context->SetTransition(true);

  int player_hp = battle->player_status_->HP();
  int enemy_hp = battle->enemy_status_->HP();
  auto attack_return = battle->client_->getAttackReturnList();
  int index = 0;
  while (index < attack_return.size()) {
    const auto &item = attack_return.at(index);
    switch (item.attackReturnCase) {
      case CatchChallenger::Skill::AttackReturnCase_NormalAttack:
        if (!item.lifeEffectMonster.empty()) {
          int index_j = 0;
          while (index_j < item.lifeEffectMonster.size()) {
            const auto &life_effect = item.lifeEffectMonster.at(index_j);
            Battle::BattleAction attack;
            attack.do_by_player = item.doByTheCurrentMonster;
            attack.success = item.success;
            attack.id = item.attack;
            attack.monster_index = item.monsterPlace;
            attack.type = Battle::kBattleAction_Fight;
            attack.apply_on = life_effect.on;
            attack.effective = life_effect.effective;
            attack.quantity = life_effect.quantity;
            attack.critical = life_effect.critical;
            attack.effective = life_effect.effective;

            if (attack.do_by_player) {
              if ((attack.apply_on == CatchChallenger::ApplyOn_AloneEnemy) ||
                  (attack.apply_on == CatchChallenger::ApplyOn_AllEnemy)) {
                attack.apply_on_enemy = true;
              }
            } else {
              if ((attack.apply_on == CatchChallenger::ApplyOn_Themself) ||
                  (attack.apply_on == CatchChallenger::ApplyOn_AllAlly)) {
                attack.apply_on_enemy = true;
              }
            }

            if (attack.apply_on_enemy) {
              std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                        << (int)attack.quantity << std::endl;
              enemy_hp += attack.quantity;
            } else {
              player_hp += attack.quantity;
            }

            battle->actions_.push_back(attack);
            index_j++;
          }
        } else if (!item.buffLifeEffectMonster.empty()) {
          int index_j = 0;
          while (index_j < item.buffLifeEffectMonster.size()) {
            const auto &life_effect = item.buffLifeEffectMonster.at(index_j);
            Battle::BattleAction attack;
            attack.do_by_player = item.doByTheCurrentMonster;
            attack.success = item.success;
            attack.id = item.attack;
            attack.type = Battle::kBattleAction_ApplyBuff;
            attack.apply_on = life_effect.on;
            attack.effective = life_effect.effective;
            attack.quantity = life_effect.quantity;
            attack.critical = life_effect.critical;
            attack.effective = life_effect.effective;

            battle->actions_.push_back(attack);
            index_j++;
          }
        } else if (!item.addBuffEffectMonster.empty()) {
          int index_j = 0;
          while (index_j < item.addBuffEffectMonster.size()) {
            const auto &effect = item.addBuffEffectMonster.at(index_j);
            Battle::BattleAction attack;
            attack.do_by_player = item.doByTheCurrentMonster;
            attack.success = item.success;
            attack.id = effect.buff;
            attack.monster_index = item.monsterPlace;
            attack.type = Battle::kBattleAction_AddBuff;
            attack.apply_on = effect.on;
            attack.level = effect.level;

            battle->actions_.push_back(attack);
            index_j++;
          }
        } else if (!item.removeBuffEffectMonster.empty()) {
          int index_j = 0;
          while (index_j < item.removeBuffEffectMonster.size()) {
            const auto &effect = item.removeBuffEffectMonster.at(index_j);
            Battle::BattleAction attack;
            attack.do_by_player = item.doByTheCurrentMonster;
            attack.success = item.success;
            attack.id = effect.buff;
            attack.monster_index = item.monsterPlace;
            attack.type = Battle::kBattleAction_AddBuff;
            attack.apply_on = effect.on;
            attack.level = effect.level;

            battle->actions_.push_back(attack);
            index_j++;
          }
        } else {
          battle->client_->removeTheFirstAttackReturn();
          return;
        }

        // Determine if the monster is dead
        if (player_hp <= 0) {
          Battle::BattleAction action;
          action.type = Battle::kBattleAction_MonsterDead;
          action.apply_on_enemy = false;
          battle->actions_.push_back(action);
        }

        if (enemy_hp <= 0) {
          Battle::BattleAction action;
          action.type = Battle::kBattleAction_MonsterDead;
          action.apply_on_enemy = true;
          battle->actions_.push_back(action);
        }

        break;
    }

    battle->client_->removeTheFirstAttackReturn();
    index++;
  }
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << battle->actions_.size() << std::endl;

  context->SetState(new BattleCycleState());
}

void BattleCycleState::Handle(BattleContext *context, Battle *battle) {
  context->SetTransition(true);
  if (battle->actions_.empty()) {
    context->SetState(new UserInputState());
  } else {
    switch (battle->actions_.front().type) {
      case Battle::kBattleAction_Fight:
        context->SetState(new FightState());
        break;
      case Battle::kBattleAction_CallMonster:
        context->SetState(new CallMonsterState());
        break;
      case Battle::kBattleAction_End:
        context->SetState(new EndState());
        break;
      case Battle::kBattleAction_Run:
        context->SetState(new EscapeState());
        break;
      case Battle::kBattleAction_MonsterDead:
        context->SetState(new MonsterDeadState());
        break;
      default:
        break;
    }
  }
}

void EscapeState::Handle(BattleContext *context, Battle *battle) {
  context->SetTransition(true);
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  if (action.success) {
    Battle::BattleAction end;
    end.type = Battle::kBattleAction_End;

    battle->actions_.push_back(end);
  }

  context->SetState(new BattleCycleState());
}

void FightState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  CatchChallenger::ApplyOn apply_on = action.apply_on;
  bool apply_on_enemy = action.apply_on_enemy;

  // attack animation
  uint32_t attack_id = action.id;
  auto animation = Sequence::Create(
      Animation::Create(Utils::GetSkillAnimation(attack_id), 75),
      CallFunc::Create([battle, apply_on_enemy, context]() {
        battle->attack_->SetVisible(false);
        auto damage = Sequence::Create(
            Blink::Create(100, 10),
            CallFunc::Create([context]() { context->Handle(); }), nullptr);
        if (apply_on_enemy) {
          battle->enemy_->RunAction(damage, true);
        } else {
          battle->player_->RunAction(damage, true);
        }
      }),
      nullptr);

  if (apply_on_enemy) {
    battle->attack_->SetSize(battle->enemy_->Width(), battle->enemy_->Height());
    battle->attack_->SetPos(battle->enemy_->X(), battle->enemy_->Y());
  } else {
    battle->attack_->SetSize(battle->player_->Width(),
                             battle->player_->Height());
    battle->attack_->SetPos(battle->player_->X(), battle->player_->Y());
  }

  battle->attack_->SetVisible(true);
  battle->attack_->RunAction(animation, true);

  context->SetState(new BattleCycleState());
}

void ChooseMonsterState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  battle->ShowMonsterDialog(true);
}

void MonsterDeadState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  if (action.apply_on_enemy) {
    battle->ShowStatusMessage(
        QObject::tr("Enemy %1 fainted!").arg(battle->enemy_status_->Name()),
        false, true);
    context->SetState(new UpdateMonsterStatState());
    battle->enemy_->RunAction(
        Sequence::Create(MoveTo::Create(1500, battle->enemy_out_),
                         CallFunc::Create([context]() { context->Handle(); }),
                         nullptr),
        true);
  } else {
    if (battle->client_->haveAnotherMonsterOnThePlayerToFight()) {
      context->SetState(new ChooseMonsterState());
    }
    battle->player_->RunAction(
        Sequence::Create(MoveTo::Create(1500, battle->player_out_),
                         CallFunc::Create([context]() { context->Handle(); }),
                         nullptr),
        true);
  }
}

void UpdateMonsterStatState::Handle(BattleContext *context, Battle *battle) {
  if (battle->battleType != Battle::BattleType_Wild &&
      battle->client_->haveBattleOtherMonster()) {
    context->SetState(new BattleBehaviorState());
    battle->ProcessActions();
    return;
  }
  battle->player_->RunAction(
      Sequence::Create(Delay::Create(500),
                       CallFunc::Create([context]() { context->Handle(); }),
                       nullptr),
      true);
  context->SetState(new EndState());
}

void EndState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  battle->Win();
}
