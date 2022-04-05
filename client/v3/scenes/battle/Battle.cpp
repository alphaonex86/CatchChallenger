// Copyright 2021 <CatchChallenger>
#include "Battle.hpp"

#include <QImageReader>
#include <QObject>
#include <iostream>
#include <string>
#include <vector>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/MoveOnTheMap.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Globals.hpp"
#include "../../action/Blink.hpp"
#include "../../action/CallFunc.hpp"
#include "../../action/Sequence.hpp"
#include "../../action/Spawn.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/EventManager.hpp"
#include "../../core/Logger.hpp"
#include "../../core/SceneManager.hpp"
#include "../../entities/Utils.hpp"
#include "BattleActions.hpp"

using Scenes::Battle;
using Scenes::BattleActions;
using Scenes::BattleContext;
using namespace CatchChallenger;
using CatchChallenger::PublicPlayerMonster;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

Battle::Battle() : Scene(nullptr) {
  labelFightBackground = Sprite::Create(this);
  labelFightBackgroundPix =
      QPixmap(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->getDatapackPath()) +
              "/map/fight/grass/background.png");
  labelFightForeground = Sprite::Create(this);
  labelFightForegroundPix =
      QPixmap(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->getDatapackPath()) +
              "/map/fight/grass/foreground.png");

  labelFightPlateformBottom = Sprite::Create(this);
  labelFightPlateformTop = Sprite::Create(this);

  player_ = Sprite::Create(this);
  enemy_ = Sprite::Create(this);
  enemy_->SetSize(400, 400);
  attack_ = Sprite::Create(this);

  escape = false;
  escapeSuccess = false;

  linked_ = UI::LinkedDialog::Create(false);
  monsters_ = MonsterSwap::Create();
  //monster->SetOnSelectMonster(std::bind(&Battle::OnMonsterSelect, this, _1));
  //linked_->AddItem(monster, "monsters");
  auto inventory = Inventory::Create();
  inventory->SetVar(Inventory::ObjectFilter_UseInFight, true);
  inventory->SetOnUseItem(std::bind(&Battle::UseBagItem, this, _1, _2, _3));
  linked_->AddItem(inventory, "inventory");

  action_bar_ = ActionBar::Create(this);
  action_bar_->SetOnActionClick([this](const ActionBar::ActionType &type) {
    action_type = type;
    if (type == ActionBar::kActionType_Monster) {
      OnActionMonsterClick(nullptr);
    } else if (type == ActionBar::kActionType_Bag) {
      OnActionBagClick(nullptr);
    } else if (type == ActionBar::kActionType_Run) {
      OnActionEscapeClick(nullptr);
    }
  });
  action_bar_->SetOnSkillClick(
      std::bind(&Battle::OnSkillClick, this, _1, _2, _3));
  player_status_ = StatusCard::Create(this);
  enemy_status_ = StatusCard::Create(this);
  message_bar_ = UI::MessageBar::Create(this);
  message_bar_->SetOnHide([&]() { battle_context_->Handle(); });

  trap_ = nullptr;

  wait_timer_.setSingleShot(true);
  wait_timer_.setInterval(1000);
  battle_context_ = BattleContext::Create(this);

  if (!connect(&wait_timer_, &QTimer::timeout, this,
               &Battle::OnWaitEngineMoveSlot))
    abort();

  ChangeLanguage();
  SetVariables();
}

Battle::~Battle() {
  delete linked_;
  linked_ = nullptr;
}

Battle *Battle::Create() { return new (std::nothrow) Battle(); }

void Battle::OnEnter() {
  Scene::OnEnter();

  action_bar_->RegisterEvents();
  message_bar_->RegisterEvents();
}

void Battle::OnExit() {
  action_bar_->UnRegisterEvents();
  message_bar_->UnRegisterEvents();
}

void Battle::SetVariables() {
  connexionManager = ConnectionManager::GetInstance();
  client_ = connexionManager->client;
  fightEngine = static_cast<CommonFightEngine *>(client_);

  if (!connect(client_, &CatchChallenger::Api_client_real::QtmonsterCatch, this,
               &Battle::MonsterCatchSlot))
    abort();
  if (!connect(client_, &CatchChallenger::Api_client_real::QtteleportTo, this,
               &Battle::TeleportToSlot))
    abort();
}

QPixmap Battle::GetBackSkin(const uint32_t &skinId) {
  /// \todo merge it cache string + id
  const std::vector<std::string> &skinFolderList =
      QtDatapackClientLoader::datapackLoader->get_skins();

  std::string skin;
  // front image
  if (skinId < (uint32_t)skinFolderList.size()) {
    skin = QtDatapackClientLoader::datapackLoader->getSkinPath(
        skinFolderList.at(skinId), "back");
  } else {
    skin = ":/CC/images/player_default/back.png";
  }
  return QPixmap(QString::fromStdString(skin));
}

void Battle::ChangeLanguage() {}

void Battle::BotFightInitialize(Map_client *map, const uint8_t &x,
                                const uint8_t &y, const uint16_t &fight_id) {
  if (CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(
          fight_id) ==
      CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights()
          .cend()) {
    // emit error("fight id not found at collision");
    return;
  }
  action_bar_->CanRun(false);
  // ShowActionBarTab(0);
  ConfigureFighters();
  ConfigureCommons();

  player_->SetVisible(false);
  enemy_->SetVisible(false);

  zone_collision_ = MoveOnTheMap::getZoneCollision(*map, x, y);
  ConfigureBattleground();
  botFightList.push_back(fightId);
  if (botFightList.size() > 1 || !battleInformationsList.empty()) return;
  botFightFull(fight_id);
}

void Battle::botFightFull(const uint16_t &fight_id) {
  this->fightId = fight_id;
  RunAction(both_delay_);
}

void Battle::prepareFight() {
  escape = false;
  escapeSuccess = false;
}

// Initialize the battle for wild encounter
bool Battle::WildFightInitialize(Map_client *map, const uint8_t &x,
                                 const uint8_t &y) {
  wait_timer_.stop();

  zone_collision_ = MoveOnTheMap::getZoneCollision(*map, x, y);
  battle_context_->Restart(Scenes::BattleContext::kStartState_Wild);
  return true;
}

void Battle::init_other_monster_display() { updateOtherMonsterInformation(); }

void Battle::TeleportToSlot(const uint32_t &mapId, const uint16_t &x,
                            const uint16_t &y, const Direction &direction) {
  Q_UNUSED(mapId);
  Q_UNUSED(x);
  Q_UNUSED(y);
  Q_UNUSED(direction);
  if (client_->currentMonsterIsKO() &&
      !client_->haveAnotherMonsterOnThePlayerToFight()) {
    // then is dead, is teleported to the last rescue point
    qDebug() << "tp on loose: " << fightTimerFinish;
    loose();
  } else {
    qDebug() << "normal tp";
  }
}

void Battle::updateCurrentMonsterInformationXp() {
  PlayerMonster *monster = client_->getCurrentMonster();
  if (monster == NULL) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "NULL pointer at updateCurrentMonsterInformation()");
    return;
  }
  //player_exp_bar_->SetValue(monster->remaining_xp);
  const Monster &monsterInformations =
      CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
  uint32_t maxXp = monsterInformations.level_to_xp.at(monster->level - 1);
  //player_exp_bar_->SetMaximum(maxXp);
}

void Battle::updateCurrentMonsterInformation() {
  if (!client_->getAbleToFight()) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "Try update the monster when have not any ready monster");
    return;
  }
  PlayerMonster *monster = client_->getCurrentMonster();
  if (monster == NULL) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "NULL pointer at updateCurrentMonsterInformation()");
    return;
  }
#ifdef CATCHCHALLENGER_DEBUG_FIGHT
  qDebug() << "Now visible monster have hp : " << monster->hp;
#endif
  player_->SetPixmap(
      QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster->monster)
          .back.scaled(400, 400));
  player_monster_lvl_ = monster->level;
  action_bar_->SetMonster(monster);
  //UpdateAttackSkills();
}

void Battle::updateOtherMonsterInformation() {
  if (!client_->isInFight()) {
    PrintError(__FILE__, __LINE__, "but have not other monter");
    return;
  }
  enemy_->SetPos(enemy_in_);
  PublicPlayerMonster *otherMonster = client_->getOtherMonster();
  if (otherMonster != NULL) {
    enemy_->SetPixmap(QtDatapackClientLoader::datapackLoader
                          ->getMonsterExtra(otherMonster->monster)
                          .front.scaled(400, 400));
    // other monster
    enemy_status_->SetVisible(true);
    enemy_status_->SetMonster(otherMonster);
  } else {
    PrintError(__FILE__, __LINE__, "Unable to load the other monster");
    return;
  }
}

void Battle::loose() {
  PrintError(__FILE__, __LINE__, "loose");
  //#ifdef CATCHCHALLENGER_EXTRA_CHECK
  // PublicPlayerMonster *currentMonster = client_->getCurrentMonster();
  // if (currentMonster != NULL) {
  // if ((int)currentMonster->hp != player_hp_bar_->Value()) {
  // std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
  //<< QStringLiteral(
  //"Current monster damage don't match with the internal "
  //"value (loose && currentMonster): %1!=%2")
  //.arg(currentMonster->hp)
  //.arg(player_hp_bar_->Value())
  //.toStdString()
  //<< std::endl;
  // return;
  //}
  //}
  // PublicPlayerMonster *otherMonster = client_->getOtherMonster();
  // if (otherMonster != NULL) {
  // if ((int)otherMonster->hp != enemy_hp_bar_->Value()) {
  // std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
  //<< QStringLiteral(
  //"Current monster damage don't match with the internal "
  //"value (loose && otherMonster): %1!=%2")
  //.arg(otherMonster->hp)
  //.arg(enemy_hp_bar_->Value())
  //.toStdString()
  //<< std::endl;
  // return;
  //}
  //}
  //#endif
  if (CommonDatapack::commonDatapack.get_monsters().empty()) return;
  client_->healAllMonsters();
  client_->fightFinished();
  fightTimerFinish = false;
  escape = false;
  doNextActionStep = DoNextActionStep_Start;
  switch (battleType) {
    case BattleType_Bot:
      if (botFightList.empty()) {
        std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                  << "battle info not found at collision" << std::endl;
        return;
      }
      botFightList.erase(botFightList.cbegin());
      fightId = 0;
      break;
    case BattleType_OtherPlayer:
      if (battleInformationsList.empty()) {
        std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                  << "battle info not found at collision" << std::endl;
        return;
      }
      battleInformationsList.erase(battleInformationsList.cbegin());
      break;
    default:
      break;
  }
  if (!battleInformationsList.empty()) {
    const BattleInformations &battleInformations =
        battleInformationsList.front();
    battleInformationsList.erase(battleInformationsList.cbegin());
    battleAcceptedByOtherFull(battleInformations);
  } else if (!botFightList.empty()) {
    uint16_t fightId = botFightList.front();
    botFightList.erase(botFightList.cbegin());
    botFightFull(fightId);
  }

  CheckEvolution();
  Finished();
}

