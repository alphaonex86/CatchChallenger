// Copyright 2021 CatchChallenger
#include "OverMapLogic.hpp"

#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Globals.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/AudioPlayer.hpp"
#include "../../core/Logger.hpp"
#include "../../core/SceneManager.hpp"
#include "../../entities/PlayerInfo.hpp"
#include "../shared/inventory/Inventory.hpp"
#include "../shared/inventory/MonsterBag.hpp"
#include "../shared/inventory/Plant.hpp"
#include "../shared/player/Clan.hpp"
#include "../shared/player/FinishedQuests.hpp"
#include "../shared/player/Player.hpp"
#include "../shared/player/Quests.hpp"
#include "../shared/player/Reputations.hpp"
#include "CCMap.hpp"

using Scenes::Inventory;
using Scenes::OverMapLogic;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

OverMapLogic::OverMapLogic() : OverMap() {
  CreatePlayerTabs();
  CreateInventoryTabs();

  multiplayer = true;
  worseQueryTime = 0;

  checkQueryTime.start(200);
  tip_timeout.setInterval(8000);
  gain_timeout.setInterval(5000);
  tip_timeout.setSingleShot(true);
  gain_timeout.setSingleShot(true);

  shop_ = nullptr;
  warehouse_ = nullptr;
  zonecatch_ = nullptr;
  crafting_ = nullptr;
  learn_ = nullptr;
  industry_ = nullptr;

  npc_talk_->SetOnItemClick(
      std::bind(&OverMapLogic::IG_dialog_text_linkActivated, this, _1));

  if (!connect(&tip_timeout, &QTimer::timeout, this, &OverMapLogic::tipTimeout))
    abort();
  if (!connect(&gain_timeout, &QTimer::timeout, this,
               &OverMapLogic::gainTimeout))
    abort();
}

OverMapLogic *OverMapLogic::Create() {
  return new (std::nothrow) OverMapLogic();
}

void OverMapLogic::setVar(CCMap *ccmap) {
  this->ccmap = ccmap;

  if (!connect(ccmap, &CCMap::pathFindingNotFound, this,
               &OverMapLogic::pathFindingNotFound))
    abort();
  if (!connect(ccmap, &CCMap::repelEffectIsOver, this,
               &OverMapLogic::repelEffectIsOver))
    abort();
  if (!connect(ccmap, &CCMap::teleportConditionNotRespected, this,
               &OverMapLogic::teleportConditionNotRespected))
    abort();
  if (!connect(ccmap, &CCMap::send_player_direction, connexionManager->client,
               &CatchChallenger::Api_protocol_Qt::send_player_direction))
    abort();
  if (!connect(ccmap, &CCMap::stopped_in_front_of, this,
               &OverMapLogic::stopped_in_front_of))
    abort();
  if (!connect(ccmap, &CCMap::actionOn, this, &OverMapLogic::actionOn)) abort();
  if (!connect(ccmap, &CCMap::actionOnNothing, this,
               &OverMapLogic::actionOnNothing))
    abort();
  if (!connect(ccmap, &CCMap::blockedOn, this, &OverMapLogic::blockedOn))
    abort();
  if (!connect(ccmap, &CCMap::error, this, &OverMapLogic::error)) abort();
  if (!connect(ccmap, &CCMap::wildFightCollision, this,
               &OverMapLogic::wildFightCollision))
    abort();
  if (!connect(ccmap, &CCMap::botFightCollision, this,
               &OverMapLogic::botFightCollision))
    abort();
  if (!connect(ccmap, &CCMap::currentMapLoaded, this,
               &OverMapLogic::currentMapLoaded))
    abort();
  if (!connect(ccmap, &CCMap::errorWithTheCurrentMap, this,
               &OverMapLogic::errorWithTheCurrentMap))
    abort();

  OverMap::setVar(ccmap);

  bag->SetOnClick(std::bind(&OverMapLogic::bag_open, this));
  crafting_btn_->SetOnClick(std::bind(&OverMapLogic::OpenCrafting, this));
  player_portrait_->SetOnClick(std::bind(&OverMapLogic::player_open, this));
}

void OverMapLogic::resetAll() {
  lastStepUsed = 0;
  worseQueryTime = 0;
  add_to_inventoryGainList.clear();
  add_to_inventoryGainTime.clear();
  add_to_inventoryGainExtraList.clear();
  add_to_inventoryGainExtraTime.clear();
  currentAmbianceFile.clear();
  visualCategory.clear();
  OverMap::resetAll();
#ifndef CATCHCHALLENGER_NOAUDIO
  AudioPlayer::GetInstance()->StopCurrentAmbiance();
#endif
  actualBot.botId = 0;
  actualBot.name.clear();
  actualBot.properties.clear();
  actualBot.skin.clear();
  actualBot.step.clear();
  actionClan.clear();
  isInQuest = false;
  questId = 0;
  clanName.clear();
  haveClanInformations = false;
  zonecatchName.clear();
  tempQuantityForSell = 0;
  waitToSell = false;
  objectInUsing.clear();
}

void OverMapLogic::pathFindingNotFound() { showTip("No path to go here"); }

void OverMapLogic::repelEffectIsOver() { showTip("The repel effect is over"); }

void OverMapLogic::teleportConditionNotRespected(const std::string &text) {
  showTip(text);
}

void OverMapLogic::stopped_in_front_of(CatchChallenger::Map_client *map,
                                       uint8_t x, uint8_t y) {
  if (stopped_in_front_of_check_bot(map, x, y))
    return;
  else if (CatchChallenger::MoveOnTheMap::isDirt(*map, x, y)) {
    unsigned int index = 0;
    while (index < map->plantList.size()) {
      if (map->plantList.at(index)->x == x &&
          map->plantList.at(index)->y == y) {
        uint64_t current_time = QDateTime::currentMSecsSinceEpoch() / 1000;
        if (map->plantList.at(index)->mature_at <= current_time)
          showTip(tr("To recolt the plant press <i>Enter</i>").toStdString());
        else
          showTip(
              tr("This plant is growing and can't be collected").toStdString());
        return;
      }
      index++;
    }
    showTip(tr("To plant a seed press <i>Enter</i>").toStdString());
    return;
  } else {
    if (!CatchChallenger::MoveOnTheMap::isWalkable(*map, x, y)) {
      // check bot with border
      CatchChallenger::CommonMap *current_map = map;
      switch (ccmap->mapController.getDirection()) {
        case CatchChallenger::Direction_look_at_left:
          if (CatchChallenger::MoveOnTheMap::canGoTo(
                  CatchChallenger::Direction_move_at_left, *map, x, y, false))
            if (CatchChallenger::MoveOnTheMap::move(
                    CatchChallenger::Direction_move_at_left, &current_map, &x,
                    &y, false))
              stopped_in_front_of_check_bot(map, x, y);
          break;
        case CatchChallenger::Direction_look_at_right:
          if (CatchChallenger::MoveOnTheMap::canGoTo(
                  CatchChallenger::Direction_move_at_right, *map, x, y, false))
            if (CatchChallenger::MoveOnTheMap::move(
                    CatchChallenger::Direction_move_at_right, &current_map, &x,
                    &y, false))
              stopped_in_front_of_check_bot(map, x, y);
          break;
        case CatchChallenger::Direction_look_at_top:
          if (CatchChallenger::MoveOnTheMap::canGoTo(
                  CatchChallenger::Direction_move_at_top, *map, x, y, false))
            if (CatchChallenger::MoveOnTheMap::move(
                    CatchChallenger::Direction_move_at_top, &current_map, &x,
                    &y, false))
              stopped_in_front_of_check_bot(map, x, y);
          break;
        case CatchChallenger::Direction_look_at_bottom:
          if (CatchChallenger::MoveOnTheMap::canGoTo(
                  CatchChallenger::Direction_move_at_bottom, *map, x, y, false))
            if (CatchChallenger::MoveOnTheMap::move(
                    CatchChallenger::Direction_move_at_bottom, &current_map, &x,
                    &y, false))
              stopped_in_front_of_check_bot(map, x, y);
          break;
        default:
          break;
      }
    }
  }
}

void OverMapLogic::connectAllSignals() {
  // for bug into solo player: Api_client_real -> Api_protocol
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtclanActionFailed, this,
               &OverMapLogic::ClanActionFailedSlot, Qt::UniqueConnection))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtclanActionSuccess, this,
               &OverMapLogic::ClanActionSuccessSlot, Qt::UniqueConnection))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtclanDissolved, this,
               &OverMapLogic::ClanDissolvedSlot, Qt::UniqueConnection))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtclanInformations, this,
               &OverMapLogic::ClanInformationSlot, Qt::UniqueConnection))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtclanInvite, this,
               &OverMapLogic::ClanInviteSlot, Qt::UniqueConnection))
    abort();

  // Signal city capture
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtcaptureCityYourAreNotLeader,
               this, &OverMapLogic::CaptureCityYourAreNotLeaderSlot))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::
                   QtcaptureCityYourLeaderHaveStartInOtherCity,
               this, &OverMapLogic::CaptureCityYourLeaderHaveStartInOtherCitySlot))
    abort();
  if (!connect(
          connexionManager->client,
          &CatchChallenger::Api_client_real::QtcaptureCityPreviousNotFinished,
          this, &OverMapLogic::CaptureCityPreviousNotFinishedSlot))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtcaptureCityStartBotFight,
               this, &OverMapLogic::CaptureCityStartBotFightSlot))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtcaptureCityWin, this,
               &OverMapLogic::CaptureCityWinSlot))
    abort();

  // inventory
  // if (!connect(connexionManager->client,
  //&CatchChallenger::Api_client_real::Qthave_inventory, this,
  //&OverMapLogic::have_inventory))
  // abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::Qtadd_to_inventory, this,
               &OverMapLogic::add_to_inventory_slot))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::Qtremove_to_inventory, this,
               &OverMapLogic::remove_to_inventory_slot))
    abort();

  // plants
  if (!connect(this, &OverMapLogic::useSeed, connexionManager->client,
               &CatchChallenger::Api_client_real::useSeed))
    abort();
  if (!connect(this, &OverMapLogic::collectMaturePlant,
               connexionManager->client,
               &CatchChallenger::Api_client_real::collectMaturePlant))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::Qtinsert_plant, this,
               &OverMapLogic::insert_plant))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::Qtremove_plant, this,
               &OverMapLogic::remove_plant))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::Qtseed_planted, this,
               &OverMapLogic::seed_planted))
    abort();
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::Qtplant_collected, this,
               &OverMapLogic::plant_collected))
    abort();
  // crafting
  /*if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtrecipeUsed,
     this,&OverMapLogic::recipeUsed)) abort();*/
  /*//trade
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeRequested,
  this,&OverMapLogic::tradeRequested)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAcceptedByOther,
  this,&OverMapLogic::tradeAcceptedByOther)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeCanceledByOther,
  this,&OverMapLogic::tradeCanceledByOther)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeFinishedByOther,
  this,&OverMapLogic::tradeFinishedByOther)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeValidatedByTheServer,
  this,&OverMapLogic::tradeValidatedByTheServer)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAddTradeCash,
  this,&OverMapLogic::tradeAddTradeCash)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAddTradeObject,
  this,&OverMapLogic::tradeAddTradeObject)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAddTradeMonster,
  this,&OverMapLogic::tradeAddTradeMonster)) abort();*/
  // inventory
  if (!connect(connexionManager->client,
               &CatchChallenger::Api_client_real::QtobjectUsed, this,
               &OverMapLogic::objectUsed))
    abort();
  /*//battle
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtbattleRequested,
  this,&OverMapLogic::battleRequested)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtbattleAcceptedByOther,
  this,&OverMapLogic::battleAcceptedByOther)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtbattleCanceledByOther,
  this,&OverMapLogic::battleCanceledByOther)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtsendBattleReturn,
  this,&OverMapLogic::sendBattleReturn)) abort();
  //market
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketList,
  this,&OverMapLogic::marketList)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketBuy,
  this,&OverMapLogic::marketBuy)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketBuyMonster,
  this,&OverMapLogic::marketBuyMonster)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketPut,
  this,&OverMapLogic::marketPut)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketGetCash,
  this,&OverMapLogic::marketGetCash)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketWithdrawCanceled,
  this,&OverMapLogic::marketWithdrawCanceled)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketWithdrawObject,
  this,&OverMapLogic::marketWithdrawObject)) abort();
  if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketWithdrawMonster,
  this,&OverMapLogic::marketWithdrawMonster)) abort();*/
  // fight
  /*    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtteleportTo,this,&OverMapLogic::teleportTo,Qt::UniqueConnection))
         abort();*/
}

