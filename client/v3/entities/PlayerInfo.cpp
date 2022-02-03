// Copyright 2021 CatchChallenger
#include "PlayerInfo.hpp"

#include <iostream>

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
PlayerInfo::GetInformationRO() {
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