void Battle::resetPosition(bool outOfScreen, bool topMonster,
                           bool bottomMonster) {
  if (outOfScreen) {
    if (bottomMonster) {
      player_->UseState(kOutScreen);
    }
    if (topMonster) {
      enemy_->UseState(kOutScreen);
    }
  } else {
    if (bottomMonster) {
      player_->UseState(kReadyForBattle);
    }
    if (topMonster) {
      enemy_->UseState(kReadyForBattle);
    }
  }
}

/*
void Battle::tradeAddTradeMonster(const PlayerMonster &monster)
{
    tradeOtherMonsters.push_back(monster);
    QString genderString;
    switch(monster.gender)
    {
        case 0x01:
        genderString=tr("Male");
        break;
        case 0x02:
        genderString=tr("Female");
        break;
        default:
        genderString=tr("Unknown");
        break;
    }
    QListWidgetItem *item=new QListWidgetItem();
    if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(monster.monster)!=
            QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).front);
        item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name));
        item->setToolTip(QStringLiteral("Level: %1, Gender:
%2").arg(monster.level).arg(genderString));
    }
    else
    {
        item->setIcon(QIcon(":/CC/images/monsters/default/front.png"));
        item->setText(tr("Unknown"));
        item->setToolTip(QStringLiteral("Level: %1, Gender:
%2").arg(monster.level).arg(genderString));
    }
    tradeOtherMonsters->addItem(item);
}

//learn
bool Battle::showLearnSkillByPosition(const uint8_t &monsterPosition)
{
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    learnAttackList->clear();
    attack_to_learn_graphical.clear();
    std::vector<PlayerMonster>
playerMonster=connexionManager->client->getPlayerMonster();
    //get the right monster
    QHash<uint16_t,uint8_t> skillToDisplay;
    unsigned int index=monsterPosition;
    PlayerMonster monster=playerMonster.at(index);
    learnMonster->setPixmap(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).front.scaled(160,160));
    if(CommonSettingsServer::commonSettingsServer.useSP)
    {
        learnSP->setVisible(true);
        learnSP->setText(tr("SP: %1").arg(monster.sp));
    }
    else
        learnSP->setVisible(false);
    if(Ultimate::ultimate.isUltimate())
        learnInfo->setText(tr("<b>%1</b><br />Level %2").arg(
                               QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name))
                           .arg(monster.level));
    unsigned int sub_index=0;
    while(sub_index<CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size())
    {
        Monster::AttackToLearn
learn=CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel<=monster.level)
        {
            unsigned int sub_index2=0;
            while(sub_index2<monster.skills.size())
            {
                const PlayerMonster::PlayerSkill
&skill=monster.skills.at(sub_index2); if(skill.skill==learn.learnSkill) break;
                sub_index2++;
            }
            if(
                    //if skill not found
                    (sub_index2==monster.skills.size() &&
learn.learnSkillLevel==1)
                    ||
                    //if skill already found and need level up
                    (sub_index2<monster.skills.size() &&
(monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
            )
            {
                if(skillToDisplay.contains(learn.learnSkill))
                {
                    if(skillToDisplay.value(learn.learnSkill)>learn.learnSkillLevel)
                        skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                }
                else
                    skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
            }
        }
        sub_index++;
    }
    QHashIterator<uint16_t,uint8_t> i(skillToDisplay);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item=new QListWidgetItem();
        const uint32_t &level=i.value();
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            if(level<=1)
                item->setText(tr("%1\nSP cost: %2")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name))
                            .arg(CommonDatapack::commonDatapack.monsterSkills.at(i.key()).level.at(i.value()-1).sp_to_learn)
                        );
            else
                item->setText(tr("%1 level %2\nSP cost: %3")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name))
                            .arg(level)
                            .arg(CommonDatapack::commonDatapack.monsterSkills.at(i.key()).level.at(i.value()-1).sp_to_learn)
                        );
        }
        else
        {
            if(level<=1)
                item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name));
            else
                item->setText(tr("%1 level %2")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name)
                            .arg(level)
                        ));
        }
        if(CommonSettingsServer::commonSettingsServer.useSP &&
CommonDatapack::commonDatapack.monsterSkills.at(i.key()).level.at(i.value()-1).sp_to_learn>monster.sp)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
            item->setToolTip(tr("You need more sp"));
        }
        attack_to_learn_graphical[item]=i.key();
        learnAttackList->addItem(item);
    }
    if(attack_to_learn_graphical.empty())
        learnDescription->setText(tr("No more attack to learn"));
    else
        learnDescription->setText(tr("Select attack to learn"));
    return true;
}

void Battle::sendBattleReturn(const std::vector<Skill::AttackReturn>
&attackReturn)
{
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster *
currentMonster=connexionManager->client->getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster was hp" << currentMonster->hp << "and
buff" << currentMonster->buffs.size(); const PublicPlayerMonster *
otherMonster=connexionManager->client->getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster was hp" << otherMonster->hp << "and buff"
<< otherMonster->buffs.size();
    }
    #endif
    connexionManager->client->addAndApplyAttackReturnList(attackReturn);
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster *
currentMonster=connexionManager->client->getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster have hp" << currentMonster->hp << "and
buff" << currentMonster->buffs.size(); const PublicPlayerMonster *
otherMonster=connexionManager->client->getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster have hp" << otherMonster->hp << "and buff"
<< otherMonster->buffs.size();
    }
    #endif
    doNextAction();
}*/

/*
void Battle::battleAcceptedByOther(const std::string &pseudo,const uint8_t
&skinId,const std::vector<uint8_t> &stat, const uint8_t &monsterPlace,const
PublicPlayerMonster &publicPlayerMonster)
{
    BattleInformations battleInformations;
    battleInformations.pseudo=pseudo;
    battleInformations.skinId=skinId;
    battleInformations.stat=stat;
    battleInformations.monsterPlace=monsterPlace;
    battleInformations.publicPlayerMonster=publicPlayerMonster;
    battleInformationsList.push_back(battleInformations);
    if(battleInformationsList.size()>1 || !botFightList.empty())
        return;
    battleAcceptedByOtherFull(battleInformations);
}*/

void Battle::battleAcceptedByOtherFull(
    const BattleInformations &battleInformations) {
  if (client_->isInFight()) {
    PrintError(__FILE__, __LINE__, "already in fight");
    client_->battleRefused();
    return;
  }
  prepareFight();
  battleType = BattleType_OtherPlayer;

  QPixmap otherFrontImage = QPixmap(QString::fromStdString(
      QtDatapackClientLoader::datapackLoader->getFrontSkinPath(
          battleInformations.skinId)));

  // reset the other player info
  enemy_->SetPixmap(otherFrontImage.scaled(160, 160));
  // battleOtherPseudo->setText(lastBattleQuery.first().first);
  player_->SetPixmap(playerBackImage.scaledToHeight(600));
  ShowStatusMessage(tr("%1 wish fight with you")
                        .arg(QString::fromStdString(battleInformations.pseudo)),
                    false, false);

  resetPosition(true, true, true);
  battleStep = BattleStep_Presentation;

  player_->RunAction(both_enter_);

  client_->setBattleMonster(battleInformations.stat,
                            battleInformations.monsterPlace,
                            battleInformations.publicPlayerMonster);

  // init_environement_display(map, map->getX(), map->getY());
}

/*void Battle::battleCanceledByOther(){
    connexionManager->client->fightFinished();
    stackedWidget->setCurrentWidget(page_map);
    showTip(tr("The other player have canceled the battle").toStdString());
    if(battleInformationsList.empty())
    {
        emit error("battle info not found at collision");
        return;
    }
    battleInformationsList.erase(battleInformationsList.cbegin());
    if(!battleInformationsList.empty())
    {
        const BattleInformations
&battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        const uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFightFull(fightId);
    }
}*/

void Battle::doNextAction() {
  // action_escape_->SetVisible(battleType == BattleType_Wild);
  if (escape) {
    doNextActionStep = DoNextActionStep_Start;
    if (!escapeSuccess) {  // the other attack
      escape = false;      // need be dropped to have text to escape
      fightTimerFinish = false;
      if (!client_->getAttackReturnList().empty()) {
        ShowStatusMessage(tr("You have failed to escape!"));
        return;
      } else {
        PublicPlayerMonster *otherMonster = client_->getOtherMonster();
        if (otherMonster == NULL) {
          // emit error("NULL pointer for other monster at doNextAction()");
          return;
        }
        qDebug() << "doNextAction(): escape fail but the wild monster can't "
                    "attack";
        ShowStatusMessage(
            tr("The wild %1 can't attack")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                        .at(otherMonster->monster)
                        .name)));
        return;
      }
    } else {
      qDebug() << "doNextAction(): escape success";
      Win();
    }
    return;  // useful to quit correctly
  }
  // apply the effect
  if (!client_->getAttackReturnList().empty()) {
    const Skill::AttackReturn returnAction =
        client_->getAttackReturnList().front();
    PublicPlayerMonster *otherMonster = client_->getOtherMonster();
    if (otherMonster) {
      qDebug() << "doNextAction(): apply the effect and display it";
      StartAttack();
    } else if (returnAction.publicPlayerMonster.monster !=
               0) {  // do the monster change
      if (!client_->addBattleMonster(returnAction.monsterPlace,
                                     returnAction.publicPlayerMonster))
        return;
      // sendBattleReturn(returnAction);:already added because the information
      // is into connexionManager->client->getAttackReturnList()
      init_other_monster_display();
      updateOtherMonsterInformation();
      resetPosition(true, true, false);
      battleStep = BattleStep_Middle;
      enemy_->RunAction(enemy_enter_);
    } else {
      newError("Internal bug", "No other monster to display attack");
    }
    return;
  }

  if (doNextActionStep == DoNextActionStep_Loose) {
    qDebug() << "doNextAction(): do step loose";
#ifdef DEBUG_CLIENT_BATTLE
    qDebug() << "doNextActionStep==DoNextActionStep_Loose, fightTimerFinish: "
             << fightTimerFinish;
#endif
    if (fightTimerFinish)
      loose();
    else
      fightTimerFinish = true;
    return;
  }

  if (CommonDatapack::commonDatapack.get_monsters().empty()) return;
  // if the current monster is KO
  if (client_->currentMonsterIsKO()) {
    if (CommonDatapack::commonDatapack.get_monsters().empty()) return;
    if (!client_->isInFight()) {
      if (CommonDatapack::commonDatapack.get_monsters().empty()) return;
      loose();
      return;
    }
    PrintError(__FILE__, __LINE__, "remplace KO current monster");
    PublicPlayerMonster *currentMonster = client_->getCurrentMonster();
    ShowStatusMessage(
        tr("Your %1 have lost!")
            .arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                    .at(currentMonster->monster)
                    .name)),
        false, false);
    doNextActionStep = DoNextActionStep_Start;
    player_->RunAction(player_dead_);
    return;
  }

  // if the other monster is KO
  if (client_->otherMonsterIsKO()) {
    ProcessEnemyMonsterKO();
    return;
  }

  // nothing to done, show the menu
  PrintError(__FILE__, __LINE__, "show the menu");
  // ShowActionBarTab(1);
  action_bar_->SetVisible(true);