void OverMapLogic::selectObject(const ObjectType &objectType) {}

void OverMapLogic::actionOn(CatchChallenger::Map_client *map, uint8_t x,
                            uint8_t y) {
  RemoveChild(npc_talk_);
  if (actionOnCheckBot(map, x, y))
    return;
  else if (CatchChallenger::MoveOnTheMap::isDirt(*map, x, y)) {
    /* -1 == not found */
    int index = havePlant(map, x, y);
    if (index >= 0) {
      uint64_t current_time = QDateTime::currentMSecsSinceEpoch() / 1000;
      if (map->plantList.at(index)->mature_at <= current_time) {
        if (QtDatapackClientLoader::datapackLoader->get_plantOnMap().find(
                map->map_file) ==
            QtDatapackClientLoader::datapackLoader->get_plantOnMap().cend())
          return;
        const std::unordered_map<std::pair<uint8_t, uint8_t>, uint16_t,
                                 pairhash> &plant =
            QtDatapackClientLoader::datapackLoader->get_plantOnMap().at(
                map->map_file);
        if (plant.find(std::pair<uint8_t, uint8_t>(x, y)) == plant.cend())
          return;
        emit collectMaturePlant();

        connexionManager->client->remove_plant(
            ccmap->mapController.getMap(map->map_file)->logicalMap.id, x, y);
        connexionManager->client->plant_collected(
            CatchChallenger::Plant_collect::Plant_collect_correctly_collected);
      } else
        showTip(
            tr("This plant is growing and can't be collected").toStdString());
    } else {
      SeedInWaiting seedInWaiting;
      seedInWaiting.map = map->map_file;
      seedInWaiting.x = x;
      seedInWaiting.y = y;
      seed_in_waiting.push_back(seedInWaiting);

      ShowPlantDialog();
    }
    return;
  } else if (map->itemsOnMap.find(std::pair<uint8_t, uint8_t>(x, y)) !=
             map->itemsOnMap.cend()) {
    CatchChallenger::Player_private_and_public_informations &informations =
        connexionManager->client->get_player_informations();
    const CatchChallenger::Map_client::ItemOnMapForClient &item =
        map->itemsOnMap.at(std::pair<uint8_t, uint8_t>(x, y));
    if (informations.itemOnMap.find(item.indexOfItemOnMap) ==
        informations.itemOnMap.cend()) {
      if (!item.infinite) informations.itemOnMap.insert(item.indexOfItemOnMap);
      add_to_inventory(item.item);
      connexionManager->client->takeAnObjectOnMap();
    }
  } else {
    // check bot with border
    CatchChallenger::CommonMap *current_map = map;
    switch (ccmap->mapController.getDirection()) {
      case CatchChallenger::Direction_look_at_left:
        if (CatchChallenger::MoveOnTheMap::canGoTo(
                CatchChallenger::Direction_move_at_left, *map, x, y, false))
          if (CatchChallenger::MoveOnTheMap::move(
                  CatchChallenger::Direction_move_at_left, &current_map, &x, &y,
                  false))
            actionOnCheckBot(map, x, y);
        break;
      case CatchChallenger::Direction_look_at_right:
        if (CatchChallenger::MoveOnTheMap::canGoTo(
                CatchChallenger::Direction_move_at_right, *map, x, y, false))
          if (CatchChallenger::MoveOnTheMap::move(
                  CatchChallenger::Direction_move_at_right, &current_map, &x,
                  &y, false))
            actionOnCheckBot(map, x, y);
        break;
      case CatchChallenger::Direction_look_at_top:
        if (CatchChallenger::MoveOnTheMap::canGoTo(
                CatchChallenger::Direction_move_at_top, *map, x, y, false))
          if (CatchChallenger::MoveOnTheMap::move(
                  CatchChallenger::Direction_move_at_top, &current_map, &x, &y,
                  false))
            actionOnCheckBot(map, x, y);
        break;
      case CatchChallenger::Direction_look_at_bottom:
        if (CatchChallenger::MoveOnTheMap::canGoTo(
                CatchChallenger::Direction_move_at_bottom, *map, x, y, false))
          if (CatchChallenger::MoveOnTheMap::move(
                  CatchChallenger::Direction_move_at_bottom, &current_map, &x,
                  &y, false))
            actionOnCheckBot(map, x, y);
        break;
      default:
        break;
    }
  }
}

void OverMapLogic::actionOnNothing() { RemoveChild(npc_talk_); }

int32_t OverMapLogic::havePlant(CatchChallenger::Map_client *map, uint8_t x,
                                uint8_t y) const {
  if (QtDatapackClientLoader::datapackLoader->get_plantOnMap().find(
          map->map_file) ==
      QtDatapackClientLoader::datapackLoader->get_plantOnMap().cend())
    return -1;
  const std::unordered_map<std::pair<uint8_t, uint8_t>, uint16_t, pairhash>
      &plant = QtDatapackClientLoader::datapackLoader->get_plantOnMap().at(
          map->map_file);
  if (plant.find(std::pair<uint8_t, uint8_t>(x, y)) == plant.cend()) return -1;
  unsigned int index = 0;
  while (index < map->plantList.size()) {
    if (map->plantList.at(index)->x == x && map->plantList.at(index)->y == y)
      return index;
    index++;
  }
  return -1;
}

void OverMapLogic::blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar) {
  switch (blockOnVar) {
    case MapVisualiserPlayer::BlockedOn_ZoneFight:
    case MapVisualiserPlayer::BlockedOn_Fight:
      qDebug()
          << "You can't enter to the fight zone if you are not able to fight";
      showTip(
          tr("You can't enter to the fight zone if you are not able to fight")
              .toStdString());
      break;
    case MapVisualiserPlayer::BlockedOn_ZoneItem:
      qDebug() << "You can't enter to this zone without the correct item";
      showTip(tr("You can't enter to this zone without the correct item")
                  .toStdString());
      break;
    case MapVisualiserPlayer::BlockedOn_RandomNumber:
      qDebug() << "You can't enter to the fight zone, because have not random "
                  "number";
      showTip(tr("You can't enter to the fight zone, because have not random "
                 "number")
                  .toStdString());
      break;
    default:
      qDebug() << "You can't enter to the zone, for unknown reason";
      showTip(tr("You can't enter to the zone").toStdString());
      break;
  }
}

void OverMapLogic::errorWithTheCurrentMap() {
  if (connexionManager->state() != QAbstractSocket::ConnectedState) return;
  connexionManager->client->tryDisconnect();
  resetAll();
  error(tr("The current map into the datapack is in error (not found, read "
           "failed, wrong format, corrupted, ...)\nReport the bug to the "
           "datapack maintainer.")
            .toStdString());
}

void OverMapLogic::bag_open() {
  std::string id = "inventory";

  inventory_tabs_->ShowNavigation(true);
  auto item = static_cast<Inventory *>(inventory_tabs_->Item(id));
  item->SetVar(Inventory::ObjectFilter_All, false);
  inventory_tabs_->SetCurrentItem(id);
  AddChild(inventory_tabs_);
}

void OverMapLogic::OpenCrafting() {
  if (crafting_ == nullptr) {
    crafting_ = Scenes::Crafting::Create();
  }
  AddChild(crafting_);
}

void OverMapLogic::OnInventoryNav(std::string id) {
  if (id == "inventory") {
    static_cast<Inventory *>(inventory_tabs_->Item(id))
        ->SetVar(Inventory::ObjectFilter_All, false);
  } else if (id == "plants") {
    static_cast<Plant *>(inventory_tabs_->Item(id))->SetVar(false);
  }
}

void OverMapLogic::inventoryNext() {
  /*
switch (inventoryIndex) {
  case 0:
    if (plant == nullptr) {
      plant = new Plant();
      if (!connect(plant, &Plant::setAbove, this, &OverMapLogic::setAbove))
        abort();
      if (!connect(plant, &Plant::sendNext, this,
                   &OverMapLogic::inventoryNext))
        abort();
      if (!connect(plant, &Plant::sendBack, this,
                   &OverMapLogic::inventoryBack))
        abort();
    }
    plant->setVar(connexionManager, false);
    setAbove(plant);
    break;
  case 1:
    if (crafting == nullptr) {
      crafting = new Crafting();
      if (!connect(crafting, &Crafting::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(crafting, &Crafting::sendNext, this,
                   &OverMapLogic::inventoryNext))
        abort();
      if (!connect(crafting, &Crafting::sendBack, this,
                   &OverMapLogic::inventoryBack))
        abort();
      if (!connect(crafting, &Crafting::remove_to_inventory, this,
                   &OverMapLogic::remove_to_inventory_slotpair))
        abort();
      if (!connect(crafting, &Crafting::add_to_inventory, this,
                   &OverMapLogic::add_to_inventory_slotpair))
        abort();
      if (!connect(crafting, &Crafting::appendReputationRewards, this,
                   &OverMapLogic::appendReputationRewards))
        abort();
    }
    crafting->setVar(connexionManager);
    setAbove(crafting);
    break;
  default:
    if (inventory == nullptr) {
      inventory = new Inventory();
      if (!connect(inventory, &Inventory::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(inventory, &Inventory::sendNext, this,
                   &OverMapLogic::inventoryNext))
        abort();
      if (!connect(inventory, &Inventory::sendBack, this,
                   &OverMapLogic::inventoryBack))
        abort();
    }
    inventory->setVar(connexionManager, Inventory::ObjectType::ObjectType_All,
                      false);
    setAbove(inventory);
    inventoryIndex = 0;
    return;
}
inventoryIndex++;
*/
}

