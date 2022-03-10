// Copyright 2021 CatchChallenger
#include "BattleStates.hpp"

#include <QObject>
#include <iostream>

#include "../../../general/base/GeneralStructures.hpp"
#include "../../action/CallFunc.hpp"
#include "../../action/MoveTo.hpp"
#include "../../action/Sequence.hpp"
#include "ActionBar.hpp"
#include "Battle.hpp"

using Scenes::ActionBar;
using Scenes::Battle;
using Scenes::BattleBehaviorState;
using Scenes::BattleContext;
using Scenes::BattleState;
using Scenes::CallMonsterState;
using Scenes::EndState;
using Scenes::EscapeState;
using Scenes::FightState;
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

void BattleContext::SetState(BattleState *state) {
  state_ = state;
}

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
  battle->player_background_->SetVisible(false);

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
  battle->resetPosition(true, false, true);
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
      auto bounding = battle->BoundingRect();
      qreal player_x = bounding.width() * 0.1;
      qreal player_y = bounding.height() - battle->player_->Height() + 50;
      battle->player_->RunAction(
          Sequence::Create(MoveTo::Create(1500, player_x, player_y),
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

  // current monster
  // player_background_->SetVisible(false);
  // player_name_->SetText(QString::fromStdString(
  // QtDatapackClientLoader::datapackLoader->get_monsterExtra()
  //.at(fight_monster->monster)
  //.name));
  // Monster::Stat fight_stat =
  // client_->getStat(CommonDatapack::commonDatapack.get_monsters().at(
  // fight_monster->monster),
  // fight_monster->level);
  // player_hp_bar_->SetMaximum(fight_stat.hp);
  // player_hp_bar_->SetValue(fight_monster->hp);
  // const Monster &monster_info =
  // CommonDatapack::commonDatapack.get_monsters().at(
  // fight_monster->monster);
  // int level_to_xp = monster_info.level_to_xp.at(fight_monster->level - 1);
  // player_exp_bar_->SetMaximum(level_to_xp);
  // player_exp_bar_->SetValue(fight_monster->remaining_xp);
  //// do the buff, todo the remake
  //{
  // player_buff_->Clear();
  // unsigned int index = 0;
  // while (index < fight_monster->buffs.size()) {
  // const PlayerBuff &buff_effect = fight_monster->buffs.at(index);
  // auto item = Sprite::Create();
  // if (QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra()
  //.find(buff_effect.buff) ==
  // QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra()
  //.cend()) {
  // item->SetToolTip(tr("Unknown buff"));
  // item->SetPixmap(QPixmap(":/CC/images/interface/buff.png"));
  //} else {
  // item->SetPixmap(QtDatapackClientLoader::datapackLoader
  //->getMonsterBuffExtra(buff_effect.buff)
  //.icon);
  // if (buff_effect.level <= 1) {
  // item->SetToolTip(QString::fromStdString(
  // QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra()
  //.at(buff_effect.buff)
  //.name));
  //} else {
  // item->SetToolTip(tr("%1 at level %2")
  //.arg(QString::fromStdString(
  // QtDatapackClientLoader::datapackLoader
  //->get_monsterBuffsExtra()
  //.at(buff_effect.buff)
  //.name))
  //.arg(buff_effect.level));
  //}
  // item->SetToolTip(
  // item->ToolTip() + "\n" +
  // QString::fromStdString(QtDatapackClientLoader::datapackLoader
  //->get_monsterBuffsExtra()
  //.at(buff_effect.buff)
  //.description));
  //}
  // item->SetData(
  // 99,
  // buff_effect.buff);  // to prevent duplicate buff, because
  //// add can be to re-enable an already
  //// enable buff (for larger period then)
  // player_buff_->AddItem(item);
  // index++;
  //}
  //}
}

void UserInputState::Handle(BattleContext *context, Battle *battle) {
  battle->player_status_->SetVisible(true);
  battle->message_bar_->SetVisible(false);
  battle->action_bar_->SetVisible(true);

  context->SetState(new BattleBehaviorState());
}

void BattleBehaviorState::Handle(BattleContext *context, Battle *battle) {
  context->SetTransition(true);
  if (battle->client_->getAttackReturnList().empty()) {
    context->SetState(new UserInputState());
  } else {
    battle->action_chained = battle->client_->getAttackReturnList();
    if (battle->player_action.type != Battle::kBattleAction_None) {
      battle->player_action.type = Battle::kBattleAction_None;
    }
  }
}

void EscapeState::Handle(BattleContext *context, Battle *battle) {}