#ifdef CATCHCHALLENGER_EXTRA_CHECK
  PublicPlayerMonster *currentMonster = client_->getCurrentMonster();
  if (currentMonster != NULL) {
    if (CommonDatapack::commonDatapack.get_monsters().find(
            currentMonster->monster) ==
        CommonDatapack::commonDatapack.get_monsters().cend()) {
      /* emit error(QStringLiteral("Current monster don't exists: %1")
                     .arg(currentMonster->monster)
                     .toStdString());*/
      return;
    }
  }
  PublicPlayerMonster *otherMonster = client_->getOtherMonster();
  if (otherMonster != NULL) {
    if (CommonDatapack::commonDatapack.get_monsters().find(
            otherMonster->monster) ==
        CommonDatapack::commonDatapack.get_monsters().cend()) {
      emit error(QStringLiteral("Current monster don't exists: %1")
                     .arg(otherMonster->monster)
                     .toStdString());
      return;
    }
  }
#endif
  return;
}

void Battle::error(std::string error) {
  qDebug() << QString::fromStdString(error);
  // emit newError(tr("Error with the protocol").toStdString(),error);
}

void Battle::ShowMessageDialog(const QString &title, const QString &message) {
  Globals::GetAlertDialog()->Show(title, message);
}

void Battle::ShowMonsterDialog(bool show_close) {
  AddChild(monsters_);
}

void Battle::ShowBagDialog() {
  linked_->SetCurrentItem("inventory");
  AddChild(linked_);
}

void Battle::UseBagItem(Inventory::ObjectType type, uint16_t item,
                        uint32_t quantity) {
  RemoveChild(linked_);

  const CatchChallenger::Player_private_and_public_informations
      &playerInformations = client_->get_player_informations_ro();
  if (playerInformations.warehouse_playerMonster.size() >=
      CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters) {
    ShowMessageDialog(tr("Error"), tr("You have already the maximum number of "
                                      "monster into you warehouse"));
    return;
  }
  if (playerInformations.items.find(item) == playerInformations.items.cend()) {
    qDebug() << "item id is not into the inventory";
    return;
  }
  if (playerInformations.items.at(item) < 1) {
    qDebug() << "item id have not the quantity";
    return;
  }
  // it's trap
  if (CommonDatapack::commonDatapack.get_items().trap.find(item) !=
          CommonDatapack::commonDatapack.get_items().trap.cend() &&
      client_->isInFightWithWild()) {
    UseTrap(item);
  } else {  // else it's to use on current monster
    const uint8_t &monsterPosition = client_->getCurrentSelectedMonsterNumber();
    if (client_->useObjectOnMonsterByPosition(item + 1, monsterPosition)) {
      client_->remove_to_inventory({{item, 1}});
      if (CommonDatapack::commonDatapack.get_items().monsterItemEffect.find(
              item) !=
          CommonDatapack::commonDatapack.get_items().monsterItemEffect.cend()) {
        client_->useObjectOnMonsterByPosition(item, monsterPosition + 1);
        ProcessActions();
      } else {
        error(tr("You have selected a buggy object").toStdString());
      }
    } else {
      ShowMessageDialog(tr("Warning"), tr("Can't be used now!"));
    }
  }
}

void Battle::ConfigureFighters() {
  auto bounding = BoundingRect();

  const auto &informations = client_->player_informations.public_informations;
  playerBackImage = GetBackSkin(informations.skinId);
  playerBackImage = playerBackImage.scaledToHeight(600);
  player_->SetPixmap(playerBackImage);

  player_in_ = QPointF(bounding.width() * 0.1, bounding.height() - player_->Height() + 200);
  player_out_ = QPointF(-playerBackImage.width(), bounding.height() - player_->Height() + 200);
  enemy_in_ = QPointF(bounding.width() - enemy_->Width() - 50, bounding.height() * 0.25);
  enemy_out_ = QPointF(bounding.width()- 50, bounding.height() * 0.25);
}

void Battle::ConfigureCommons() {
  action_bar_->SetVisible(false);
  player_status_->SetVisible(false);
  enemy_status_->SetVisible(false);

  // status_label_->SetText(QString());
  status_timeout_ = Sequence::Create(
      Delay::Create(1500),
      CallFunc::Create([this]() { battle_context_->Handle(); }), nullptr);
}

void Battle::OnSkillClick(uint8_t index, uint16_t skill_id, uint8_t endurance) {
  if (!client_->canDoFightAction()) {
    ShowMessageDialog(
        "Communication problem",
        tr("Sorry but the client wait more data from the server to do it"));
    return;
  }
  if (endurance <= 0 && !useTheRescueSkill) {
    ShowMessageDialog(tr("No endurance"),
                      tr("You have no more endurance to use this skill"));
    return;
  }

  client_->useSkill(skill_id);
  ProcessActions();
}

void Battle::ShowStatusMessage(const std::string &text, bool show_next_btn,
                               bool use_timeout) {
  message_bar_->SetMessage(text);
  if (use_timeout) {
    message_bar_->RunAction(Delay::Create(1500), true);
    message_bar_->ShowNext(false);
  } else {
    message_bar_->ShowNext(show_next_btn);
  }
  message_bar_->Show();
}

void Battle::ShowStatusMessage(const QString &text, bool show_next_btn,
                               bool use_timeout) {
  ShowStatusMessage(text.toStdString(), show_next_btn, use_timeout);
}

void Battle::OnMonsterSelect(uint8_t monster_index) {
  linked_->Close();
  // do copie here because the call of changeOfMonsterInFight apply the skill
  // / buff effect
  const PlayerMonster *tempMonster = client_->monsterByPosition(monster_index);
  if (tempMonster == NULL) {
    qDebug() << "Monster not found";
    return;
  }
  if (!client_->changeOfMonsterInFight(monster_index)) return;
  client_->changeOfMonsterInFightByPosition(monster_index);
  player_status_->SetVisible(false);


  BattleAction action;
  action.type = kBattleAction_CallMonster;
  action.success = true;
  actions_.push_back(action);

  ProcessActions();
}

void Battle::StartAttack() {
  PublicPlayerMonster *other_monster = client_->getOtherMonster();
  PublicPlayerMonster *current_monster = client_->getCurrentMonster();
  if (other_monster == NULL) {
    error(
        "StartAttack(): crash: unable to get the other monster to display "
        "an "
        "attack");
    doNextAction();
    return;
  }
  if (current_monster == NULL) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "StartAttack(): crash: unable to get the current monster");
    doNextAction();
    return;
  }

  const Skill::AttackReturn &attack_return = client_->getFirstAttackReturn();
  // get the life effect to display
  CatchChallenger::ApplyOn apply_on;
  if (!attack_return.lifeEffectMonster.empty()) {
    apply_on = attack_return.lifeEffectMonster.front().on;
  } else if (!attack_return.buffLifeEffectMonster.empty()) {
    apply_on = attack_return.buffLifeEffectMonster.front().on;
  } else if (!attack_return.addBuffEffectMonster.empty()) {
    apply_on = attack_return.addBuffEffectMonster.front().on;
  } else if (!attack_return.removeBuffEffectMonster.empty()) {
    apply_on = attack_return.removeBuffEffectMonster.front().on;
  } else {
    error(QStringLiteral(
              "StartAttack(): strange: nothing to display, "
              "lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): "
              "%2, addBuffEffectMonster.size(): %3, "
              "removeBuffEffectMonster.size(): %4, AttackReturnList: %5")
              .arg(attack_return.lifeEffectMonster.size())
              .arg(attack_return.buffLifeEffectMonster.size())
              .arg(attack_return.addBuffEffectMonster.size())
              .arg(attack_return.removeBuffEffectMonster.size())
              .arg(client_->getAttackReturnList().size())
              .toStdString());
    client_->removeTheFirstAttackReturn();
    // doNextAction();
    return;
  }

  bool apply_on_enemy = false;
  if (attack_return.doByTheCurrentMonster) {
    if ((apply_on == ApplyOn::ApplyOn_AloneEnemy) ||
        (apply_on == ApplyOn::ApplyOn_AllEnemy))
      apply_on_enemy = true;
  } else {
    if ((apply_on == ApplyOn::ApplyOn_Themself) ||
        (apply_on == ApplyOn::ApplyOn_AllAlly))
      apply_on_enemy = true;
  }

  // attack animation
  {
    uint32_t attack_id = client_->getFirstAttackReturn().attack;

    static_cast<Animation *>(attack_animation_->ActionAt(0))
        ->SetFrames(Utils::GetSkillAnimation(attack_id));
  }

  if (apply_on_enemy) {
    attack_->SetSize(enemy_->Width(), enemy_->Height());
    attack_->SetPos(enemy_->X(), enemy_->Y());
  } else {
    attack_->SetSize(player_->Width(), player_->Height());
    attack_->SetPos(player_->X(), player_->Y());
  }

  if (!ProcessAttack()) {
    return;
  }

  attack_->SetVisible(true);
  attack_->RunAction(attack_animation_);
}