void OverMapLogic::inventoryBack() {
  /*
switch (inventoryIndex) {
  case 1:
    if (inventory == nullptr) {
      inventory = new Inventory();
      if (!connect(inventory, &Inventory::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(inventory, &Inventory::sendNext, this,
                   &OverMapLogic::inventoryNext))
        abort();
      if (!connect(inventory, &Inventory::sendBack, this,
                   &OverMapLogic::inventoryBack))
        abort();
    }
    inventory->setVar(connexionManager, Inventory::ObjectType::ObjectType_All,
                      false);
    setAbove(inventory);
    break;
  case 2:
    if (plant == nullptr) {
      plant = new Plant();
      if (!connect(plant, &Plant::setAbove, this, &OverMapLogic::setAbove))
        abort();
      if (!connect(plant, &Plant::sendNext, this,
                   &OverMapLogic::inventoryNext))
        abort();
      if (!connect(plant, &Plant::sendBack, this,
                   &OverMapLogic::inventoryBack))
        abort();
    }
    plant->setVar(connexionManager, false);
    setAbove(plant);
    break;
  default:
    if (crafting == nullptr) {
      crafting = new Crafting();
      if (!connect(crafting, &Crafting::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(crafting, &Crafting::sendNext, this,
                   &OverMapLogic::inventoryNext))
        abort();
      if (!connect(crafting, &Crafting::sendBack, this,
                   &OverMapLogic::inventoryBack))
        abort();
      if (!connect(crafting, &Crafting::add_to_inventory, this,
                   &OverMapLogic::add_to_inventory_slotpair))
        abort();
      if (!connect(crafting, &Crafting::remove_to_inventory, this,
                   &OverMapLogic::remove_to_inventory_slotpair))
        abort();
      if (!connect(crafting, &Crafting::appendReputationRewards, this,
                   &OverMapLogic::appendReputationRewards))
        abort();
    }
    crafting->setVar(connexionManager, false);
    setAbove(crafting);
    inventoryIndex = 2;
    return;
}
inventoryIndex--;
*/
}

void OverMapLogic::player_open() {
  player_tabs_->ShowNavigation(true);
  player_tabs_->SetCurrentItem("player");
  AddChild(player_tabs_);
}

void OverMapLogic::playerNext() {
  /*
switch (playerIndex) {
  case 0:
    if (reputations == nullptr) {
      reputations = new Reputations();
      if (!connect(reputations, &Reputations::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(reputations, &Reputations::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(reputations, &Reputations::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    reputations->setVar(connexionManager);
    setAbove(reputations);
    break;
  case 1:
    if (quests == nullptr) {
      quests = new Quests();
      if (!connect(quests, &Quests::setAbove, this, &OverMapLogic::setAbove))
        abort();
      if (!connect(quests, &Quests::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(quests, &Quests::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    quests->setVar(connexionManager);
    setAbove(quests);
    break;
  case 2:
    if (finishedQuests == nullptr) {
      finishedQuests = new FinishedQuests();
      if (!connect(finishedQuests, &FinishedQuests::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(finishedQuests, &FinishedQuests::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(finishedQuests, &FinishedQuests::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    finishedQuests->setVar(connexionManager);
    setAbove(finishedQuests);
    break;
  default:
    if (player == nullptr) {
      player = new Player();
      if (!connect(player, &Player::setAbove, this, &OverMapLogic::setAbove))
        abort();
      if (!connect(player, &Player::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(player, &Player::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    player->setVar(connexionManager);
    setAbove(player);
    playerIndex = 0;
    return;
}
playerIndex++;
*/
}

void OverMapLogic::playerBack() {
  /*
switch (playerIndex) {
  case 1:
    if (player == nullptr) {
      player = new Player();
      if (!connect(player, &Player::setAbove, this, &OverMapLogic::setAbove))
        abort();
      if (!connect(player, &Player::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(player, &Player::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    player->setVar(connexionManager);
    setAbove(player);
    break;
  case 2:
    if (reputations == nullptr) {
      reputations = new Reputations();
      if (!connect(reputations, &Reputations::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(reputations, &Reputations::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(reputations, &Reputations::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    reputations->setVar(connexionManager);
    setAbove(reputations);
    break;
  case 3:
    if (quests == nullptr) {
      quests = new Quests();
      if (!connect(quests, &Quests::setAbove, this, &OverMapLogic::setAbove))
        abort();
      if (!connect(quests, &Quests::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(quests, &Quests::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    quests->setVar(connexionManager);
    setAbove(quests);
    break;
  default:
    if (finishedQuests == nullptr) {
      finishedQuests = new FinishedQuests();
      if (!connect(finishedQuests, &FinishedQuests::setAbove, this,
                   &OverMapLogic::setAbove))
        abort();
      if (!connect(finishedQuests, &FinishedQuests::sendNext, this,
                   &OverMapLogic::playerNext))
        abort();
      if (!connect(finishedQuests, &FinishedQuests::sendBack, this,
                   &OverMapLogic::playerBack))
        abort();
    }
    finishedQuests->setVar(connexionManager);
    setAbove(finishedQuests);
    playerIndex = 2;
    return;
}
playerIndex--;
*/
}

void OverMapLogic::currentMapLoaded() {
  qDebug() << "OverMapLogic::currentMapLoaded(): map: "
           << QString::fromStdString(ccmap->mapController.currentMap())
           << " with type: "
           << QString::fromStdString(ccmap->mapController.currentMapType());
  // name
  {
    Map_full *mapFull = ccmap->mapController.currentMapFull();
    std::string visualName;
    if (!mapFull->zone.empty())
      if (QtDatapackClientLoader::datapackLoader->get_zonesExtra().find(
              mapFull->zone) !=
          QtDatapackClientLoader::datapackLoader->get_zonesExtra().cend()) {
        const QtDatapackClientLoader::ZoneExtra &zoneExtra =
            QtDatapackClientLoader::datapackLoader->get_zonesExtra().at(
                mapFull->zone);
        visualName = zoneExtra.name;
      }
    if (visualName.empty()) visualName = mapFull->name;
    if (!visualName.empty() && lastPlaceDisplayed != visualName) {
      lastPlaceDisplayed = visualName;
      showPlace(tr("You arrive at <b><i>%1</i></b>")
                    .arg(QString::fromStdString(visualName))
                    .toStdString());
    }
  }
  const std::string &type = ccmap->mapController.currentMapType();
#ifndef CATCHCHALLENGER_NOAUDIO
  // sound
  {
    std::vector<std::string> soundList;
    const std::string &backgroundsound =
        ccmap->mapController.currentBackgroundsound();
    // map sound
    if (!backgroundsound.empty() &&
        !vectorcontainsAtLeastOne(soundList, backgroundsound))
      soundList.push_back(backgroundsound);
    // zone sound
    Map_full *mapFull = ccmap->mapController.currentMapFull();
    if (!mapFull->zone.empty())
      if (QtDatapackClientLoader::datapackLoader->get_zonesExtra().find(
              mapFull->zone) !=
          QtDatapackClientLoader::datapackLoader->get_zonesExtra().cend()) {
        const QtDatapackClientLoader::ZoneExtra &zoneExtra =
            QtDatapackClientLoader::datapackLoader->get_zonesExtra().at(
                mapFull->zone);
        if (zoneExtra.audioAmbiance.find(type) !=
            zoneExtra.audioAmbiance.cend()) {
          const std::string &backgroundsound = zoneExtra.audioAmbiance.at(type);
          if (!backgroundsound.empty() &&
              !vectorcontainsAtLeastOne(soundList, backgroundsound))
            soundList.push_back(backgroundsound);
        }
      }
    // general sound
    if (QtDatapackClientLoader::datapackLoader->get_audioAmbiance().find(
            type) !=
        QtDatapackClientLoader::datapackLoader->get_audioAmbiance().cend()) {
      const std::string &backgroundsound =
          QtDatapackClientLoader::datapackLoader->get_audioAmbiance().at(type);
      if (!backgroundsound.empty() &&
          !vectorcontainsAtLeastOne(soundList, backgroundsound))
        soundList.push_back(backgroundsound);
    }

    std::string finalSound;
    unsigned int index = 0;
    while (index < soundList.size()) {
      // search into main datapack
      const std::string &fileToSearchMain =
          connexionManager->client->datapackPathMain() + soundList.at(index);
      if (QFileInfo(QString::fromStdString(fileToSearchMain)).isFile()) {
        finalSound = fileToSearchMain;
        break;
      }
      // search into base datapack
      const std::string &fileToSearchBase =
          connexionManager->client->datapackPathBase() + soundList.at(index);
      if (QFileInfo(QString::fromStdString(fileToSearchBase)).isFile()) {
        finalSound = fileToSearchBase;
        break;
      }
      index++;
    }

    // set the sound
    if (!finalSound.empty() && currentAmbianceFile != finalSound)
      AudioPlayer::GetInstance()->StartAmbiance(finalSound);
  }
#endif
  // color
  {
    if (visualCategory != type) {
      visualCategory = type;
      if (QtDatapackClientLoader::datapackLoader->get_visualCategories().find(
              type) !=
          QtDatapackClientLoader::datapackLoader->get_visualCategories()
              .cend()) {
        const std::vector<
            QtDatapackClientLoader::VisualCategory::VisualCategoryCondition>
            &conditions =
                QtDatapackClientLoader::datapackLoader->get_visualCategories()
                    .at(type)
                    .conditions;
        unsigned int index = 0;
        while (index < conditions.size()) {
          const QtDatapackClientLoader::VisualCategory::VisualCategoryCondition
              &condition = conditions.at(index);
          const std::vector<uint8_t> events =
              connexionManager->client->getEvents();
          if (condition.event < events.size()) {
            if (events.at(condition.event) == condition.eventValue) {
              ccmap->mapController.setColor(
                  QColor(condition.color.r, condition.color.g,
                         condition.color.b, condition.color.a));
              break;
            }
          } else
            qDebug()
                << QStringLiteral(
                       "event for condition out of range: %1 for %2 event(s)")
                       .arg(condition.event)
                       .arg(events.size());
          index++;
        }
        if (index == conditions.size()) {
          QtDatapackClientLoader::CCColor defaultColor =
              QtDatapackClientLoader::datapackLoader->get_visualCategories()
                  .at(type)
                  .defaultColor;
          ccmap->mapController.setColor(QColor(defaultColor.r, defaultColor.g,
                                               defaultColor.b, defaultColor.a),
                                        15000);
        }
      } else
        ccmap->mapController.setColor(Qt::transparent);
    }
  }
}

void OverMapLogic::tipTimeout() { hideTip(); }

void OverMapLogic::gainTimeout() {
  unsigned int index = 0;
  while (index < add_to_inventoryGainTime.size()) {
    if (add_to_inventoryGainTime.at(index).elapsed() > 8000) {
      add_to_inventoryGainTime.erase(add_to_inventoryGainTime.cbegin() + index);
      add_to_inventoryGainList.erase(add_to_inventoryGainList.cbegin() + index);
    } else
      index++;
  }
  index = 0;
  while (index < add_to_inventoryGainExtraTime.size()) {
    if (add_to_inventoryGainExtraTime.at(index).elapsed() > 8000) {
      add_to_inventoryGainExtraTime.erase(
          add_to_inventoryGainExtraTime.cbegin() + index);
      add_to_inventoryGainExtraList.erase(
          add_to_inventoryGainExtraList.cbegin() + index);
    } else
      index++;
  }
  if (add_to_inventoryGainTime.empty() && add_to_inventoryGainExtraTime.empty())
    hideGain();
  else {
    gain_timeout.start();
    composeAndDisplayGain();
  }
}

void OverMapLogic::showTip(const std::string &tip) {
  tip_->SetText(tip);
  tip_->SetVisible(true);
  tip_timeout.start();
}

void OverMapLogic::hideTip() { tip_->SetVisible(false); }

void OverMapLogic::showPlace(const std::string &place) {
  add_to_inventoryGainExtraList.push_back(place);
  add_to_inventoryGainExtraTime.push_back(QTime::currentTime());
  // ui->gain->setVisible(true);
  composeAndDisplayGain();
  gain_timeout.start();
}

void OverMapLogic::showGain() {
  gainTimeout();
  // ui->gain->setVisible(true);
  composeAndDisplayGain();
  gain_timeout.start();
}

