// Copyright 2021 CatchChallenger
#include "BattleStates.hpp"

#include <QObject>
#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include "../../action/Animation.hpp"
#include "../../action/Blink.hpp"
#include "../../action/CallFunc.hpp"
#include "../../action/MoveTo.hpp"
#include "../../action/Sequence.hpp"
#include "../../core/Logger.hpp"
#include "../../core/SceneManager.hpp"
#include "../../entities/ActionUtils.hpp"
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
using Scenes::LoseState;
using Scenes::MonsterDeadState;
using Scenes::UpdateMonsterStatState;
using Scenes::UseItemState;
using Scenes::UserInputState;
using Scenes::UseTrapState;
using Scenes::WildPresentationState;
using Scenes::WinState;

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
  auto action_bar = battle->action_bar_;
  auto player_status = battle->player_status_;
  auto enemy_status = battle->enemy_status_;
  auto client = battle->client_;
  auto player = battle->player_;
  auto enemy = battle->enemy_;

  battle->battleStep = Battle::BattleStep_PresentationMonster;
  battle->battleType = Battle::BattleType_Wild;

  action_bar->CanRun(true);
  action_bar->SetVisible(false);
  action_bar->ShowSkills(false);
  battle->ConfigureFighters();
  battle->ConfigureCommons();
  player_status->SetVisible(false);

  if (!client->isInFight()) {
    // emit error("is in fight but without monster");
    // return false;
  }
  battle->ConfigureBattleground();

  enemy->SetPos(battle->enemy_in_);
  auto other_monster = client->getOtherMonster();
  if (other_monster != NULL) {
    enemy->SetPixmap(QtDatapackClientLoader::datapackLoader
                         ->getMonsterExtra(other_monster->monster)
                         .front.scaled(400, 400));
    // other monster
    enemy_status->SetVisible(true);
    enemy_status->SetMonster(other_monster);
  }

  player->SetPos(battle->player_out_);
  enemy->SetPos(battle->enemy_in_);

  battle->PrintError(
      __FILE__, __LINE__,
      QStringLiteral("You are in front of monster id: %1 level: %2 with hp: %3")
          .arg(other_monster->monster)
          .arg(other_monster->level)
          .arg(other_monster->hp));

  battle->ShowStatusMessage(
      QStringLiteral("Wild %1 appeared")
          .arg(QString::fromStdString(
              QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                  .at(other_monster->monster)
                  .name)),
      true, false);

  Battle::BattleAction action;
  action.type = Battle::kBattleAction_CallMonster;
  action.apply_on_enemy = false;
  battle->actions_.push_back(action);

  context->SetState(new CallMonsterState());
}

void CallMonsterState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  auto client = battle->client_;
  auto player = battle->player_;
  auto enemy = battle->enemy_;
  auto action_bar = battle->action_bar_;
  std::string monster_name;

  if (action.apply_on_enemy) {
    std::vector<Action *> actions;

    // Verify if the monster is in the screen
    if (enemy->X() < battle->Width()) {
      actions.push_back(MoveTo::Create(500, battle->enemy_out_));
    }

    actions.push_back(CallFunc::Create([battle]() {
      auto monster = battle->client_->getOtherMonster();
      auto enemy = battle->enemy_;
      enemy->SetPixmap(QtDatapackClientLoader::datapackLoader
                           ->getMonsterExtra(monster->monster)
                           .front.scaled(400, 400));
      auto name = QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(monster->monster)
                      .name;
      battle->enemy_status_->SetMonster(monster);
      battle->ShowStatusMessage(
          QObject::tr("Enemy call %1!").arg(QString::fromStdString(name)),
          false, true);
    }));

    actions.push_back(MoveTo::Create(1000, battle->enemy_in_));

    actions.push_back(CallFunc::Create([context, battle]() {
      battle->enemy_status_->SetVisible(true);
      context->Handle();
    }));

    enemy->RunAction(Sequence::Create(actions), true);
  } else {
    std::vector<Action *> actions;

    if (player->X() < 0) {
      actions.push_back(MoveTo::Create(500, battle->player_out_));
    }
    actions.push_back(CallFunc::Create([battle]() {
      std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< "asdasd" << std::endl;
      auto monster = battle->client_->getCurrentMonster();
      auto player = battle->player_;
      player->SetPixmap(QtDatapackClientLoader::datapackLoader
                            ->getMonsterExtra(monster->monster)
                            .front.scaled(400, 400));
      auto name = QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(monster->monster)
                      .name;
      battle->player_status_->SetMonster(monster);
      battle->action_bar_->SetMonster(monster);
      battle->ShowStatusMessage(
          QObject::tr("Go %1!").arg(QString::fromStdString(name)), false, true);
    }));

    actions.push_back(MoveTo::Create(1000, battle->player_in_));

    actions.push_back(CallFunc::Create([context, battle]() {
      battle->player_status_->SetVisible(true);
      context->Handle();
    }));

    player->RunAction(Sequence::Create(actions), true);
  }

  context->SetState(new BattleCycleState());
}