void Battle::OnAttackDone() {
  attack_->SetVisible(false);
  if (client_->getAttackReturnList().empty()) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "OnAttackDone(): crash: display an empty attack return");
    doNextAction();
    return;
  }
  if (client_->getAttackReturnList().front().lifeEffectMonster.empty() &&
      client_->getAttackReturnList().front().buffLifeEffectMonster.empty() &&
      client_->getAttackReturnList().front().addBuffEffectMonster.empty() &&
      client_->getAttackReturnList().front().removeBuffEffectMonster.empty() &&
      client_->getAttackReturnList().front().success) {
    qDebug() << QStringLiteral(
                    "OnAttackDone(): strange: display an empty lifeEffect "
                    "list into attack return, attack: %1, "
                    "doByTheCurrentMonster: %2, success: %3")
                    .arg(connexionManager->client->getAttackReturnList()
                             .front()
                             .attack)
                    .arg(connexionManager->client->getAttackReturnList()
                             .front()
                             .doByTheCurrentMonster)
                    .arg(connexionManager->client->getAttackReturnList()
                             .front()
                             .success);
    client_->removeTheFirstAttackReturn();
    doNextAction();
    return;
  }

  const Skill::AttackReturn &attack_return = client_->getFirstAttackReturn();
  // get the life effect to display
  CatchChallenger::ApplyOn apply_on;
  if (!attack_return.lifeEffectMonster.empty()) {
    apply_on = attack_return.lifeEffectMonster.front().on;
  } else if (!attack_return.buffLifeEffectMonster.empty()) {
    apply_on = attack_return.buffLifeEffectMonster.front().on;
  } else if (!attack_return.addBuffEffectMonster.empty()) {
    apply_on = attack_return.addBuffEffectMonster.front().on;
  } else if (!attack_return.removeBuffEffectMonster.empty()) {
    apply_on = attack_return.removeBuffEffectMonster.front().on;
  } else {
    error(QStringLiteral(
              "OnAttackDone(): strange: nothing to display, "
              "lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): "
              "%2, addBuffEffectMonster.size(): %3, "
              "removeBuffEffectMonster.size(): %4, AttackReturnList: %5")
              .arg(attack_return.lifeEffectMonster.size())
              .arg(attack_return.buffLifeEffectMonster.size())
              .arg(attack_return.addBuffEffectMonster.size())
              .arg(attack_return.removeBuffEffectMonster.size())
              .arg(client_->getAttackReturnList().size())
              .toStdString());
    client_->removeTheFirstAttackReturn();
    // doNextAction();
    return;
  }

  bool apply_on_enemy = false;
  if (attack_return.doByTheCurrentMonster) {
    if ((apply_on == ApplyOn::ApplyOn_AloneEnemy) ||
        (apply_on == ApplyOn::ApplyOn_AllEnemy))
      apply_on_enemy = true;
  } else {
    if ((apply_on == ApplyOn::ApplyOn_Themself) ||
        (apply_on == ApplyOn::ApplyOn_AllAlly))
      apply_on_enemy = true;
  }
  if (apply_on_enemy) {
    enemy_->RunAction(sprite_damage_);
  } else {
    player_->RunAction(sprite_damage_);
  }

  if (!attack_return.lifeEffectMonster.empty()) {
    int hp_to_change = attack_return.lifeEffectMonster.front().quantity;
    if (hp_to_change != 0) {
      client_->firstLifeEffectQuantityChange(-hp_to_change);
      //if (apply_on_enemy) {
        //enemy_hp_bar_->IncrementValue(hp_to_change, true);
      //} else {
        //player_hp_bar_->IncrementValue(hp_to_change, true);
      //}
    }
  }
}

void Battle::OnDamageDone() {
  const Skill::AttackReturn &attack_return = client_->getFirstAttackReturn();
  if (!attack_return.lifeEffectMonster.empty())
    client_->removeTheFirstLifeEffectAttackReturn();
  else
    client_->removeTheFirstBuffEffectAttackReturn();
  if (!client_->firstAttackReturnHaveMoreEffect()) {
#ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
      qDebug() << "after display attack: currentMonster have hp "
               << player_hp_bar_->Value() << "and buff" << bottomBuff->count();
      qDebug() << "after display attack: otherMonster have hp"
               << enemy_hp_bar_->value() << "and buff" << enemy_buff_->Count();
    }
#endif
    client_->removeTheFirstAttackReturn();
  }
  // attack is finish
  doNextAction();
}

void Battle::newError(std::string error, std::string detailedError) {
  qDebug() << QString::fromStdString(error + ": " + detailedError);
}

bool Battle::AddBuffEffect(PublicPlayerMonster *current_monster,
                           PublicPlayerMonster *other_monster,
                           const Skill::AttackReturn current_attack,
                           QString attack_owner) {
  const auto &buff_effect = current_attack.addBuffEffectMonster.front();
  UI::ListView *buff_list = nullptr;
#ifdef CATCHCHALLENGER_EXTRA_CHECK
  bool on_buff_current_monster = false;
#endif
  if (current_attack.doByTheCurrentMonster) {
    if ((buff_effect.on & ApplyOn::ApplyOn_AllEnemy) ||
        (buff_effect.on & ApplyOn::ApplyOn_AloneEnemy)) {
      attack_owner +=
          tr("add the buff %2 on the other %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(other_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(buff_effect.buff)
                                              .name));
      //buff_list = enemy_buff_;
#ifdef CATCHCHALLENGER_EXTRA_CHECK
      on_buff_current_monster = false;
#endif
    }
    if ((buff_effect.on & ApplyOn::ApplyOn_AllAlly) ||
        (buff_effect.on & ApplyOn::ApplyOn_Themself)) {
      attack_owner +=
          tr("add the buff %1 on themself")
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(buff_effect.buff)
                                              .name));
      //buff_list = player_buff_;
#ifdef CATCHCHALLENGER_EXTRA_CHECK
      on_buff_current_monster = true;
#endif
    }
  } else {
    if ((buff_effect.on & ApplyOn::ApplyOn_AllEnemy) ||
        (buff_effect.on & ApplyOn::ApplyOn_AloneEnemy)) {
      attack_owner +=
          tr("add the buff %2 on your %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(current_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(buff_effect.buff)
                                              .name));
      //buff_list = player_buff_;
#ifdef CATCHCHALLENGER_EXTRA_CHECK
      on_buff_current_monster = true;
#endif
    }
    if ((buff_effect.on & ApplyOn::ApplyOn_AllAlly) ||
        (buff_effect.on & ApplyOn::ApplyOn_Themself)) {
      attack_owner +=
          tr("add the buff %1 on themself")
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(buff_effect.buff)
                                              .name));
      //buff_list = enemy_buff_;
#ifdef CATCHCHALLENGER_EXTRA_CHECK
      on_buff_current_monster = false;
#endif
    }
  }
  ShowStatusMessage(attack_owner);
  if (buff_list != nullptr) {
    // search the buff
    Sprite *item = nullptr;
    int index = 0;
    while (index < buff_list->Count()) {
      if (buff_list->GetItem(index)->Data(99) == buff_effect.buff) {
        item = static_cast<Sprite *>(buff_list->GetItem(index));
        break;
      }
      index++;
    }
    // add new buff because don't exist
    if (index == buff_list->Count()) {
      item = Sprite::Create();
      item->SetData(99,
                    buff_effect.buff);  // to prevent duplicate buff, because
                                        // add can be to re-enable an already
                                        // enable buff (for larger period then)
    }
    if (QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().find(
            buff_effect.buff) ==
        QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra()
            .cend()) {
      item->SetToolTip(tr("Unknown buff"));
      item->SetPixmap(QPixmap(":/CC/images/interface/buff.png"));
    } else {
      const QtDatapackClientLoader::MonsterExtra::Buff &buff_extra =
          QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(
              buff_effect.buff);
      const QtDatapackClientLoader::QtBuffExtra &qt_buff_extra =
          QtDatapackClientLoader::datapackLoader->getMonsterBuffExtra(
              buff_effect.buff);
      if (!qt_buff_extra.icon.isNull()) {
        item->SetPixmap(qt_buff_extra.icon);
      } else {
        item->SetPixmap(QPixmap(":/CC/images/interface/buff.png"));
      }
      if (buff_effect.level <= 1) {
        item->SetToolTip(QString::fromStdString(buff_extra.name));
      } else {
        item->SetToolTip(tr("%1 at level %2")
                             .arg(QString::fromStdString(buff_extra.name))
                             .arg(buff_effect.level));
      }
      item->SetToolTip(item->ToolTip() + "\n" +
                       QString::fromStdString(buff_extra.description));
    }
    item->SetSize(16, 16);
    if (index == buff_list->Count()) buff_list->AddItem(item);
  }
  client_->removeTheFirstAddBuffEffectAttackReturn();
  if (!client_->firstAttackReturnHaveMoreEffect())
    client_->removeTheFirstAttackReturn();
#ifdef CATCHCHALLENGER_EXTRA_CHECK
  if (!on_buff_current_monster && other_monster != NULL) {
    if (other_monster->buffs.size() != buff_list->Count()) {
      error(
          "displayFirstAttackText(): bug: buffs number displayed != than "
          "real buff count on other monter");
      doNextAction();
      return false;
    }
  }
  if (on_buff_current_monster && current_monster != NULL) {
    if (current_monster->buffs.size() != buff_list->Count()) {
      error(
          "displayFirstAttackText(): bug: buffs number displayed != than "
          "real buff count on current monter");
      doNextAction();
      return false;
    }
  }
#endif
  qDebug() << "display the add buff";
  qDebug()
      << QStringLiteral(
             "displayFirstAttackText(): after display text, "
             "lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): "
             "%2, buff_effect.size(): %3, "
             "removeBuffEffectMonster.size(): %4, attackReturnList.size(): "
             "%5")
             .arg(current_attack.lifeEffectMonster.size())
             .arg(current_attack.buffLifeEffectMonster.size())
             .arg(current_attack.addBuffEffectMonster.size())
             .arg(current_attack.removeBuffEffectMonster.size())
             .arg(client_->getAttackReturnList().size());
  return false;
}