void OverMapLogic::hideGain() { toast_->SetVisible(false); }

void OverMapLogic::composeAndDisplayGain() {
  std::vector<std::string> buffer = add_to_inventoryGainExtraList;
  if (add_to_inventoryGainList.size() > 1) {
    buffer.push_back(tr("You have obtained: ").toStdString());
    buffer.insert(buffer.end(), add_to_inventoryGainList.begin(),
                  add_to_inventoryGainList.end());
  } else if (!add_to_inventoryGainList.empty()) {
    buffer.push_back(tr("You have obtained: ").toStdString());
    buffer.insert(buffer.end(), add_to_inventoryGainList.begin(),
                  add_to_inventoryGainList.end());
  }

  toast_->SetContent(buffer);
  toast_->SetVisible(true);
}

void OverMapLogic::addQuery(const QueryType &queryType) {
  queryList.push_back(queryType);
  updateQueryList();
}

void OverMapLogic::removeQuery(const QueryType &queryType) {
  vectorremoveOne(queryList, queryType);
  updateQueryList();
}

void OverMapLogic::updateQueryList() {
  QStringList queryStringList;
  unsigned int index = 0;
  while (index < queryList.size() && index < 5) {
    switch (queryList.at(index)) {
      case QueryType_Seed:
        queryStringList << tr("Planting seed...");
        break;
      case QueryType_CollectPlant:
        queryStringList << tr("Collecting plant...");
        break;
      default:
        queryStringList << tr("Unknown action...");
        break;
    }
    index++;
  }
  persistant_tipString = queryStringList.join("<br />");
  persistant_tip->SetText(persistant_tipString);
}

bool OverMapLogic::stopped_in_front_of_check_bot(
    CatchChallenger::Map_client *map, uint8_t x, uint8_t y) {
  if (map->bots.find(std::pair<uint8_t, uint8_t>(x, y)) == map->bots.cend())
    return false;
  showTip(
      tr("To interact with the bot press <i><b>Enter</b></i>").toStdString());
  return true;
}

bool OverMapLogic::actionOnCheckBot(CatchChallenger::Map_client *map, uint8_t x,
                                    uint8_t y) {
  if (map->bots.find(std::pair<uint8_t, uint8_t>(x, y)) == map->bots.cend())
    return false;
  actualBot = map->bots.at(std::pair<uint8_t, uint8_t>(x, y));
  isInQuest = false;
  goToBotStep(1);
  return true;
}

/*void OverMapLogic::on_clanActionLeave_clicked()
{
    actionClan.push_back(ActionClan_Leave);
    client->leaveClan();
}

void OverMapLogic::on_clanActionDissolve_clicked()
{
    actionClan.push_back(ActionClan_Dissolve);
    client->dissolveClan();
}

void OverMapLogic::on_clanActionInvite_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player
pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(),
&ok).toStdString(); if(ok && !text.empty())
    {
        actionClan.push_back(ActionClan_Invite);
        client->inviteClan(text);
    }
}

void OverMapLogic::on_clanActionEject_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player
pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(),
&ok).toStdString(); if(ok && !text.empty())
    {
        actionClan.push_back(ActionClan_Eject);
        client->ejectClan(text);
    }
}*/

/*void OverMapLogic::have_inventory(const std::unordered_map<uint16_t,uint32_t>
&items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    CatchChallenger::Player_private_and_public_informations
&playerInformations=connexionManager->client->get_player_informations();
    playerInformations.items=items;
    playerInformations.warehouse_items=warehouse_items;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::have_inventory()";
    #endif
    haveInventory=true;
    updateConnectingStatus();
}

void OverMapLogic::load_inventory()
{
    const CatchChallenger::Player_private_and_public_informations
&playerInformations=connexionManager->client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::load_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->inventoryInformation->setVisible(false);
    ui->inventoryUse->setVisible(false);
    ui->inventoryDestroy->setVisible(false);
    ui->inventory->clear();
    items_graphical.clear();
    items_to_graphical.clear();
    auto i=playerInformations.items.begin();
    while (i!=playerInformations.items.cend())
    {
        bool show=!inSelection;
        if(inSelection)
        {
            switch(waitedObjectType)
            {
                case ObjectType_Seed:
                    //reputation requierement control is into
load_plant_inventory() NOT: on_listPlantList_itemSelectionChanged()
                    if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(i->first)!=QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
                        show=true;
                break;
                case ObjectType_UseInFight:
                    if(fightEngine.isInFightWithWild() &&
CommonDatapack::commonDatapack.items.trap.find(i->first)!=CommonDatapack::commonDatapack.items.trap.cend())
                        show=true;
                    else
if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(i->first)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
                        show=true;
                    else
                        show=false;
                break;
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            QListWidgetItem *item=new QListWidgetItem();
            items_to_graphical[i->first]=item;
            items_graphical[item]=i->first;
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(i->first)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(i->first).image);
                if(i->second>1)
                    item->setText(QString::number(i->second));
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(i->first).name));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                if(i->second>1)
                    item->setText(QStringLiteral("id: %1
(x%2)").arg(i->first).arg(i->second)); else item->setText(QStringLiteral("id:
%1").arg(i->first));
            }
            ui->inventory->addItem(item);
        }
        ++i;
    }
}*/

void OverMapLogic::add_to_inventory_slot(
    const std::unordered_map<uint16_t, uint32_t> &items) {
  add_to_inventory(items);
}

void OverMapLogic::add_to_inventory_slotpair(const uint16_t &item,
                                             const uint32_t &quantity,
                                             const bool &showGain) {
  add_to_inventory(item, quantity, showGain);
}

void OverMapLogic::add_to_inventory(const uint16_t &item,
                                    const uint32_t &quantity,
                                    const bool &showGain) {
  std::vector<std::pair<uint16_t, uint32_t> > items;
  items.push_back(std::pair<uint16_t, uint32_t>(item, quantity));
  add_to_inventory(items, showGain);
}

void OverMapLogic::add_to_inventory(
    const std::vector<std::pair<uint16_t, uint32_t> > &items,
    const bool &showGain) {
  unsigned int index = 0;
  std::unordered_map<uint16_t, uint32_t> tempHash;
  while (index < items.size()) {
    tempHash[items.at(index).first] = items.at(index).second;
    index++;
  }
  add_to_inventory(tempHash, showGain);
}

void OverMapLogic::add_to_inventory(
    const std::unordered_map<uint16_t, uint32_t> &items, const bool &showGain) {
  // TODO(lanstat): Prevent convert to b64 png
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  if (items.empty()) return;
  if (showGain) {
    std::vector<std::string> objects;

    for (const auto &n : items) {
      const uint16_t &item = n.first;
      const uint32_t &quantity = n.second;
      QPixmap image;
      std::string name;
      if (QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(item) !=
          QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend()) {
        image =
            QtDatapackClientLoader::datapackLoader->getItemExtra(item).image;
        name = QtDatapackClientLoader::datapackLoader->get_itemsExtra()
                   .at(item)
                   .name;
      } else {
        image = QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
        name = "id: %1" + std::to_string(item);
      }

      image = image.scaled(24, 24);
      QByteArray byteArray;
      QBuffer buffer(&byteArray);
      image.save(&buffer, "PNG");
      if (objects.size() < 16) {
        if (item > 1)
          objects.push_back("<b>" + std::to_string(quantity) + "x</b> " + name +
                            " <img src=\"data:image/png;base64," +
                            QString(byteArray.toBase64()).toStdString() +
                            "\" />");
        else
          objects.push_back(name + " <img src=\"data:image/png;base64," +
                            QString(byteArray.toBase64()).toStdString() +
                            "\" />");
      }
    }
    if (objects.size() >= 16) objects.push_back("...");
    add_to_inventoryGainList.push_back(stringimplode(objects, ", "));
    add_to_inventoryGainTime.push_back(QTime::currentTime());
    OverMapLogic::showGain();
  }

  PlayerInfo::GetInstance()->AddInventory(items);
  PlayerInfo::GetInstance()->Notify();
  /*load_inventory();
  load_plant_inventory();
  on_listCraftingList_itemSelectionChanged();*/
}

void OverMapLogic::remove_to_inventory(const uint16_t &itemId,
                                       const uint32_t &quantity) {
  std::unordered_map<uint16_t, uint32_t> items;
  items[itemId] = quantity;
  remove_to_inventory(items);
}

void OverMapLogic::remove_to_inventory_slotpair(const uint16_t &itemId,
                                                const uint32_t &quantity) {
  remove_to_inventory(itemId, quantity);
}

void OverMapLogic::remove_to_inventory_slot(
    const std::unordered_map<uint16_t, uint32_t> &items) {
  remove_to_inventory(items);
}

void OverMapLogic::remove_to_inventory(
    const std::unordered_map<uint16_t, uint32_t> &items) {
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  for (const auto &n : items) {
    const uint16_t &item = n.first;
    const uint32_t &quantity = n.second;
    // add really to the list
    if (playerInformations.items.find(item) !=
        playerInformations.items.cend()) {
      if (playerInformations.items.at(item) <= quantity)
        playerInformations.items.erase(item);
      else
        playerInformations.items[item] -= quantity;
    }
  }
  /*load_inventory();
  load_plant_inventory();*/
}

/*void OverMapLogic::load_plant_inventory()
{
    const CatchChallenger::Player_private_and_public_informations
&playerInformations=connexionManager->client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::load_plant_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->listPlantList->clear();
    plants_items_graphical.clear();
    plants_items_to_graphical.clear();
    auto i=playerInformations.items.begin();
    while(i!=playerInformations.items.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(i->first)!=
                QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
        {
            const uint8_t
&plantId=QtDatapackClientLoader::datapackLoader->itemToPlants.at(i->first);
            QListWidgetItem *item;
            item=new QListWidgetItem();
            plants_items_to_graphical[plantId]=item;
            plants_items_graphical[item]=plantId;
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(i->first)!=
                    QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[i->first].image);
                item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[i->first].name)+
                        "\n"+tr("Quantity: %1").arg(i->second));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                item->setText(QStringLiteral("item id:
%1").arg(i->first)+"\n"+tr("Quantity: %1").arg(i->second));
            }
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId).requirements.reputation))
            {
                item->setText(item->text()+"\n"+tr("You don't have the
requirements")); item->setFont(disableIntoListFont);
                item->setForeground(disableIntoListBrush);
            }
            ui->listPlantList->addItem(item);
        }
        ++i;
    }
}
*/

void OverMapLogic::botFight(const uint16_t &fightId) {
  // todo
  std::cerr << "OverMapLogic::botFight " << fightId << std::endl;
}

void OverMapLogic::wildFightCollision(CatchChallenger::Map_client *map,
                                      const uint8_t &x, const uint8_t &y) {
  std::cerr << "OverMapLogic::wildFightCollision" << std::endl;
  auto battle = Globals::GetBattleScene();
  battle->WildFightInitialize(map, x, y);
  SceneManager::GetInstance()->PushScene(battle);
}

void OverMapLogic::botFightCollision(CatchChallenger::Map_client *map,
                                     const uint8_t &x, const uint8_t &y,
                                     const uint16_t &fightId) {
  std::cerr << "OverMapLogic::botFightCollision " << std::endl;

  auto battle = Globals::GetBattleScene();
  battle->BotFightInitialize(map, x, y, fightId);
  SceneManager::GetInstance()->PushScene(battle);
}

void OverMapLogic::BattleFinished() {}