void UserInputState::Handle(BattleContext *context, Battle *battle) {
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
      case CatchChallenger::Skill::AttackReturnCase_ItemUsage:
        std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                  << "asdasd" << std::endl;
        break;
      case CatchChallenger::Skill::AttackReturnCase_MonsterChange:
        Battle::BattleAction action;
        action.type = Battle::kBattleAction_CallMonster;
        action.apply_on_enemy = true;
        battle->actions_.push_back(action);
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
    std::string state;
    switch (battle->actions_.front().type) {
      case Battle::kBattleAction_Fight:
        context->SetState(new FightState());
        state = "Fight";
        break;
      case Battle::kBattleAction_CallMonster:
        context->SetState(new CallMonsterState());
        state = "CallMonster";
        break;
      case Battle::kBattleAction_End:
        context->SetState(new EndState());
        state = "End";
        break;
      case Battle::kBattleAction_Run:
        context->SetState(new EscapeState());
        state = "Run";
        break;
      case Battle::kBattleAction_MonsterDead:
        context->SetState(new MonsterDeadState());
        state = "MonsterDead";
        break;
      case Battle::kBattleAction_UseTrap:
        context->SetState(new UseTrapState());
        state = "UseTrap";
        break;
      default:
        break;
    }
    std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] " << state
              << std::endl;
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

  /*
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
  */
  battle->RunAction(
      ActionUtils::WaitAndThen(1000, [context]() { context->Handle(); }), true);

  context->SetState(new BattleCycleState());
}

void ChooseMonsterState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  battle->ShowMonsterDialog(false);
}

void MonsterDeadState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  if (action.apply_on_enemy) {
    battle->enemy_status_->SetVisible(false);
    battle->ShowStatusMessage(
        QObject::tr("Enemy %1 fainted!").arg(battle->enemy_status_->Name()),
        false, true);
    context->SetState(new UpdateMonsterStatState());
    // battle->enemy_->RunAction(
    // Sequence::Create(MoveTo::Create(1500, battle->enemy_out_),
    // CallFunc::Create([context]() { context->Handle(); }),
    // nullptr),
    // true);
    battle->RunAction(
        ActionUtils::WaitAndThen(1000, [context]() { context->Handle(); }),
        true);
  } else {
    battle->player_status_->SetVisible(false);
    if (battle->client_->haveAnotherMonsterOnThePlayerToFight()) {
      context->SetState(new ChooseMonsterState());
    } else {
      context->SetState(new LoseState());
    }
    battle->player_->RunAction(
        Sequence::Create(MoveTo::Create(1500, battle->player_out_),
                         CallFunc::Create([context]() { context->Handle(); }),
                         nullptr),
        true);
  }
}

void UseTrapState::Handle(BattleContext *context, Battle *battle) {
  auto action = battle->actions_.front();
  battle->actions_.erase(battle->actions_.begin());

  auto client = battle->client_;

  if (action.success) {
    if (client->getPlayerMonster().size() >=
        CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters) {
      CatchChallenger::Player_private_and_public_informations &informations =
          client->get_player_informations();
      if (informations.warehouse_playerMonster.size() >=
          CommonSettingsCommon::commonSettingsCommon
              .maxWarehousePlayerMonsters) {
        battle->ShowMessageDialog(
            QObject::tr("Error"),
            QObject::tr("You have already the maximum number "
                        "of monster into you warehouse"));
        return;
      }
      informations.warehouse_playerMonster.push_back(
          client->playerMonster_catchInProgress.front());
    } else {
      Logger::Log(QStringLiteral("mob: %1").arg(
          client->playerMonster_catchInProgress.front().id));
      client->addPlayerMonster(client->playerMonster_catchInProgress.front());
    }
    context->SetState(new EndState());
  } else {
    context->SetState(new BattleCycleState());
  }
  client->playerMonster_catchInProgress.erase(
      client->playerMonster_catchInProgress.cbegin());

  battle->RunAction(
      ActionUtils::WaitAndThen(1000, [context]() { context->Handle(); }), true);
}

void UpdateMonsterStatState::Handle(BattleContext *context, Battle *battle) {
  if (battle->battleType != Battle::BattleType_Wild &&
      battle->client_->haveBattleOtherMonster()) {
    context->SetState(new BattleBehaviorState());
    battle->ProcessActions();
    return;
  }
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << "updatemonsterstat" << std::endl;
  battle->RunAction(
      ActionUtils::WaitAndThen(1000, [context]() { context->Handle(); }), true);
  context->SetState(new WinState());
}

void WinState::Handle(BattleContext *context, Battle *battle) {
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << "Wind" << std::endl;
  battle->RunAction(
      ActionUtils::WaitAndThen(500, [context]() { context->Handle(); }), true);
  context->SetState(new EndState());
}

void LoseState::Handle(BattleContext *context, Battle *battle) {
  auto client = battle->client_;
  client->healAllMonsters();
  battle->RunAction(
      ActionUtils::WaitAndThen(500, [context]() { context->Handle(); }), true);
  context->SetState(new EndState());
}

void EndState::Handle(BattleContext *context, Battle *battle) {
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << "endstate" << std::endl;

  auto fight_id = battle->fightId;
  auto client = battle->client_;
  auto battle_type = battle->battleType;

  client->fightFinished();
  switch (battle_type) {
    case Battle::BattleType_Bot: {
      if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_botFights()
              .find(fight_id) ==
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_botFights()
              .cend()) {
        // emit error("fight id not found at collision");
        return;
      }
      auto cash =
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_botFights()
              .at(fight_id)
              .cash;
      auto information = client->get_player_informations();
      information.cash += cash;
      client->addBeatenBotFight(fight_id);
      /*
      if (botFightList.empty()) {
        emit error("battle info not found at collision");
        return;
      }
      botFightList.erase(botFightList.cbegin());
      */
      fight_id = 0;
    } break;
    case Battle::BattleType_OtherPlayer:
      /*
      if (battleInformationsList.empty()) {
        emit error("battle info not found at collision");
        return;
      }
      battleInformationsList.erase(battleInformationsList.cbegin());
      */
      break;
    default:
      break;
  }
  // CheckEvolution();
  SceneManager::GetInstance()->PopScene();
  context->SetState(nullptr);
}