bool Battle::RemoveBuffEffect(PublicPlayerMonster *current_monster,
                              PublicPlayerMonster *other_monster,
                              const Skill::AttackReturn current_attack,
                              QString attack_owner) {
  const Skill::BuffEffect &remove_buff_effect =
      current_attack.removeBuffEffectMonster.front();
  UI::ListView *buff_list = nullptr;
  if (current_attack.doByTheCurrentMonster) {
    if ((remove_buff_effect.on & ApplyOn::ApplyOn_AllEnemy) ||
        (remove_buff_effect.on & ApplyOn::ApplyOn_AloneEnemy)) {
      attack_owner +=
          tr("add the buff %2 on the other %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(other_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(remove_buff_effect.buff)
                                              .name));
      //buff_list = enemy_buff_;
    }
    if ((remove_buff_effect.on & ApplyOn::ApplyOn_AllAlly) ||
        (remove_buff_effect.on & ApplyOn::ApplyOn_Themself)) {
      attack_owner +=
          tr("add the buff %1 on themself")
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(remove_buff_effect.buff)
                                              .name));
      //buff_list = player_buff_;
    }
  } else {
    if ((remove_buff_effect.on & ApplyOn::ApplyOn_AllEnemy) ||
        (remove_buff_effect.on & ApplyOn::ApplyOn_AloneEnemy)) {
      attack_owner +=
          tr("add the buff %2 on your %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(current_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(remove_buff_effect.buff)
                                              .name));
      //buff_list = player_buff_;
    }
    if ((remove_buff_effect.on & ApplyOn::ApplyOn_AllAlly) ||
        (remove_buff_effect.on & ApplyOn::ApplyOn_Themself)) {
      attack_owner +=
          tr("add the buff %1 on themself")
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterBuffsExtra()
                                              .at(remove_buff_effect.buff)
                                              .name));
      //buff_list = enemy_buff_;
    }
  }
  ShowStatusMessage(attack_owner);
  if (buff_list != nullptr) {
    uint8_t index = 0;
    while (index < buff_list->Count()) {
      auto tmp = buff_list->GetItem(index);
      if (tmp->Data(99) == remove_buff_effect.buff) {
        buff_list->RemoveItem(index);
        delete tmp;
      }
    }
  }
  client_->removeTheFirstRemoveBuffEffectAttackReturn();
  if (!client_->firstAttackReturnHaveMoreEffect())
    client_->removeTheFirstAttackReturn();
  qDebug() << "display the remove buff";
  qDebug() << QStringLiteral(
                  "displayFirstAttackText(): after display text, "
                  "lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): "
                  "%2, addBuffEffectMonster.size(): %3, "
                  "remove_buff_effect.size(): %4, attackReturnList.size(): "
                  "%5")
                  .arg(current_attack.lifeEffectMonster.size())
                  .arg(current_attack.buffLifeEffectMonster.size())
                  .arg(current_attack.addBuffEffectMonster.size())
                  .arg(current_attack.removeBuffEffectMonster.size())
                  .arg(client_->getAttackReturnList().size());
  return false;
}

bool Battle::ProcessAttack() {
  bool firstText = true;
  auto other_monster = client_->getOtherMonster();
  auto current_monster = client_->getCurrentMonster();
  if (other_monster == NULL) {
    error(
        "ProcessAttack(): crash: unable to get the other monster to display "
        "an "
        "attack");
    doNextAction();
    return false;
  }
  if (current_monster == NULL) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "ProcessAttack(): crash: unable to get the current monster");
    doNextAction();
    return false;
  }
  if (client_->getAttackReturnList().empty()) {
    emit error("Display text for not existant attack");
    return false;
  }
  const Skill::AttackReturn current_attack = client_->getFirstAttackReturn();
  qDebug() << QStringLiteral(
                  "ProcessAttack(): before display text, "
                  "lifeEffectMonster.size(): %1, "
                  "buffLifeEffectMonster.size(): %2, "
                  "addBuffEffectMonster.size(): %3, "
                  "removeBuffEffectMonster.size(): "
                  "%4, attackReturnList.size(): %5")
                  .arg(current_attack.lifeEffectMonster.size())
                  .arg(current_attack.buffLifeEffectMonster.size())
                  .arg(current_attack.addBuffEffectMonster.size())
                  .arg(current_attack.removeBuffEffectMonster.size())
                  .arg(client_->getAttackReturnList().size());

  // in case of failed
  if (!current_attack.success) {
    if (current_attack.doByTheCurrentMonster)
      ShowStatusMessage(
          tr("Your %1 have failed the attack %2")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(current_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterSkillsExtra()
                                              .at(current_attack.attack)
                                              .name)));
    else
      ShowStatusMessage(
          tr("The other %1 have failed the attack %2")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(other_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterSkillsExtra()
                                              .at(current_attack.attack)
                                              .name)));
    client_->removeTheFirstAttackReturn();
    return false;
  }

  QString attack_owner;
  if (firstText) {
    if (current_attack.doByTheCurrentMonster)
      attack_owner =
          tr("Your %1 do the attack %2 and ")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(current_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterSkillsExtra()
                                              .at(current_attack.attack)
                                              .name));
    else
      attack_owner =
          tr("The other %1 do the attack %2 and ")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(other_monster->monster)
                      .name))
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader
                                              ->get_monsterSkillsExtra()
                                              .at(current_attack.attack)
                                              .name));
  } else {
    if (current_attack.doByTheCurrentMonster) {
      attack_owner = tr("Your %1").arg(QString::fromStdString(
          QtDatapackClientLoader::datapackLoader->get_monsterExtra()
              .at(current_monster->monster)
              .name));
    } else {
      attack_owner =
          tr("The other %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                      .at(other_monster->monster)
                      .name));
    }
  }

  // the life effect
  if (!current_attack.lifeEffectMonster.empty()) {
    const Skill::LifeEffectReturn &life_effect_return =
        current_attack.lifeEffectMonster.front();
    if (current_attack.doByTheCurrentMonster) {
      if ((life_effect_return.on & ApplyOn::ApplyOn_AllEnemy) ||
          (life_effect_return.on & ApplyOn::ApplyOn_AloneEnemy)) {
        if (life_effect_return.quantity > 0)
          attack_owner +=
              tr("heal of %2 the other %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(other_monster->monster)
                          .name))
                  .arg(life_effect_return.quantity);
        else if (life_effect_return.quantity < 0)
          attack_owner +=
              tr("hurt of %2 the other %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(other_monster->monster)
                          .name))
                  .arg(-life_effect_return.quantity);
      }
      if ((life_effect_return.on & ApplyOn::ApplyOn_AllAlly) ||
          (life_effect_return.on & ApplyOn::ApplyOn_Themself)) {
        if (life_effect_return.quantity > 0)
          attack_owner +=
              tr("heal themself of %1").arg(life_effect_return.quantity);
        else if (life_effect_return.quantity < 0)
          attack_owner +=
              tr("hurt themself of %1").arg(-life_effect_return.quantity);
      }
    } else {
      if ((life_effect_return.on & ApplyOn::ApplyOn_AllEnemy) ||
          (life_effect_return.on & ApplyOn::ApplyOn_AloneEnemy)) {
        if (life_effect_return.quantity > 0)
          attack_owner +=
              tr("heal of %2 your %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(current_monster->monster)
                          .name))
                  .arg(life_effect_return.quantity);
        else if (life_effect_return.quantity < 0)
          attack_owner +=
              tr("hurt of %2 your %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(current_monster->monster)
                          .name))
                  .arg(-life_effect_return.quantity);
      }
      if ((life_effect_return.on & ApplyOn::ApplyOn_AllAlly) ||
          (life_effect_return.on & ApplyOn::ApplyOn_Themself)) {
        if (life_effect_return.quantity > 0)
          attack_owner +=
              tr("heal themself of %1").arg(life_effect_return.quantity);
        else if (life_effect_return.quantity < 0)
          attack_owner +=
              tr("hurt themself of %1").arg(-life_effect_return.quantity);
      }
    }
    if (life_effect_return.effective != 1 && life_effect_return.critical) {
      if (life_effect_return.effective > 1)
        attack_owner +=
            QStringLiteral("\n") + tr("It's very effective and critical throw");
      else
        attack_owner += QStringLiteral("\n") +
                        tr("It's not very effective but it's critical throw");
    } else if (life_effect_return.effective != 1) {
      if (life_effect_return.effective > 1)
        attack_owner += QStringLiteral("\n") + tr("It's very effective");
      else
        attack_owner += QStringLiteral("\n") + tr("It's not very effective");
    } else if (life_effect_return.critical) {
      attack_owner += QStringLiteral("\n") + tr("Critical throw");
    }
    ShowStatusMessage(attack_owner, false, false);
    PrintError(__FILE__, __LINE__, "display the life effect");
    PrintError(
        __FILE__, __LINE__,
        QStringLiteral(
            "after display text, lifeEffectMonster.size(): %1, "
            "buffLifeEffectMonster.size(): "
            "%2, addBuffEffectMonster.size(): %3, "
            "removeBuffEffectMonster.size(): %4, attackReturnList.size(): "
            "%5")
            .arg(current_attack.lifeEffectMonster.size())
            .arg(current_attack.buffLifeEffectMonster.size())
            .arg(current_attack.addBuffEffectMonster.size())
            .arg(current_attack.removeBuffEffectMonster.size())
            .arg(client_->getAttackReturnList().size()));
    return true;
  }

  // the add buff
  if (!current_attack.addBuffEffectMonster.empty()) {
    return AddBuffEffect(current_monster, other_monster, current_attack,
                         attack_owner);
  }

  // the remove buff
  if (!current_attack.removeBuffEffectMonster.empty()) {
    return RemoveBuffEffect(current_monster, other_monster, current_attack,
                            attack_owner);
  }

  // the apply buff effect
  if (!current_attack.buffLifeEffectMonster.empty()) {
    qDebug() << "QQQWEQWE";
    const Skill::LifeEffectReturn &buffLifeEffectMonster =
        current_attack.buffLifeEffectMonster.front();
    if (current_attack.doByTheCurrentMonster) {
      if ((buffLifeEffectMonster.on & ApplyOn::ApplyOn_AllEnemy) ||
          (buffLifeEffectMonster.on & ApplyOn::ApplyOn_AloneEnemy)) {
        if (buffLifeEffectMonster.quantity > 0)
          attack_owner +=
              tr("heal of %2 the other %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(other_monster->monster)
                          .name))
                  .arg(buffLifeEffectMonster.quantity);
        else if (buffLifeEffectMonster.quantity < 0)
          attack_owner +=
              tr("hurt of %2 the other %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(other_monster->monster)
                          .name))
                  .arg(-buffLifeEffectMonster.quantity);
      }
      if ((buffLifeEffectMonster.on & ApplyOn::ApplyOn_AllAlly) ||
          (buffLifeEffectMonster.on & ApplyOn::ApplyOn_Themself)) {
        if (buffLifeEffectMonster.quantity > 0)
          attack_owner +=
              tr("heal themself of %1").arg(buffLifeEffectMonster.quantity);
        else if (buffLifeEffectMonster.quantity < 0)
          attack_owner +=
              tr("hurt themself of %1").arg(-buffLifeEffectMonster.quantity);
      }
    } else {
      if ((buffLifeEffectMonster.on & ApplyOn::ApplyOn_AllEnemy) ||
          (buffLifeEffectMonster.on & ApplyOn::ApplyOn_AloneEnemy)) {
        if (buffLifeEffectMonster.quantity > 0)
          attack_owner +=
              tr("heal of %2 your %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(current_monster->monster)
                          .name))
                  .arg(buffLifeEffectMonster.quantity);
        else if (buffLifeEffectMonster.quantity < 0)
          attack_owner +=
              tr("hurt of %2 your %1")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                          .at(current_monster->monster)
                          .name))
                  .arg(-buffLifeEffectMonster.quantity);
      }
      if ((buffLifeEffectMonster.on & ApplyOn::ApplyOn_AllAlly) ||
          (buffLifeEffectMonster.on & ApplyOn::ApplyOn_Themself)) {
        if (buffLifeEffectMonster.quantity > 0)
          attack_owner +=
              tr("heal themself of %1").arg(buffLifeEffectMonster.quantity);
        else if (buffLifeEffectMonster.quantity < 0)
          attack_owner +=
              tr("hurt themself of %1").arg(-buffLifeEffectMonster.quantity);
      }
    }
    if (buffLifeEffectMonster.effective != 1 &&
        buffLifeEffectMonster.critical) {
      if (buffLifeEffectMonster.effective > 1) {
        attack_owner +=
            QStringLiteral("\n") + tr("It's very effective and critical throw");
      } else {
        attack_owner += QStringLiteral("\n") +
                        tr("It's not very effective but it's critical throw");
      }
    } else if (buffLifeEffectMonster.effective != 1) {
      if (buffLifeEffectMonster.effective > 1) {
        attack_owner += QStringLiteral("\n") + tr("It's very effective");
      } else {
        attack_owner += QStringLiteral("\n") + tr("It's not very effective");
      }
    } else if (buffLifeEffectMonster.critical) {
      attack_owner += QStringLiteral("\n") + tr("Critical throw");
    }
    qDebug() << "display the buff effect";
    qDebug()
        << QStringLiteral(
               "displayFirstAttackText(): after display text, "
               "lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): "
               "%2, addBuffEffectMonster.size(): %3, "
               "removeBuffEffectMonster.size(): %4, attackReturnList.size(): "
               "%5")
               .arg(current_attack.lifeEffectMonster.size())
               .arg(current_attack.buffLifeEffectMonster.size())
               .arg(current_attack.addBuffEffectMonster.size())
               .arg(current_attack.removeBuffEffectMonster.size())
               .arg(client_->getAttackReturnList().size());
    return true;
  }
  emit error("Can't display text without effect");
  return false;
}