void OverMapLogic::insert_plant(const uint32_t &mapId, const uint8_t &x,
                                const uint8_t &y, const uint8_t &plant_id,
                                const uint16_t &seconds_to_mature) {
  Q_UNUSED(plant_id);
  Q_UNUSED(seconds_to_mature);
  if (mapId >=
      (uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size()) {
    qDebug() << "MapController::insert_plant() mapId greater than "
                "QtDatapackClientLoader::datapackLoader->maps.size()";
    return;
  }
  cancelAllPlantQuery(ccmap->mapController.mapIdToString(mapId), x, y);
}

void OverMapLogic::remove_plant(const uint32_t &mapId, const uint8_t &x,
                                const uint8_t &y) {
  if (mapId >=
      (uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size()) {
    qDebug() << "MapController::insert_plant() mapId greater than "
                "QtDatapackClientLoader::datapackLoader->maps.size()";
    return;
  }
  cancelAllPlantQuery(ccmap->mapController.mapIdToString(mapId), x, y);
}

void OverMapLogic::cancelAllPlantQuery(const std::string map, const uint8_t x,
                                       const uint8_t y) {
  unsigned int index;
  index = 0;
  while (index < seed_in_waiting.size()) {
    const SeedInWaiting &seedInWaiting = seed_in_waiting.at(index);
    if (seedInWaiting.map == map && seedInWaiting.x == x &&
        seedInWaiting.y == y) {
      seed_in_waiting[index].map = std::string();
      seed_in_waiting[index].x = 0;
      seed_in_waiting[index].y = 0;
    }
    index++;
  }
  index = 0;
  while (index < plant_collect_in_waiting.size()) {
    const ClientPlantInCollecting &clientPlantInCollecting =
        plant_collect_in_waiting.at(index);
    if (clientPlantInCollecting.map == map && clientPlantInCollecting.x == x &&
        clientPlantInCollecting.y == y) {
      plant_collect_in_waiting[index].map = std::string();
      plant_collect_in_waiting[index].x = 0;
      plant_collect_in_waiting[index].y = 0;
    }
    index++;
  }
}

void OverMapLogic::seed_planted(const bool &ok) {
  removeQuery(QueryType_Seed);
  if (ok) {
    /// \todo add to the map here, and don't send on the server
    showTip(tr("Seed correctly planted").toStdString());
    // do the rewards
    {
      const uint16_t &itemId = seed_in_waiting.front().seedItemId;
      if (QtDatapackClientLoader::datapackLoader->get_itemToPlants().find(
              itemId) ==
          QtDatapackClientLoader::datapackLoader->get_itemToPlants().cend()) {
        qDebug() << "Item is not a plant";
        emit error(tr("Internal error").toStdString() + ", file: " +
                   std::string(__FILE__) + ":" + std::to_string(__LINE__));
        return;
      }
      const uint8_t &plant =
          QtDatapackClientLoader::datapackLoader->get_itemToPlants().at(itemId);
      appendReputationRewards(
          CatchChallenger::CommonDatapack::commonDatapack.get_plants()
              .at(plant)
              .rewards.reputation);
    }
  } else {
    if (!seed_in_waiting.front().map.empty()) {
      ccmap->mapController.remove_plant_full(seed_in_waiting.front().map,
                                             seed_in_waiting.front().x,
                                             seed_in_waiting.front().y);
      cancelAllPlantQuery(seed_in_waiting.front().map,
                          seed_in_waiting.front().x, seed_in_waiting.front().y);
    }
    add_to_inventory(seed_in_waiting.front().seedItemId, 1, false);
    showTip(tr("Seed cannot be planted").toStdString());
  }
  seed_in_waiting.erase(seed_in_waiting.cbegin());
}

void OverMapLogic::plant_collected(const CatchChallenger::Plant_collect &stat) {
  removeQuery(QueryType_CollectPlant);
  switch (stat) {
    case CatchChallenger::Plant_collect_correctly_collected:
      // see to optimise
      // CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true
      // and use the internal random number list
      showTip(
          tr("Plant collected").toStdString());  // the item is send by another
                                                 // message with the protocol
      break;
    /*case CatchChallenger::Plant_collect_empty_dirt:
        showTip(tr("Try collect an empty dirt").toStdString());
    break;
    case CatchChallenger::Plant_collect_owned_by_another_player:
        showTip(tr("This plant had been planted recently by another
    player").toStdString());
        ccmap->mapController.insert_plant_full(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y,
                                         plant_collect_in_waiting.front().plant_id,plant_collect_in_waiting.front().seconds_to_mature);
        cancelAllPlantQuery(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y);
    break;
    case CatchChallenger::Plant_collect_impossible:
        showTip(tr("This plant can't be collected").toStdString());
        ccmap->mapController.insert_plant_full(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y,
                                         plant_collect_in_waiting.front().plant_id,plant_collect_in_waiting.front().seconds_to_mature);
        cancelAllPlantQuery(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y);
    break;*/
    default:
      qDebug() << "OverMapLogic::plant_collected(): unknown return";
      break;
  }
}

/*void OverMapLogic::on_toolButton_quit_plants_clicked()
{
    ui->listPlantList->reset();
    if(inSelection)
    {
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        objectSelection(false,0);
    }
    else
        ui->stackedWidget->setCurrentWidget(ui->page_inventory);
    on_listPlantList_itemSelectionChanged();
}

void OverMapLogic::on_plantUse_clicked()
{
    QList<QListWidgetItem *> items=ui->listPlantList->selectedItems();
    if(items.size()!=1)
        return;
    on_listPlantList_itemActivated(items.first());
}

void OverMapLogic::on_listPlantList_itemActivated(QListWidgetItem *item)
{
    if(plants_items_graphical.find(item)==plants_items_graphical.cend())
    {
        qDebug() << "OverMapLogic::on_inventory_itemActivated(): activated item
not found"; return;
    }
    if(!inSelection)
    {
        qDebug() << "OverMapLogic::on_inventory_itemActivated(): not in
selection, use is not done actually"; return;
    }
    objectSelection(true,CatchChallenger::CommonDatapack::commonDatapack.plants[plants_items_graphical[item]].itemUsed);
}

void OverMapLogic::on_pushButton_interface_crafting_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_crafting);
}

void OverMapLogic::on_toolButton_quit_crafting_clicked()
{
    ui->listCraftingList->reset();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    on_listCraftingList_itemSelectionChanged();
}

void OverMapLogic::on_listCraftingList_itemSelectionChanged()
{
    const CatchChallenger::Player_private_and_public_informations
&playerInformations=client->get_player_informations_ro();
    ui->listCraftingMaterials->clear();
    QList<QListWidgetItem *>
displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
    {
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        ui->labelCraftingDetails->setText(tr("Select a recipe"));
        ui->craftingUse->setVisible(false);
        return;
    }
    QListWidgetItem *itemMaterials=displayedItems.first();
    const CatchChallenger::CraftingRecipe
&content=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[crafting_recipes_items_graphical[itemMaterials]];

    qDebug() << "on_listCraftingList_itemSelectionChanged() load the name";
    //load the name
    QString name;
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.doItemId)!=
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        name=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].name);
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->QtitemsExtra[content.doItemId].image);
    }
    else
    {
        name=tr("Unknow item (%1)").arg(content.doItemId);
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
    }
    ui->labelCraftingDetails->setText(tr("Name: <b>%1</b><br /><br />Success:
<b>%2%</b><br /><br />Result:
<b>%3</b>").arg(name).arg(content.success).arg(content.quantity));

    //load the materials
    bool haveMaterials=true;
    unsigned int index=0;
    QString nameMaterials;
    QListWidgetItem *item;
    uint32_t quantity;
    while(index<content.materials.size())
    {
        //load the material item
        item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.materials.at(index).item)!=
                QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        {
            nameMaterials=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].name);
            item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[content.materials.at(index).item].image);
        }
        else
        {
            nameMaterials=tr("Unknow item
(%1)").arg(content.materials.at(index).item);
            item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        }

        //load the quantity into the inventory
        quantity=0;
        if(playerInformations.items.find(content.materials.at(index).item)!=playerInformations.items.cend())
            quantity=playerInformations.items.at(content.materials.at(index).item);

        //load the display
        item->setText(tr("Needed: %1 %2\nIn the inventory: %3
%4").arg(content.materials.at(index).quantity).arg(nameMaterials).arg(quantity).arg(nameMaterials));
        if(quantity<content.materials.at(index).quantity)
        {
            item->setFont(disableIntoListFont);
            item->setForeground(disableIntoListBrush);
        }

        if(quantity<content.materials.at(index).quantity)
            haveMaterials=false;

        ui->listCraftingMaterials->addItem(item);
        ui->craftingUse->setVisible(haveMaterials);
        index++;
    }
}

void OverMapLogic::on_craftingUse_clicked()
{
    const CatchChallenger::Player_private_and_public_informations
&playerInformations=client->get_player_informations_ro();
    //recipeInUsing
    QList<QListWidgetItem *>
displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
        return;
    QListWidgetItem *selectedItem=displayedItems.first();
    const CatchChallenger::CraftingRecipe
&content=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[crafting_recipes_items_graphical[selectedItem]];

    QStringList mIngredients;
    QString mRecipe;
    QString mProduct;
    //load the materials
    unsigned int index=0;
    while(index<content.materials.size())
    {
        if(playerInformations.items.find(content.materials.at(index).item)==playerInformations.items.cend())
            return;
        if(playerInformations.items.at(content.materials.at(index).item)<content.materials.at(index).quantity)
            return;
        uint32_t sub_index=0;
        while(sub_index<content.materials.at(index).quantity)
        {
            mIngredients.push_back(QUrl::fromLocalFile(
                                       QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].imagePath)
                                   ).toEncoded());
            sub_index++;
        }
        index++;
    }
    index=0;
    std::vector<std::pair<uint16_t,uint32_t> > recipeUsage;
    while(index<content.materials.size())
    {
        std::pair<uint16_t,uint32_t> pair;
        pair.first=content.materials.at(index).item;
        pair.second=content.materials.at(index).quantity;
        remove_to_inventory(pair.first,pair.second);
        recipeUsage.push_back(pair);
        index++;
    }
    materialOfRecipeInUsing.push_back(recipeUsage);
    //the product do
    std::pair<uint16_t,uint32_t> pair;
    pair.first=content.doItemId;
    pair.second=content.quantity;
    productOfRecipeInUsing.push_back(pair);
    mProduct=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].imagePath)).toEncoded();
    mRecipe=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.itemToLearn].imagePath)).toEncoded();
    //update the UI
    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
    //send to the network
    client->useRecipe(crafting_recipes_items_graphical.at(selectedItem));
    appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(
                                crafting_recipes_items_graphical.at(selectedItem)).rewards.reputation);
    //create animation widget
    if(animationWidget!=NULL)
        delete animationWidget;
    if(qQuickViewContainer!=NULL)
        delete qQuickViewContainer;
    animationWidget=new QQuickView();
    qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
    qQuickViewContainer->setMinimumSize(QSize(800,600));
    qQuickViewContainer->setMaximumSize(QSize(800,600));
    qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
    ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
    //show the animation
    previousAnimationWidget=ui->stackedWidget->currentWidget();
    ui->stackedWidget->setCurrentWidget(ui->page_animation);
    if(craftingAnimationObject!=NULL)
        delete craftingAnimationObject;
    craftingAnimationObject=new CraftingAnimation(mIngredients,
                                                  mRecipe,mProduct,
                                                  QUrl::fromLocalFile(QString::fromStdString(
              playerBackImagePath)).toEncoded());
    animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
    animationWidget->rootContext()->setContextProperty("craftingAnimationObject",craftingAnimationObject);
    const QString
datapackQmlFile=QString::fromStdString(client->datapackPathBase())+"qml/crafting-animation.qml";
    if(QFile(datapackQmlFile).exists())
        animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
    else
        animationWidget->setSource(QStringLiteral("qrc:/qml/crafting-animation.qml"));
}

void OverMapLogic::on_listCraftingMaterials_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    ui->craftingUse->clicked();
}*/

/*void OverMapLogic::recipeUsed(const CatchChallenger::RecipeUsage &recipeUsage)
{
    switch(recipeUsage)
    {
        case CatchChallenger::RecipeUsage_ok:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            add_to_inventory(productOfRecipeInUsing.front().first,productOfRecipeInUsing.front().second);
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        break;
        case CatchChallenger::RecipeUsage_impossible:
        {
            unsigned int index=0;
            while(index<materialOfRecipeInUsing.front().size())
            {
                add_to_inventory(materialOfRecipeInUsing.front().at(index).first,
                                 materialOfRecipeInUsing.front().at(index).first,false);
                index++;
            }
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        }
        break;
        case RecipeUsage_failed:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
        break;
        default:
        qDebug() << "recipeUsed() unknow code";
        return;
    }
}*/

void OverMapLogic::appendReputationRewards(
    const std::vector<CatchChallenger::ReputationRewards> &reputationList) {
  unsigned int index = 0;
  while (index < reputationList.size()) {
    const CatchChallenger::ReputationRewards &reputationRewards =
        reputationList.at(index);
    appendReputationPoint(
        CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
            .at(reputationRewards.reputationId)
            .name,
        reputationRewards.point);
    index++;
  }
}

// reputation
void OverMapLogic::appendReputationPoint(const std::string &type,
                                         const int32_t &point) {
  if (point == 0) return;
  if (QtDatapackClientLoader::datapackLoader->get_reputationNameToId().find(
          type) ==
      QtDatapackClientLoader::datapackLoader->get_reputationNameToId().cend()) {
    emit error("Unknow reputation: " + type);
    return;
  }
  const uint8_t &reputationId =
      QtDatapackClientLoader::datapackLoader->get_reputationNameToId().at(type);
  CatchChallenger::PlayerReputation playerReputation;
  if (connexionManager->client->player_informations.reputation.find(
          reputationId) !=
      connexionManager->client->player_informations.reputation.cend())
    playerReputation =
        connexionManager->client->player_informations.reputation.at(
            reputationId);
  else {
    playerReputation.point = 0;
    playerReputation.level = 0;
  }
  CatchChallenger::PlayerReputation oldPlayerReputation = playerReputation;
  int32_t old_level = playerReputation.level;
  CatchChallenger::FacilityLib::appendReputationPoint(
      &playerReputation, point,
      CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(
          reputationId));
  if (oldPlayerReputation.level == playerReputation.level &&
      oldPlayerReputation.point == playerReputation.point)
    return;
  if (connexionManager->client->player_informations.reputation.find(
          reputationId) !=
      connexionManager->client->player_informations.reputation.cend())
    connexionManager->client->player_informations.reputation[reputationId] =
        playerReputation;
  else
    connexionManager->client->player_informations.reputation[reputationId] =
        playerReputation;
  const std::string &reputationCodeName =
      CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
          .at(reputationId)
          .name;
  if (old_level < playerReputation.level) {
    if (QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
            reputationCodeName) !=
        QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend())
      showTip(
          tr("You have better reputation into %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_reputationExtra()
                      .at(reputationCodeName)
                      .name))
              .toStdString());
    else
      showTip(
          tr("You have better reputation into %1").arg("???").toStdString());
  } else if (old_level > playerReputation.level) {
    if (QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
            reputationCodeName) !=
        QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend())
      showTip(
          tr("You have worse reputation into %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_reputationExtra()
                      .at(reputationCodeName)
                      .name))
              .toStdString());
    else
      showTip(tr("You have worse reputation into %1").arg("???").toStdString());
  }
}

