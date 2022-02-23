// Copyright 2021 CatchChallenger
#include "PlayerInfo.hpp"

#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

using CatchChallenger::CommonDatapack;

PlayerInfo *PlayerInfo::instance_ = nullptr;

PlayerInfo::PlayerInfo() {
  HaveClanInformation = false;
  connection_manager_ = ConnectionManager::GetInstance();
}

PlayerInfo::~PlayerInfo() {}

PlayerInfo *PlayerInfo::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new PlayerInfo();
  }
  return instance_;
}

CatchChallenger::Player_private_and_public_informations &
PlayerInfo::GetInformation() {
  return connection_manager_->client->get_player_informations();
}

const CatchChallenger::Player_private_and_public_informations &
PlayerInfo::GetInformationRO() const {
  return connection_manager_->client->get_player_informations_ro();
}

void PlayerInfo::Notify() { emit OnUpdateInfo(this); }

void PlayerInfo::AddCash(int32_t value) {
  auto &info = GetInformation();
  info.cash += value;
}

void PlayerInfo::AddInventory(uint16_t item_id, uint32_t quantity) {
  std::unordered_map<uint16_t, uint32_t> items;
  items[item_id] = quantity;

  AddInventory(items);
}

void PlayerInfo::AddInventory(
    const std::unordered_map<uint16_t, uint32_t> &items) {
  auto &info = GetInformation();

  for (const auto &n : items) {
    const uint16_t &item = n.first;
    const uint32_t &quantity = n.second;
    if (info.encyclopedia_item != NULL)
      info.encyclopedia_item[item / 8] |= (1 << (7 - item % 8));
    else
      std::cerr << "encyclopedia_item is null, unable to set" << std::endl;

    if (info.items.find(item) != info.items.cend())
      info.items[item] += quantity;
    else
      info.items[item] = quantity;
  }
}

void PlayerInfo::RemoveInventory(uint16_t item_id, uint32_t quantity) {
  std::vector<std::pair<uint16_t, uint32_t> > items;
  items.push_back(std::pair<uint16_t, uint32_t>(item_id, quantity));
  auto &info = GetInformation();

  for (const auto &n : items) {
    const uint16_t &item = n.first;
    const uint32_t &quantity = n.second;

    if (info.items.find(item) != info.items.cend()) {
      if (info.items.at(item) <= quantity)
        info.items.erase(item);
      else
        info.items[item] -= quantity;
    }
  }
}

void PlayerInfo::UpdateMonsters(
    std::vector<CatchChallenger::PlayerMonster> monsters) {
  auto &info = GetInformation();
  info.playerMonster = monsters;
}

bool PlayerInfo::IsAllowed(CatchChallenger::ActionAllow action) {
  auto const &info = GetInformationRO();
  return info.allow.find(action) != info.allow.cend();
}

void PlayerInfo::AppendReputationPoints(
    const std::vector<CatchChallenger::ReputationRewards> &rewards) {
  unsigned int index = 0;
  while (index < rewards.size()) {
    const CatchChallenger::ReputationRewards &reputationRewards =
        rewards.at(index);
    AppendReputationPoints(
        CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
            .at(reputationRewards.reputationId)
            .name,
        reputationRewards.point);
    index++;
  }
}

void PlayerInfo::AppendReputationPoints(const std::string &type,
                                        const uint32_t &point) {
  if (point == 0) return;
  if (QtDatapackClientLoader::datapackLoader->get_reputationNameToId().find(
          type) ==
      QtDatapackClientLoader::datapackLoader->get_reputationNameToId().cend()) {
    emit OnTipShowed("Unknow reputation: " + type, true);
    return;
  }
  const uint8_t &reputationId =
      QtDatapackClientLoader::datapackLoader->get_reputationNameToId().at(type);
  CatchChallenger::PlayerReputation playerReputation;

  auto &info = GetInformation();
  if (info.reputation.find(reputationId) != info.reputation.cend()) {
    playerReputation = info.reputation.at(reputationId);
  } else {
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
  if (info.reputation.find(reputationId) != info.reputation.cend()) {
    info.reputation[reputationId] = playerReputation;
  } else {
    info.reputation[reputationId] = playerReputation;
  }
  const std::string &reputationCodeName =
      CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
          .at(reputationId)
          .name;

  std::string tip;
  if (old_level < playerReputation.level) {
    if (QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
            reputationCodeName) !=
        QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend()) {
      tip =
          tr("You have better reputation into %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_reputationExtra()
                      .at(reputationCodeName)
                      .name))
              .toStdString();
    } else {
      tip = tr("You have better reputation into %1").arg("???").toStdString();
    }
  } else if (old_level > playerReputation.level) {
    if (QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
            reputationCodeName) !=
        QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend()) {
      tip =
          tr("You have worse reputation into %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_reputationExtra()
                      .at(reputationCodeName)
                      .name))
              .toStdString();
    } else {
      tip = tr("You have worse reputation into %1").arg("???").toStdString();
    }
  }

  emit OnTipShowed(tip, true);
}

bool PlayerInfo::HaveReputationRequirements(
    const std::vector<CatchChallenger::ReputationRequirements> &reputations) const
    {
  unsigned int index = 0;
  auto const &info = GetInformationRO();
  while (index < reputations.size()) {
    const CatchChallenger::ReputationRequirements &reputation =
        reputations.at(index);
    if (info.reputation.find(reputation.reputationId) !=
        info.reputation.cend()) {
      const CatchChallenger::PlayerReputation &playerReputation =
          info.reputation.at(reputation.reputationId);
      if (!reputation.positif) {
        if (-reputation.level < playerReputation.level) {
          //emit OnTipShowed(
              //QStringLiteral("reputation.level(%1)<playerReputation.level(%2)")
                  //.arg(reputation.level)
                  //.arg(playerReputation.level)
                  //.toStdString(),
              //true);
          return false;
        }
      } else {
        if (reputation.level > playerReputation.level ||
            playerReputation.point < 0) {
          //emit OnTipShowed(
              //QStringLiteral("reputation.level(%1)>playerReputation.level(%2) "
                             //"|| playerReputation.point(%3)<0")
                  //.arg(reputation.level)
                  //.arg(playerReputation.level)
                  //.arg(playerReputation.point)
                  //.toStdString(),
              //true);
          return false;
        }
      }
    } else if (!reputation.positif)  // default level is 0, but required level
                                     // is negative
    {
      //emit OnTipShowed(
          //QStringLiteral("reputation.level(%1)<0 and no reputation.type=%2")
              //.arg(reputation.level)
              //.arg(QString::fromStdString(
                  //CommonDatapack::commonDatapack.get_reputation()
                      //.at(reputation.reputationId)
                      //.name))
              //.toStdString(),
          //true);
      return false;
    }
    index++;
  }
  return true;
}

void PlayerInfo::AppendReputationRewards(
    const std::vector<CatchChallenger::ReputationRewards> &reputations) {
  unsigned int index = 0;
  while (index < reputations.size()) {
    const CatchChallenger::ReputationRewards &reputationRewards = reputations.at(index);
    AppendReputationPoints(CommonDatapack::commonDatapack.get_reputation()
                              .at(reputationRewards.reputationId)
                              .name,
                          reputationRewards.point);
    index++;
  }
  //show_reputation();
}