void Battle::UseTrap(const uint16_t &item_id) {
  if (!client_->tryCatchClient(item_id)) {
    qDebug() << "!connexionManager->client->tryCatchClient(itemId)";
    abort();
  }

  BattleAction action;
  action.type = kBattleAction_UseTrap;
  action.id = item_id;
  action.success = false;
  actions_.push_back(action);

  ShowStatusMessage(
      QStringLiteral("Try catch the wild %1")
          .arg(QString::fromStdString(
              QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                  .at(client_->getOtherMonster()->monster)
                  .name)),
      false, false);
}

void Battle::MonsterCatchSlot(const bool &success) {
  if (client_->playerMonster_catchInProgress.empty()) {
    error("Internal bug: cupture monster list is emtpy");
    return;
  }

  actions_.front().success = success;

  Logger::Log(QStringLiteral("Trap success: %1").arg(success));

  if (!success) {
    client_->generateOtherAttack();
    wait_timer_.start();
  } else {
    battle_context_->Handle();
  }
}

void Battle::OnActionNextClick(Node *) {
  // action_next_->SetVisible(false);
  switch (battleStep) {
    case BattleStep_Presentation:
      if (client_->isInFightWithWild()) {
        player_->RunAction(player_exit_);
      } else {
        player_->RunAction(both_exit_);
      }
      break;
    case BattleStep_PresentationMonster:
    default:
      // PlayerMonsterInitialize(nullptr);
      // enemy_background_->SetVisible(true);
      // player_background_->SetVisible(true);
      player_->RunAction(player_enter_);
      PlayerMonster *monster = client_->getCurrentMonster();
      if (monster != NULL) {
        ShowStatusMessage(
            tr("Protect me %1!")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                        .at(monster->monster)
                        .name)),
            false, true);
      } else {
        qDebug() << "OnActionNextClick(): NULL pointer for "
                    "current monster";
        ShowStatusMessage(tr("Protect me %1!").arg("(Unknow monster)"), false,
                          true);
      }
      break;
  }
}

void Battle::OnActionAttackClick(Node *) {  // ShowActionBarTab(2);
}

void Battle::OnActionBagClick(Node *) { ShowBagDialog(); }

void Battle::OnActionMonsterClick(Node *) { ShowMonsterDialog(true); }

void Battle::OnActionEscapeClick(Node *) {
  if (!client_->canDoFightAction()) {
    ShowMessageDialog(
        "Communication problem",
        tr("Sorry but the client wait more data from the server to do it"));
    return;
  }

  BattleAction action;
  action.type = kBattleAction_Run;
  action.success = fightEngine->tryEscape();
  actions_.push_back(action);

  ProcessActions();
}

void Battle::OnActionReturnClick(Node *) {
  // ShowActionBarTab(1);
}

void Battle::ConfigureBattleground() {
  const auto &playerInformations = client_->get_player_informations_ro();
  // map not located
  if (zone_collision_.walkOn.size() == 0) {
    labelFightBackgroundPix =
        QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png"));
    labelFightForegroundPix =
        QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png"));
    labelFightPlateformBottomPix = QPixmap(
        QStringLiteral(":/CC/images/interface/fight/plateform-background.png"));

    labelFightBackground->SetPixmap(
        QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png"))
            .scaled(800, 440),
        false);
    labelFightForeground->SetPixmap(
        QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png"))
            .scaled(800, 440),
        false);
    labelFightPlateformTop->SetPixmap(
        QPixmap(
            QStringLiteral(":/CC/images/interface/fight/plateform-front.png"))
            .scaled(260, 90),
        false);
    labelFightPlateformBottom->SetPixmap(
        QPixmap(QStringLiteral(
                    ":/CC/images/interface/fight/plateform-background.png"))
            .scaled(230, 90),
        false);
    return;
  }
  unsigned int index = 0;
  while (index < zone_collision_.walkOn.size()) {
    const MonstersCollision &monstersCollision =
        CommonDatapack::commonDatapack.get_monstersCollision().at(
            zone_collision_.walkOn.at(index));
    const MonstersCollisionTemp &monstersCollisionTemp =
        CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(
            zone_collision_.walkOn.at(index));
    if (monstersCollision.item == 0 ||
        playerInformations.items.find(monstersCollision.item) !=
            playerInformations.items.cend()) {
      if (!monstersCollisionTemp.background.empty()) {
        const QString &baseSearch =
            QString::fromStdString(
                connexionManager->client->datapackPathBase()) +
            DATAPACK_BASE_PATH_MAPBASE +
            QString::fromStdString(monstersCollisionTemp.background);
        if (QFile(baseSearch + "/background.png").exists()) {
          labelFightBackground->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/background.png"))
                  .scaled(800, 440),
              false);
        } else if (QFile(baseSearch + "/background.jpg").exists()) {
          const QList<QByteArray> &supportedImageFormats =
              QImageReader::supportedImageFormats();
          if (supportedImageFormats.contains("jpeg") ||
              supportedImageFormats.contains("jpg"))
            labelFightBackground->SetPixmap(
                QPixmap(baseSearch + QStringLiteral("/background.jpg"))
                    .scaled(800, 440),
                false);
        } else {
          labelFightBackground->SetPixmap(
              QPixmap(
                  QStringLiteral(":/CC/images/interface/fight/background.png"))
                  .scaled(800, 440),
              false);
        }

        if (QFile(baseSearch + "/foreground.png").exists())
          labelFightForeground->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/foreground.png"))
                  .scaled(800, 440),
              false);
        else if (QFile(baseSearch + "/foreground.gif").exists())
          labelFightForeground->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/foreground.gif"))
                  .scaled(800, 440),
              false);
        else
          labelFightForeground->SetPixmap(
              QPixmap(
                  QStringLiteral(":/CC/images/interface/fight/foreground.png"))
                  .scaled(800, 440),
              false);

        if (QFile(baseSearch + "/plateform-front.png").exists())
          labelFightPlateformTop->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/plateform-front.png"))
                  .scaled(260, 90),
              false);
        else if (QFile(baseSearch + "/plateform-front.gif").exists())
          labelFightPlateformTop->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/plateform-front.gif"))
                  .scaled(260, 90),
              false);
        else
          labelFightPlateformTop->SetPixmap(
              QPixmap(QStringLiteral(
                          ":/CC/images/interface/fight/plateform-front.png"))
                  .scaled(260, 90),
              false);

        if (QFile(baseSearch + "/plateform-background.png").exists())
          labelFightPlateformBottom->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/plateform-background.png"))
                  .scaled(230, 90),
              false);
        else if (QFile(baseSearch + "/plateform-background.gif").exists())
          labelFightPlateformBottom->SetPixmap(
              QPixmap(baseSearch + QStringLiteral("/plateform-background.gif"))
                  .scaled(230, 90),
              false);
        else
          labelFightPlateformBottom->SetPixmap(
              ":/CC/images/interface/fight/plateform-background.png", false);
      } else {
        labelFightBackgroundPix = QPixmap(
            QStringLiteral(":/CC/images/interface/fight/background.png"));
        labelFightForegroundPix = QPixmap(
            QStringLiteral(":/CC/images/interface/fight/foreground.png"));
        labelFightPlateformBottomPix = QPixmap(QStringLiteral(
            ":/CC/images/interface/fight/plateform-background.png"));

        labelFightBackground->SetPixmap(
            ":/CC/images/interface/fight/background.png", false);
        labelFightForeground->SetPixmap(
            ":/CC/images/interface/fight/foreground.png", false);
        labelFightPlateformTop->SetPixmap(
            ":/CC/images/interface/fight/plateform-front.png", false);
        labelFightPlateformBottom->SetPixmap(
            ":/CC/images/interface/fight/plateform-background.png", false);
      }
      return;
    }
    index++;
  }
  labelFightBackgroundPix =
      QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png"));
  labelFightForegroundPix =
      QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png"));
  labelFightPlateformBottomPix = QPixmap(
      QStringLiteral(":/CC/images/interface/fight/plateform-background.png"));

  labelFightBackground->SetPixmap(":/CC/images/interface/fight/background.png",
                                  false);
  labelFightForeground->SetPixmap(":/CC/images/interface/fight/foreground.png",
                                  false);
  labelFightPlateformTop->SetPixmap(
      QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-front.png"))
          .scaled(260, 90));
  labelFightPlateformBottom->SetPixmap(
      QPixmap(QStringLiteral(
                  ":/CC/images/interface/fight/plateform-background.png"))
          .scaled(230, 90));
}