void OverMapLogic::objectSelection(const bool &ok, const uint16_t &itemId,
                                   const uint32_t &quantity) {
  abort();
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  ObjectType waitedObjectType = ObjectType_All;
  ObjectType tempWaitedObjectType = waitedObjectType;
  waitedObjectType = ObjectType_All;
  switch (tempWaitedObjectType) {
    case ObjectType_ItemEvolutionOnMonster:
    case ObjectType_ItemLearnOnMonster:
    case ObjectType_ItemOnMonster:
    case ObjectType_ItemOnMonsterOutOfFight: {
      /*const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
      const uint16_t item=objectInUsing.back();
      objectInUsing.erase(objectInUsing.cbegin());
      if(!ok)
      {
          ui->stackedWidget->setCurrentWidget(ui->page_inventory);
          ui->inventoryUse->setText(tr("Select"));
          if(CatchChallenger::CommonDatapack::commonDatapack.items.item.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
              if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                  add_to_inventory(item,1,false);
          break;
      }
      const CatchChallenger::PlayerMonster * const
      monster=connexionManager->client->monsterByPosition(monsterPosition);
      if(monster==NULL)
      {
          if(CatchChallenger::CommonDatapack::commonDatapack.items.item.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
              if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                  add_to_inventory(item,1,false);
          break;
      }
      const CatchChallenger::Monster
      &monsterInformations=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster->monster);
      const QtDatapackClientLoader::MonsterExtra
      &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster->monster);
      if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
      {
          uint8_t monsterEvolutionPostion=0;
          const CatchChallenger::Monster
      &monsterInformationsEvolution=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(item).at(monster->monster));
          const QtDatapackClientLoader::MonsterExtra
      &monsterInformationsEvolutionExtra=
                  QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                      CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(item).at(monster->monster)
                      );
          abort();
          //create animation widget
          if(animationWidget!=NULL)
              delete animationWidget;
          if(qQuickViewContainer!=NULL)
              delete qQuickViewContainer;
          animationWidget=new QQuickView();
          qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
          qQuickViewContainer->setMinimumSize(QSize(800,600));
          qQuickViewContainer->setMaximumSize(QSize(800,600));
          qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
          ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
          //show the animation
          ui->stackedWidget->setCurrentWidget(ui->page_animation);
          previousAnimationWidget=ui->page_map;
          if(baseMonsterEvolution!=NULL)
              delete baseMonsterEvolution;
          if(targetMonsterEvolution!=NULL)
              delete targetMonsterEvolution;
          baseMonsterEvolution=new
      QmlMonsterGeneralInformations(monsterInformations,monsterInformationsExtra);
          targetMonsterEvolution=new
      QmlMonsterGeneralInformations(monsterInformationsEvolution,monsterInformationsEvolutionExtra);
          if(evolutionControl!=NULL)
              delete evolutionControl;
          evolutionControl=new
      EvolutionControl(monsterInformations,monsterInformationsExtra,monsterInformationsEvolution,monsterInformationsEvolutionExtra);
          animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
          animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
          animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(false));
          animationWidget->rootContext()->setContextProperty("itemEvolution",
                                                             QUrl::fromLocalFile(
                                                                 QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra
                                                                                        .at(item).imagePath)));
          animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
          animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
          const std::string
      datapackQmlFile=client->datapackPathBase()+"qml/evolution-animation.qml";
          if(QFile(QString::fromStdString(datapackQmlFile)).exists())
              animationWidget->setSource(QUrl::fromLocalFile(QString::fromStdString(datapackQmlFile)));
          else
              animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
          client->useObjectOnMonsterByPosition(item,monsterPosition);
          if(!fightEngine.useObjectOnMonsterByPosition(item,monsterPosition))
          {
              std::cerr << "fightEngine.useObjectOnMonsterByPosition() Bug at "
      << __FILE__ << ":" << __LINE__ << std::endl; #ifdef
      CATCHCHALLENGER_EXTRA_CHECK abort(); #endif
          }
          load_monsters();
          return;
      }
      else
      {
          abort();
          ui->stackedWidget->setCurrentWidget(ui->page_inventory);
          ui->inventoryUse->setText(tr("Select"));
          if(fightEngine.useObjectOnMonsterByPosition(item,monsterPosition))
          {
              showTip(tr("Using <b>%1</b> on <b>%2</b>")
                      .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(item).name))
                      .arg(QString::fromStdString(monsterInformationsExtra.name))
                      .toStdString());
              client->useObjectOnMonsterByPosition(item,monsterPosition);
              load_monsters();
              checkEvolution();
          }
          else
          {
              showTip(tr("Failed to use <b>%1</b> on <b>%2</b>")
                      .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(item).name))
                      .arg(QString::fromStdString(monsterInformationsExtra.name))
                      .toStdString());
              if(CatchChallenger::CommonDatapack::commonDatapack.items.item.find(item)!=
                      CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
                  if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                      add_to_inventory(item,1,false);
          }
      }*/
      abort();
    } break;
    case ObjectType_Sell: {
      /*ui->stackedWidget->setCurrentWidget(ui->page_map);
      ui->inventoryUse->setText(tr("Select"));
      if(!ok)
          break;
      if(playerInformations.items.find(itemId)==playerInformations.items.cend())
      {
          qDebug() << "item id is not into the inventory";
          break;
      }
      if(playerInformations.items.at(itemId)<quantity)
      {
          qDebug() << "item id have not the quantity";
          break;
      }
      remove_to_inventory(itemId,quantity);
      ItemToSellOrBuy tempItem;
      tempItem.object=itemId;
      tempItem.quantity=quantity;
      tempItem.price=CatchChallenger::CommonDatapack::commonDatapack.items.item.at(itemId).price/2;
      itemsToSell.push_back(tempItem);
      client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
      load_inventory();
      load_plant_inventory();*/
      abort();
    } break;
    case ObjectType_SellToMarket: {
      /*            ui->inventoryUse->setText(tr("Select"));
                  ui->stackedWidget->setCurrentWidget(ui->page_market);
                  if(!ok)
                      break;
                  if(playerInformations.items.find(itemId)==playerInformations.items.cend())
                  {
                      qDebug() << "item id is not into the inventory";
                      break;
                  }
                  if(playerInformations.items.at(itemId)<quantity)
                  {
                      qDebug() << "item id have not the quantity";
                      break;
                  }
                  uint32_t suggestedPrice=50;
                  if(CommonDatapack::commonDatapack.items.item.find(itemId)!=CommonDatapack::commonDatapack.items.item.cend())
                      suggestedPrice=CommonDatapack::commonDatapack.items.item.at(itemId).price;
                  GetPrice getPrice(this,suggestedPrice);
                  getPrice.exec();
                  if(!getPrice.isOK())
                      break;
                  client->putMarketObject(itemId,quantity,getPrice.price());
                  marketPutCashInSuspend=getPrice.price();
                  remove_to_inventory(itemId,quantity);
                  std::pair<uint16_t,uint32_t> pair;
                  pair.first=itemId;
                  pair.second=quantity;
                  marketPutObjectInSuspendList.push_back(pair);
                  load_inventory();
                  load_plant_inventory();*/
      abort();
    } break;
    case ObjectType_Trade:
      /*            ui->inventoryUse->setText(tr("Select"));
                  ui->stackedWidget->setCurrentWidget(ui->page_trade);
                  if(!ok)
                      break;
                  if(playerInformations.items.find(itemId)==playerInformations.items.cend())
                  {
                      qDebug() << "item id is not into the inventory";
                      break;
                  }
                  if(playerInformations.items.at(itemId)<quantity)
                  {
                      qDebug() << "item id have not the quantity";
                      break;
                  }
                  client->addObject(itemId,quantity);
                  playerInformations.items[itemId]-=quantity;
                  if(playerInformations.items.at(itemId)==0)
                      playerInformations.items.erase(itemId);
                  if(tradeCurrentObjects.find(itemId)!=tradeCurrentObjects.cend())
                      tradeCurrentObjects[itemId]+=quantity;
                  else
                      tradeCurrentObjects[itemId]=quantity;
                  load_inventory();
                  load_plant_inventory();
                  tradeUpdateCurrentObject();*/
      abort();
      break;
    case ObjectType_MonsterToLearn: {
      /*ui->stackedWidget->setCurrentWidget(ui->page_map);
      ui->inventoryUse->setText(tr("Select"));
      load_monsters();
      if(!ok)
          return;
      ui->stackedWidget->setCurrentWidget(ui->page_learn);
      monsterPositionToLearn=static_cast<uint8_t>(itemId);
      if(!showLearnSkillByPosition(monsterPositionToLearn))
      {
          newError(tr("Internal error").toStdString()+", file:
      "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Unable to load the
      right monster"); return;
      }*/
      abort();
    } break;
    case ObjectType_MonsterToFight:
    case ObjectType_MonsterToFightKO: {
      /*            ui->inventoryUse->setText(tr("Select"));
                  ui->stackedWidget->setCurrentWidget(ui->page_battle);
                  load_monsters();
                  if(!ok)
                      return;
                  resetPosition(true,false,true);
                  //do copie here because the call of changeOfMonsterInFight
         apply the skill/buff effect const uint8_t
         monsterPosition=static_cast<uint8_t>(itemId); const PlayerMonster
         *tempMonster=fightEngine.monsterByPosition(monsterPosition);
                  if(tempMonster==NULL)
                  {
                      qDebug() << "Monster not found";
                      return;
                  }
                  PlayerMonster copiedMonster=*tempMonster;
                  if(!fightEngine.changeOfMonsterInFight(monsterPosition))
                      return;
                  client->changeOfMonsterInFightByPosition(monsterPosition);
                  PlayerMonster * playerMonster=fightEngine.getCurrentMonster();
                  init_current_monster_display(&copiedMonster);
                  ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                  if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(playerMonster->monster)!=
                          QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
                  {
                      ui->labelFightEnter->setText(tr("Go %1")
                                                   .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(playerMonster->monster).name)));
                      ui->labelFightMonsterBottom->setPixmap(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(playerMonster->monster).back.scaled(160,160));
                  }
                  else
                  {
                      ui->labelFightEnter->setText(tr("You change of monster"));
                      ui->labelFightMonsterBottom->setPixmap(QPixmap(":/CC/images/monsters/default/back.png"));
                  }
                  ui->pushButtonFightEnterNext->setVisible(false);
                  moveType=MoveType_Enter;
                  battleStep=BattleStep_Presentation;
                  monsterBeforeMoveForChangeInWaiting=true;
                  moveFightMonsterBottom();*/
    } break;
    case ObjectType_MonsterToTradeToMarket: {
      /*            ui->inventoryUse->setText(tr("Select"));
                  ui->stackedWidget->setCurrentWidget(ui->page_market);
                  load_monsters();
                  if(!ok)
                      break;
                  std::vector<PlayerMonster>
         playerMonster=fightEngine.getPlayerMonster();
                  if(playerMonster.size()<=1)
                  {
                      QMessageBox::warning(this,tr("Warning"),tr("You can't
         trade your last monster")); break;
                  }
                  const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
                  if(!fightEngine.remainMonstersToFightWithoutThisMonster(monsterPosition))
                  {
                      QMessageBox::warning(this,tr("Warning"),tr("You don't have
         more monster valid")); break;
                  }
                  //get the right monster
                  GetPrice getPrice(this,15000);
                  getPrice.exec();
                  if(!getPrice.isOK())
                      break;
                  marketPutMonsterList.push_back(playerMonster.at(monsterPosition));
                  marketPutMonsterPlaceList.push_back(monsterPosition);
                  fightEngine.removeMonsterByPosition(monsterPosition);
                  client->putMarketMonsterByPosition(monsterPosition,getPrice.price());
                  marketPutCashInSuspend=getPrice.price();*/
    } break;
    case ObjectType_MonsterToTrade: {
      /*            ui->inventoryUse->setText(tr("Select"));
                  load_monsters();
                  const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
                  if(waitedObjectType==ObjectType_MonsterToLearn)
                  {
                      ui->stackedWidget->setCurrentWidget(ui->page_learn);
                      monsterPositionToLearn=monsterPosition;
                      return;
                  }
                  ui->stackedWidget->setCurrentWidget(ui->page_trade);
                  if(!ok)
                      break;
                  std::vector<PlayerMonster>
         playerMonster=fightEngine.getPlayerMonster();
                  if(playerMonster.size()<=1)
                  {
                      QMessageBox::warning(this,tr("Warning"),tr("You can't
         trade your last monster")); break;
                  }
                  if(!fightEngine.remainMonstersToFightWithoutThisMonster(monsterPosition))
                  {
                      QMessageBox::warning(this,tr("Warning"),tr("You don't have
         more monster valid")); break;
                  }
                  //get the right monster
                  tradeCurrentMonstersPosition.push_back(monsterPosition);
                  tradeCurrentMonsters.push_back(playerMonster.at(monsterPosition));
                  fightEngine.removeMonsterByPosition(monsterPosition);
                  client->addMonsterByPosition(monsterPosition);
                  QListWidgetItem *item=new QListWidgetItem();
                  item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(tradeCurrentMonsters.back().monster).name));
                  item->setToolTip(tr("Level:
         %1").arg(tradeCurrentMonsters.back().level));
                  item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(tradeCurrentMonsters.back().monster).front);
                  ui->tradePlayerMonsters->addItem(item);*/
    } break;
    case ObjectType_Seed: {
    } break;
    case ObjectType_UseInFight: {
      /*            ui->inventoryUse->setText(tr("Select"));
                  ui->stackedWidget->setCurrentWidget(ui->page_battle);
                  load_inventory();
                  if(!ok)
                      break;
                  const CatchChallenger::Player_private_and_public_informations
         &playerInformations=client->get_player_informations_ro();
                  if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
                  {
                      QMessageBox::warning(this,tr("Error"),tr("You have already
         the maximum number of monster into you warehouse")); break;
                  }
                  if(playerInformations.items.find(itemId)==playerInformations.items.cend())
                  {
                      qDebug() << "item id is not into the inventory";
                      break;
                  }
                  if(playerInformations.items.at(itemId)<quantity)
                  {
                      qDebug() << "item id have not the quantity";
                      break;
                  }
                  //it's trap
                  if(CommonDatapack::commonDatapack.items.trap.find(itemId)!=CommonDatapack::commonDatapack.items.trap.cend()
         && fightEngine.isInFightWithWild())
                  {
                      remove_to_inventory(itemId);
                      useTrap(itemId);
                  }
                  else//else it's to use on current monster
                  {
                      const uint8_t
         &monsterPosition=fightEngine.getCurrentSelectedMonsterNumber();
                      if(fightEngine.useObjectOnMonsterByPosition(itemId,monsterPosition))
                      {
                          remove_to_inventory(itemId);
                          if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(itemId)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
                          {
                              client->useObjectOnMonsterByPosition(itemId,monsterPosition);
                              updateAttackList();
                              displayAttackProgression=0;
                              attack_quantity_changed=0;
                              if(battleType!=BattleType_OtherPlayer)
                                  doNextAction();
                              else
                              {
                                  ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                                  ui->labelFightEnter->setText(tr("In waiting of
         the other player action")); ui->pushButtonFightEnterNext->hide();
                              }
                          }
                          else
                              error(tr("You have selected a buggy
         object").toStdString());
                      }
                      else
                          QMessageBox::warning(this,tr("Warning"),tr("Can't be
         used now!"));
                  }
      */
    } break;
    default:
      qDebug() << "waitedObjectType is unknow";
      return;
  }
  waitedObjectType = ObjectType_All;
}

void OverMapLogic::objectUsed(const CatchChallenger::ObjectUsage &objectUsage) {
  if (objectInUsing.empty()) {
    emit error("No object usage to validate");
    return;
  }
  switch (objectUsage) {
    case CatchChallenger::ObjectUsage_correctlyUsed: {
      const uint16_t item = objectInUsing.front();
      // is crafting recipe
      if (CatchChallenger::CommonDatapack::commonDatapack
              .get_itemToCraftingRecipes()
              .find(item) != CatchChallenger::CommonDatapack::commonDatapack
                                 .get_itemToCraftingRecipes()
                                 .cend()) {
        connexionManager->client->addRecipe(
            CatchChallenger::CommonDatapack::commonDatapack
                .get_itemToCraftingRecipes()
                .at(item));
        // load_crafting_inventory();
        abort();
      } else if (CatchChallenger::CommonDatapack::commonDatapack.get_items()
                     .trap.find(item) !=
                 CatchChallenger::CommonDatapack::commonDatapack.get_items()
                     .trap.cend()) {
      } else if (CatchChallenger::CommonDatapack::commonDatapack.get_items()
                     .repel.find(item) !=
                 CatchChallenger::CommonDatapack::commonDatapack.get_items()
                     .repel.cend()) {
      } else
        qDebug() << "OverMapLogic::objectUsed(): unknow object type";
    } break;
    case CatchChallenger::ObjectUsage_failedWithConsumption:
      break;
    case CatchChallenger::ObjectUsage_failedWithoutConsumption:
      add_to_inventory(objectInUsing.front());
      break;
    default:
      break;
  }
  objectInUsing.erase(objectInUsing.cbegin());
}

void OverMapLogic::addCash(const uint32_t &cash) {
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  playerInformations.cash += cash;
}

void OverMapLogic::removeCash(const uint32_t &cash) {
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  playerInformations.cash -= cash;
}

void OverMapLogic::OnEnter() {
  OverMap::OnEnter();
  player_portrait_->RegisterEvents();
  bag->RegisterEvents();
  crafting_btn_->RegisterEvents();
}

void OverMapLogic::OnExit() {
  player_portrait_->UnRegisterEvents();
  bag->UnRegisterEvents();
  crafting_btn_->UnRegisterEvents();
  OverMap::OnExit();
}

void OverMapLogic::CreateInventoryTabs() {
  inventory_tabs_ = UI::LinkedDialog::Create();

  auto item = Inventory::Create();
  item->SetOnUseItem(std::bind(&OverMapLogic::OnUseItem, this, _1, _2, _3));
  inventory_tabs_->AddItem(item, "inventory");
  inventory_tabs_->AddItem(MonsterBag::Create(), "monsters");
  auto plant = Plant::Create();
  plant->SetOnUseItem(std::bind(&OverMapLogic::OnUseItem, this, _1, _2, _3));
  inventory_tabs_->AddItem(plant, "plants");
  inventory_tabs_->SetOnBack(
      std::bind(&OverMapLogic::OnInventoryNav, this, _1));
  inventory_tabs_->SetOnNext(
      std::bind(&OverMapLogic::OnInventoryNav, this, _1));
}