void Battle::Win() {
  client_->fightFinished();
  doNextActionStep = DoNextActionStep_Start;
  switch (battleType) {
    case BattleType_Bot: {
      if (CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights()
              .find(fightId) ==
          CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights()
              .cend()) {
        emit error("fight id not found at collision");
        return;
      }
      auto cash =
          CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights()
              .at(fightId)
              .cash;
      auto information = client_->get_player_informations();
      information.cash += cash;
      client_->addBeatenBotFight(fightId);
      if (botFightList.empty()) {
        emit error("battle info not found at collision");
        return;
      }
      botFightList.erase(botFightList.cbegin());
      fightId = 0;
    } break;
    case BattleType_OtherPlayer:
      if (battleInformationsList.empty()) {
        emit error("battle info not found at collision");
        return;
      }
      battleInformationsList.erase(battleInformationsList.cbegin());
      break;
    default:
      break;
  }
  CheckEvolution();
  Finished();
}

void Battle::CheckEvolution() {
  PlayerMonster *monster = client_->evolutionByLevelUp();
  if (monster != NULL) {
    const Monster &monsterInformations =
        CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
    const QtDatapackClientLoader::MonsterExtra &monsterInformationsExtra =
        QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
            monster->monster);
    unsigned int index = 0;
    while (index < monsterInformations.evolutions.size()) {
      const Monster::Evolution &evolution =
          monsterInformations.evolutions.at(index);
      if (evolution.type == Monster::EvolutionType_Level &&
          evolution.data.level == monster->level) {
        monsterEvolutionPostion = client_->getPlayerMonsterPosition(monster);
        const Monster &monsterInformationsEvolution =
            CommonDatapack::commonDatapack.get_monsters().at(
                evolution.evolveTo);
        const QtDatapackClientLoader::MonsterExtra
            &monsterInformationsEvolutionExtra =
                QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                    evolution.evolveTo);
        /*
        // create animation widget
        if (animationWidget != NULL) delete animationWidget;
        if (qQuickViewContainer != NULL) delete qQuickViewContainer;
        animationWidget = new QQuickView();
        qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
        qQuickViewContainer->setMinimumSize(QSize(width(), height()));
        qQuickViewContainer->setMaximumSize(QSize(width(), height()));
        qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
        verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
        // show the animation
        stackedWidget->setCurrentWidget(page_animation);
        previousAnimationWidget = page_map;
        if (baseMonsterEvolution != NULL) delete baseMonsterEvolution;
        if (targetMonsterEvolution != NULL) delete targetMonsterEvolution;
        baseMonsterEvolution = new QmlMonsterGeneralInformations(
            monsterInformations, monsterInformationsExtra);
        targetMonsterEvolution = new QmlMonsterGeneralInformations(
            monsterInformationsEvolution, monsterInformationsEvolutionExtra);
        if (evolutionControl != NULL) delete evolutionControl;
        evolutionControl = new EvolutionControl(
            monsterInformations, monsterInformationsExtra,
            monsterInformationsEvolution, monsterInformationsEvolutionExtra);
        if (!connect(evolutionControl, &EvolutionControl::cancel, this,
                     &Battle::evolutionCanceled, Qt::QueuedConnection))
          abort();
        animationWidget->rootContext()->setContextProperty("animationControl",
                                                           &animationControl);
        animationWidget->rootContext()->setContextProperty("evolutionControl",
                                                           evolutionControl);
        animationWidget->rootContext()->setContextProperty("canBeCanceled",
                                                           QVariant(true));
        animationWidget->rootContext()->setContextProperty("itemEvolution",
                                                           QString());
        animationWidget->rootContext()->setContextProperty(
            "baseMonsterEvolution", baseMonsterEvolution);
        animationWidget->rootContext()->setContextProperty(
            "targetMonsterEvolution", targetMonsterEvolution);
        const QString datapackQmlFile =
            QString::fromStdString(client->datapackPathBase()) +
            "qml/evolution-animation.qml";
        if (QFile(datapackQmlFile).exists())
          animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
        else
          animationWidget->setSource(
              QStringLiteral("qrc:/qml/evolution-animation.qml"));*/
        return;
      }
      index++;
    }
  }
  while (!tradeEvolutionMonsters.empty()) {
    const uint8_t monsterPosition = tradeEvolutionMonsters.at(0);
    PlayerMonster *const playerMonster =
        client_->monsterByPosition(monsterPosition);
    if (playerMonster == NULL) {
      tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
      continue;
    }
    const Monster &monsterInformations =
        CommonDatapack::commonDatapack.get_monsters().at(
            playerMonster->monster);
    const QtDatapackClientLoader::MonsterExtra &monsterInformationsExtra =
        QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
            playerMonster->monster);
    unsigned int index = 0;
    while (index < monsterInformations.evolutions.size()) {
      const Monster::Evolution &evolution =
          monsterInformations.evolutions.at(index);
      if (evolution.type == Monster::EvolutionType_Trade) {
        monsterEvolutionPostion = monsterPosition;
        const Monster &monsterInformationsEvolution =
            CommonDatapack::commonDatapack.get_monsters().at(
                evolution.evolveTo);
        const QtDatapackClientLoader::MonsterExtra
            &monsterInformationsEvolutionExtra =
                QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                    evolution.evolveTo);
        /*
        // create animation widget
        if (animationWidget != NULL) delete animationWidget;
        if (qQuickViewContainer != NULL) delete qQuickViewContainer;
        animationWidget = new QQuickView();
        qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
        qQuickViewContainer->setMinimumSize(QSize(800, 600));
        qQuickViewContainer->setMaximumSize(QSize(800, 600));
        qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
        verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
        // show the animation
        stackedWidget->setCurrentWidget(page_animation);
        previousAnimationWidget = page_map;
        if (baseMonsterEvolution != NULL) delete baseMonsterEvolution;
        if (targetMonsterEvolution != NULL) delete targetMonsterEvolution;
        baseMonsterEvolution = new QmlMonsterGeneralInformations(
            monsterInformations, monsterInformationsExtra);
        targetMonsterEvolution = new QmlMonsterGeneralInformations(
            monsterInformationsEvolution, monsterInformationsEvolutionExtra);
        if (evolutionControl != NULL) delete evolutionControl;
        evolutionControl = new EvolutionControl(
            monsterInformations, monsterInformationsExtra,
            monsterInformationsEvolution, monsterInformationsEvolutionExtra);
        if (!connect(evolutionControl, &EvolutionControl::cancel, this,
                     &Battle::evolutionCanceled, Qt::QueuedConnection))
          abort();
        animationWidget->rootContext()->setContextProperty("animationControl",
                                                           &animationControl);
        animationWidget->rootContext()->setContextProperty("evolutionControl",
                                                           evolutionControl);
        animationWidget->rootContext()->setContextProperty("canBeCanceled",
                                                           QVariant(true));
        animationWidget->rootContext()->setContextProperty("itemEvolution",
                                                           QString());
        animationWidget->rootContext()->setContextProperty(
            "baseMonsterEvolution", baseMonsterEvolution);
        animationWidget->rootContext()->setContextProperty(
            "targetMonsterEvolution", targetMonsterEvolution);
        const QString datapackQmlFile =
            QString::fromStdString(client->datapackPathBase()) +
            "qml/evolution-animation.qml";
        if (QFile(datapackQmlFile).exists())
          animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
        else
          animationWidget->setSource(
              QStringLiteral("qrc:/qml/evolution-animation.qml"));*/
        tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
        return;
      }
      index++;
    }
    tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
  }
}

void Battle::OnExperienceGainDone() {
  PlayerMonster *current_monster = client_->getCurrentMonster();
  if (current_monster == NULL) {
    newError(
        tr("Internal error").toStdString() +
            ", file: " + std::string(__FILE__) + ":" + std::to_string(__LINE__),
        "displayExperienceGain(): crash: unable to get the current monster");
    doNextAction();
    return;
  }
  //if (player_monster_lvl_ == current_monster->level &&
      //(uint32_t)player_exp_bar_->Value() > current_monster->remaining_xp) {
    //[>message(
        //QStringLiteral(
            //"part0: displayed xp greater than the real xp: %1>%2, auto fix")
            //.arg(player_exp_bar_->Value())
            //.arg(currentMonster->remaining_xp)
            //.toStdString());*/
    ////player_exp_bar_->SetValue(current_monster->remaining_xp);
    //qDebug() << "no deberia entrar";
    //doNextAction();
    //return;
  //}
  doNextAction();
  return;
}