void OverMapLogic::CreatePlayerTabs() {
  player_tabs_ = UI::LinkedDialog::Create();

  player_tabs_->AddItem(Player::Create(), "player");
  player_tabs_->AddItem(Reputations::Create(), "reputations");
  player_tabs_->AddItem(Quests::Create(), "quests");
  player_tabs_->AddItem(FinishedQuests::Create(), "finished_quests");
  player_tabs_->AddItem(Clan::Create(), "clan");
}

void OverMapLogic::OnUseItem(Inventory::ObjectType type,
                             const uint16_t &item_id,
                             const uint32_t &quantity) {
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  auto info = PlayerInfo::GetInstance();
  switch (type) {
    case Inventory::kSeed: {
      if (QtDatapackClientLoader::datapackLoader->get_itemToPlants().find(
              item_id) ==
          QtDatapackClientLoader::datapackLoader->get_itemToPlants().cend()) {
        Logger::Log(QString("Item is not a plant"));
        // QMessageBox::critical(this,tr("Error"),tr("Internal
        // error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
        seed_in_waiting.pop_back();
        return;
      }
      const uint8_t &plant_id =
          QtDatapackClientLoader::datapackLoader->get_itemToPlants().at(
              item_id);
      if (!connexionManager->client->haveReputationRequirements(
              CatchChallenger::CommonDatapack::commonDatapack.get_plants()
                  .at(plant_id)
                  .requirements.reputation)) {
        Logger::Log(
            QString("You don't have the requirements to plant the seed"));
        // QMessageBox::critical(this,tr(" Error
        //"),tr(" You don't have the requirements to plant the seed
        //"));
        seed_in_waiting.pop_back();
        return;
      }
      if (havePlant(
              &ccmap->mapController.getMap(ccmap->mapController.current_map)
                   ->logicalMap,
              ccmap->mapController.getX(), ccmap->mapController.getY()) >= 0) {
        Logger::Log("Too slow to select a seed, have plant now");
        showTip(
            tr("Sorry, but now the dirt is not free to plant").toStdString());
        seed_in_waiting.pop_back();
        return;
      }
      if (playerInformations.items.find(item_id) ==
          playerInformations.items.cend()) {
        Logger::Log(QString("item id is not into the inventory"));
        seed_in_waiting.pop_back();
        break;
      }
      info->RemoveInventory(item_id, 1);
      info->Notify();

      const SeedInWaiting seedInWaiting = seed_in_waiting.back();
      seed_in_waiting.back().seedItemId = item_id;
      insert_plant(
          ccmap->mapController.getMap(seedInWaiting.map)->logicalMap.id,
          seedInWaiting.x, seedInWaiting.y, plant_id,
          static_cast<uint16_t>(
              CatchChallenger::CommonDatapack::commonDatapack.get_plants()
                  .at(plant_id)
                  .fruits_seconds));
      addQuery(QueryType_Seed);
      Logger::Log(QStringLiteral("send seed for: %1").arg(plant_id));
      emit useSeed(plant_id);
      connexionManager->client->seed_planted(true);
      connexionManager->client->insert_plant(
          ccmap->mapController.getMap(seedInWaiting.map)->logicalMap.id,
          seedInWaiting.x, seedInWaiting.y, plant_id,
          static_cast<uint16_t>(
              CatchChallenger::CommonDatapack::commonDatapack.get_plants()
                  .at(plant_id)
                  .fruits_seconds));
    } break;
    case Inventory::kRecipe:
      connexionManager->client->useObject(item_id);
      connexionManager->client->addRecipe(
          CatchChallenger::CommonDatapack::commonDatapack
              .get_itemToCraftingRecipes()
              .at(item_id));
      break;
      // case Inventory::ObjectType_ItemEvolutionOnMonster:
      // case Inventory::ObjectType_ItemLearnOnMonster:
      // case Inventory::ObjectType_ItemOnMonster:
      // case Inventory::ObjectType_ItemOnMonsterOutOfFight: {
      // const uint8_t monsterPosition = static_cast<uint8_t>(item_id);
      // const uint16_t item = item_id;
      // objectInUsing.erase(objectInUsing.cbegin());
      // if (!ok) {
      // ui->stackedWidget->setCurrentWidget(ui->page_inventory);
      // ui->inventoryUse->setText(tr("Select"));
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.item.find(
      // item) !=
      // CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.item[item]
      //.consumeAtUse)
      // add_to_inventory(item, 1, false);
      // break;
      //}
      // const CatchChallenger::PlayerMonster *const monster =
      // connexionManager->client->monsterByPosition(monsterPosition);
      // if (monster == NULL) {
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.item.find(
      // item) !=
      // CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.item[item]
      //.consumeAtUse)
      // add_to_inventory(item, 1, false);
      // break;
      //}
      // const CatchChallenger::Monster &monsterInformations =
      // CatchChallenger::CommonDatapack::commonDatapack.monsters.at(
      // monster->monster);
      // const QtDatapackClientLoader::MonsterExtra &monsterInformationsExtra =
      // QtDatapackClientLoader::datapackLoader->monsterExtra.at(
      // monster->monster);
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem
      //.find(item) != CatchChallenger::CommonDatapack::commonDatapack
      //.items.evolutionItem.cend()) {
      // uint8_t monsterEvolutionPostion = 0;
      // const CatchChallenger::Monster &monsterInformationsEvolution =
      // CatchChallenger::CommonDatapack::commonDatapack.monsters.at(
      // CatchChallenger::CommonDatapack::commonDatapack.items
      //.evolutionItem.at(item)
      //.at(monster->monster));
      // const QtDatapackClientLoader::MonsterExtra
      //&monsterInformationsEvolutionExtra =
      // QtDatapackClientLoader::datapackLoader->monsterExtra.at(
      // CatchChallenger::CommonDatapack::commonDatapack.items
      //.evolutionItem.at(item)
      //.at(monster->monster));
      // abort();
      //// create animation widget
      // if (animationWidget != NULL) delete animationWidget;
      // if (qQuickViewContainer != NULL) delete qQuickViewContainer;
      // animationWidget = new QQuickView();
      // qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
      // qQuickViewContainer->setMinimumSize(QSize(800, 600));
      // qQuickViewContainer->setMaximumSize(QSize(800, 600));
      // qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
      // ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
      //// show the animation
      // ui->stackedWidget->setCurrentWidget(ui->page_animation);
      // previousAnimationWidget = ui->page_map;
      // if (baseMonsterEvolution != NULL) delete baseMonsterEvolution;
      // if (targetMonsterEvolution != NULL) delete targetMonsterEvolution;
      // baseMonsterEvolution = new QmlMonsterGeneralInformations(
      // monsterInformations, monsterInformationsExtra);
      // targetMonsterEvolution = new QmlMonsterGeneralInformations(
      // monsterInformationsEvolution, monsterInformationsEvolutionExtra);
      // if (evolutionControl != NULL) delete evolutionControl;
      // evolutionControl = new EvolutionControl(
      // monsterInformations, monsterInformationsExtra,
      // monsterInformationsEvolution, monsterInformationsEvolutionExtra);
      // animationWidget->rootContext()->setContextProperty("animationControl",
      //&animationControl);
      // animationWidget->rootContext()->setContextProperty("evolutionControl",
      // evolutionControl);
      // animationWidget->rootContext()->setContextProperty("canBeCanceled",
      // QVariant(false));
      // animationWidget->rootContext()->setContextProperty(
      //"itemEvolution",
      // QUrl::fromLocalFile(QString::fromStdString(
      // QtDatapackClientLoader::datapackLoader->itemsExtra.at(item)
      //.imagePath)));
      // animationWidget->rootContext()->setContextProperty(
      //"baseMonsterEvolution", baseMonsterEvolution);
      // animationWidget->rootContext()->setContextProperty(
      //"targetMonsterEvolution", targetMonsterEvolution);
      // const std::string datapackQmlFile =
      // client->datapackPathBase() + "qml/evolution-animation.qml";
      // if (QFile(QString::fromStdString(datapackQmlFile)).exists())
      // animationWidget->setSource(
      // QUrl::fromLocalFile(QString::fromStdString(datapackQmlFile)));
      // else
      // animationWidget->setSource(
      // QStringLiteral("qrc:/qml/evolution-animation.qml"));
      // client->useObjectOnMonsterByPosition(item, monsterPosition);
      // if (!fightEngine.useObjectOnMonsterByPosition(item, monsterPosition)) {
      // std::cerr << "fightEngine.useObjectOnMonsterByPosition() Bug at "
      //<< __FILE__ << ":" << __LINE__ << std::endl;
      //#ifdef CATCHCHALLENGER_EXTRA_CHECK abort();
      //#endif
      //}
      // load_monsters();
      // return;
      //} else {
      // abort();
      // ui->stackedWidget->setCurrentWidget(ui->page_inventory);
      // ui->inventoryUse->setText(tr("Select"));
      // if (fightEngine.useObjectOnMonsterByPosition(item, monsterPosition)) {
      // showTip(
      // tr("Using <b>%1</b> on <b>%2</b>")
      //.arg(QString::fromStdString(
      // QtDatapackClientLoader::datapackLoader->itemsExtra
      //.at(item)
      //.name))
      //.arg(QString::fromStdString(monsterInformationsExtra.name))
      //.toStdString());
      // client->useObjectOnMonsterByPosition(item, monsterPosition);
      // load_monsters();
      // checkEvolution();
      //} else {
      // showTip(
      // tr("Failed to use <b>%1</b> on <b>%2</b>")
      //.arg(QString::fromStdString(
      // QtDatapackClientLoader::datapackLoader->itemsExtra
      //.at(item)
      //.name))
      //.arg(QString::fromStdString(monsterInformationsExtra.name))
      //.toStdString());
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.item.find(
      // item) !=
      // CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
      // if (CatchChallenger::CommonDatapack::commonDatapack.items.item[item]
      //.consumeAtUse)
      // add_to_inventory(item, 1, false);
      //}
      //}
      //}
  }
  RemoveChild(inventory_tabs_);
}

void OverMapLogic::ShowPlantDialog() {
  auto item = static_cast<Plant *>(inventory_tabs_->Item("plants"));
  item->SetVar(true);
  inventory_tabs_->SetCurrentItem("plants");
  AddChild(inventory_tabs_);
}

void OverMapLogic::CaptureCityYourAreNotLeaderSlot() {
  showTip(
      tr("You are not a clan leader to start a city capture").toStdString());
}

void OverMapLogic::CaptureCityYourLeaderHaveStartInOtherCitySlot(
    const std::string &zone) {
  if (QtDatapackClientLoader::datapackLoader->get_zonesExtra().find(zone) !=
      QtDatapackClientLoader::datapackLoader->get_zonesExtra().cend()) {
    showTip(
        tr("Your clan leader have start a capture for another city")
            .toStdString() +
        ": <b>" +
        QtDatapackClientLoader::datapackLoader->get_zonesExtra().at(zone).name +
        "</b>");
  } else {
    showTip(tr("Your clan leader have start a capture for another city")
                .toStdString());
  }
}

void OverMapLogic::CaptureCityPreviousNotFinishedSlot() {
  showTip(tr("Previous capture of this city is not finished").toStdString());
}

void OverMapLogic::CaptureCityStartBotFightSlot(const uint16_t &player_count,
                                            const uint16_t &clan_count,
                                            const uint16_t &fightId) {
  botFight(fightId);
}

void OverMapLogic::CaptureCityWinSlot() {
  if (!zonecatchName.empty()) {
    showTip(tr("Your clan win the city").toStdString() + ": <b>" +
            zonecatchName + "</b>");
  } else {
    showTip(tr("Your clan win the city").toStdString());
  }
}