void Battle::ProcessEnemyMonsterKO() {
  uint32_t last_given_xp = client_->lastGivenXP();
  if (last_given_xp > 2 * 1000 * 1000) {
    newError(tr("Internal error").toStdString() + ", file: " +
                 std::string(__FILE__) + ":" + std::to_string(__LINE__),
             "last_given_xp is negative");
    doNextAction();
    return;
  }
  if (last_given_xp > 0) {
    auto current_monster = client_->getCurrentMonster();
    ShowStatusMessage(
        tr("You %1 gain %2 of experience ")
            .arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                    .at(current_monster->monster)
                    .name))
            .arg(last_given_xp),
        false, false);

    if (player_monster_lvl_ >= CATCHCHALLENGER_MONSTER_LEVEL_MAX) {
      doNextAction();
      return;
    }

    uint32_t max_xp = player_status_->XPMax();
    if (((uint32_t)player_status_->XP() + last_given_xp) >= max_xp) {
      delta_given_xp_ = last_given_xp - (max_xp - player_status_->XP());

      const Monster::Stat &old_stat =
          client_->getStat(CommonDatapack::commonDatapack.get_monsters().at(
                               current_monster->monster),
                           player_monster_lvl_);
      const Monster::Stat &new_stat =
          client_->getStat(CommonDatapack::commonDatapack.get_monsters().at(
                               current_monster->monster),
                           player_monster_lvl_ + 1);
      if (old_stat.hp < new_stat.hp) {
        //qDebug() << QStringLiteral(
                        //"Now the old hp: %1/%2 increased of %3 for the old "
                        //"level %4")
                        //.arg(player_hp_bar_->Value())
                        //.arg(player_hp_bar_->Maximum())
                        //.arg(new_stat.hp - old_stat.hp)
                        //.arg(player_monster_lvl_);
        //player_hp_bar_->SetMaximum(player_hp_bar_->Maximum() +
                                   //(new_stat.hp - old_stat.hp));
        //player_hp_bar_->SetValue(player_hp_bar_->Value() +
                                 //(new_stat.hp - old_stat.hp));
      } else {
        //qDebug()
            //<< QStringLiteral(
                   //"The hp at level change is untouched: %1/%2 for the old "
                   //"level %3")
                   //.arg(player_hp_bar_->Value())
                   //.arg(player_hp_bar_->Maximum())
                   //.arg(player_monster_lvl_);
      }
      //player_exp_bar_->IncrementValue(max_xp - player_exp_bar_->Value(), true);

      if (player_monster_lvl_ >= CATCHCHALLENGER_MONSTER_LEVEL_MAX) {
        doNextAction();
        return;
      }
      player_monster_lvl_++;
      player_->RunAction(experience_gain_);
      return;
    } else {
      if (player_monster_lvl_ < CATCHCHALLENGER_MONSTER_LEVEL_MAX) {
        //player_exp_bar_->IncrementValue(last_given_xp, true);
      } else {
        doNextAction();
        return;
      }
    }
    player_->RunAction(experience_gain_);
    return;
  } else {
    updateCurrentMonsterInformation();
  }
  if (!client_->isInFight()) {
    Win();
    return;
  }
  updateCurrentMonsterInformationXp();
#ifdef CATCHCHALLENGER_EXTRA_CHECK
  if (client_->getAttackReturnList().empty()) {
    PublicPlayerMonster *other_monster = client_->getOtherMonster();
    PublicPlayerMonster *current_monster = client_->getCurrentMonster();
    if (current_monster != NULL) {
      //if (static_cast<int>(current_monster->hp) != player_hp_bar_->Value()) {
        //emit error(QStringLiteral("Current monster hp don't match with the "
                                  //"internal value (do next action): %1!=%2")
                       //.arg(current_monster->hp)
                       //.arg(player_hp_bar_->Value())
                       //.toStdString());
        //return;
      //}
    }
    if (other_monster != NULL) {
      //if (static_cast<int>(other_monster->hp) != enemy_hp_bar_->Value()) {
        //emit error(QStringLiteral("Other monster hp don't match with the "
                                  //"internal value (do next action): %1!=%2")
                       //.arg(other_monster->hp)
                       //.arg(enemy_hp_bar_->Value())
                       //.toStdString());
        //return;
      //}
    }
  }
#endif
  PrintError(__FILE__, __LINE__, "remplace KO other monster");
  auto other_monster = client_->getOtherMonster();
  if (other_monster == NULL) {
    emit error("No other monster into doNextAction()");
    return;
  }
  ShowStatusMessage(
      tr("The other %1 have lost!")
          .arg(QString::fromStdString(
              QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                  .at(other_monster->monster)
                  .name)),
      false, false);
  doNextActionStep = DoNextActionStep_Start;
  // current player monster is KO
  enemy_->RunAction(enemy_dead_);
}

void Battle::OnExperienceIncrementDone() {
  // If the monster levels up
  //if (player_exp_bar_->Value() == player_exp_bar_->Maximum()) {
    //auto current_monster = client_->getCurrentMonster();
    //player_exp_bar_->SetMaximum(CommonDatapack::commonDatapack.get_monsters()
                                    //.at(current_monster->monster)
                                    //.level_to_xp.at(player_monster_lvl_ - 1));
    //player_exp_bar_->SetValue(0);
    //qDebug() << "Delta exp: " << delta_given_xp_;
    //if (delta_given_xp_ > 0) {
      //player_exp_bar_->IncrementValue(delta_given_xp_, true);
    //}
    //delta_given_xp_ = 0;
  //}
}

void Battle::OnBothActionDone(int8_t type) {
  if (type == 0) {  // Enter
    if (battleStep == BattleStep_Presentation) {
      resetPosition(false, true, true);
      // action_next_->SetVisible(true);
    } else if (battleStep == BattleStep_PresentationMonster) {
      doNextActionStep = DoNextActionStep_Start;
      doNextAction();
    } else {
      // action_next_->SetVisible(true);
    }
  } else if (type == 1) {  // Exit
    if (battleStep == BattleStep_Presentation) {
      // PlayerMonsterInitialize(nullptr);
      //  action_next_->SetVisible(false);
      init_other_monster_display();
      updateCurrentMonsterInformation();
      updateOtherMonsterInformation();
      resetPosition(true, true, true);
      QString text;
      PublicPlayerMonster *otherMonster = client_->getOtherMonster();
      if (otherMonster != NULL) {
        text +=
            tr("The other player call %1!")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                        .at(otherMonster->monster)
                        .name));
      } else {
        PrintError(__FILE__, __LINE__, "NULL pointer for current monster");
        text += tr("The other player call %1!").arg("(Unknow monster)");
      }
      text += "\n";
      PublicPlayerMonster *monster = client_->getCurrentMonster();
      if (monster != NULL) {
        text +=
            tr("You call %1!")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                        .at(monster->monster)
                        .name));
      } else {
        PrintError(__FILE__, __LINE__, "NULL pointer for current monster");
        text += tr("You call %1!").arg("(Unknow monster)");
      }
      // ShowActionBarTab(0);
      std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                << text.toStdString() << std::endl;
      // status_label_->SetText(text);
      battleStep = BattleStep_PresentationMonster;

      player_->RunAction(both_enter_);
    } else if (battleStep == BattleStep_PresentationMonster) {
      doNextActionStep = DoNextActionStep_Start;
      doNextAction();
    } else {
      // action_next_->SetVisible(true);
    }
  } else if (type == 2) {  // Dead
    if (battleStep == BattleStep_Presentation) {
      player_->SetPixmap(playerBackImage.scaledToHeight(600));
      // PlayerMonsterInitialize(nullptr);
      init_other_monster_display();
      updateCurrentMonsterInformation();
      updateOtherMonsterInformation();
      battleStep = BattleStep_PresentationMonster;
      player_->RunAction(both_enter_);
    } else {
      // action_next_->SetVisible(true);
    }
  }
}

void Battle::BotFightFullDiffered() {
  prepareFight();
  player_->SetVisible(true);
  enemy_->SetVisible(true);
  ShowStatusMessage(QtDatapackClientLoader::datapackLoader->get_botFightsExtra()
                        .at(fightId)
                        .start,
                    false, false);
  std::vector<PlayerMonster> botFightMonstersTransformed;
  const std::vector<BotFight::BotFightMonster> &monsters =
      CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights()
          .at(fightId)
          .monsters;
  unsigned int index = 0;
  while (index < monsters.size()) {
    const BotFight::BotFightMonster &botFightMonster = monsters.at(index);
    botFightMonstersTransformed.push_back(
        FacilityLib::botFightMonsterToPlayerMonster(
            monsters.at(index),
            client_->getStat(CommonDatapack::commonDatapack.get_monsters().at(
                                 botFightMonster.id),
                             monsters.at(index).level)));
    index++;
  }
  client_->setBotMonster(botFightMonstersTransformed, fightId);
  player_->SetPixmap(playerBackImage.scaledToHeight(600));
  // PlayerMonsterInitialize(nullptr);
  //  action_next_->SetVisible(false);
  battleType = BattleType_Bot;
  QPixmap botImage;
  if (actualBot.properties.find("skin") != actualBot.properties.cend()) {
    botImage = QPixmap(QString::fromStdString(
        QtDatapackClientLoader::datapackLoader->getFrontSkinPath(
            std::stoi(actualBot.properties.at("skin")))));
  } else {
    botImage = QPixmap(QStringLiteral(":/CC/images/player_default/front.png"));
  }
  enemy_->SetPixmap(botImage.scaled(300, 300));
  PrintError(__FILE__, __LINE__,
             QStringLiteral("The bot %1 is a front of you").arg(fightId));
  battleStep = BattleStep_Presentation;
  resetPosition(true, true, true);
  battleStep = BattleStep_Presentation;

  player_->RunAction(both_enter_);
}

void Battle::Finished() { SceneManager::GetInstance()->PopScene(); }

void Battle::OnWaitEngineMoveSlot() {
  if (client_->getAttackReturnList().empty()) {
    wait_timer_.start();
  } else {
    battle_context_->Handle();
  }
}

void Battle::ProcessActions() {
  // Move action run to attackreturn
  if (client_->getAttackReturnList().empty() && actions_.empty()) {
    ShowStatusMessage(tr("In waiting of the other player action"), false,
                      false);
    wait_timer_.start();
  } else {
    battle_context_->Handle();
  }
}
