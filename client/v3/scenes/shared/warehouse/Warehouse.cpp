// Copyright 2021 CatchChallenger
#include "Warehouse.hpp"

#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"
#include "WarehouseItem.hpp"

using Scenes::Warehouse;
using Scenes::WarehouseItem;
using std::placeholders::_1;
using std::placeholders::_2;

Warehouse::Warehouse() {
  SetDialogSize(1200, 600);
  on_transaction_finish_ = nullptr;
  selected_item_ = nullptr;

  player_name_ = UI::Label::Create(this);
  storage_name_ = UI::Label::Create(this);
  player_name_->SetAlignment(Qt::AlignCenter);
  storage_name_->SetAlignment(Qt::AlignCenter);

  current_mode_ = 0;

  current_mode_ = 0;

  mode_ = UI::Combo::Create(this);
  mode_->AddItem(tr("Item"));
  mode_->AddItem(tr("Monster"));
  mode_->AddItem(tr("Cash"));
  mode_->SetOnSelectChange(std::bind(&Warehouse::OnModeChange, this, _1));

  inventory_content_ = UI::GridView::Create(this);
  storage_content_ = UI::GridView::Create(this);

  player_cash_ = UI::Label::Create(this);
  storage_cash_ = UI::Label::Create(this);
  player_cash_->SetAlignment(Qt::AlignCenter);
  storage_cash_->SetAlignment(Qt::AlignCenter);

  move_left_ = UI::Button::Create(":/CC/images/interface/back.png", this);
  move_left_->SetOnClick(std::bind(&Warehouse::OnWithdraw, this));
  move_right_ = UI::Button::Create(":/CC/images/interface/next.png", this);
  move_right_->SetOnClick(std::bind(&Warehouse::OnDeposit, this));

  accept_ = UI::Button::Create(":/CC/images/interface/validate.png", this);
  accept_->SetOnClick(std::bind(&Warehouse::OnAcceptClick, this));

  SetTitle(tr("Warehouse"));

  connexionManager = ConnectionManager::GetInstance();
  AddActionButton(accept_);
}

Warehouse::~Warehouse() {
  delete inventory_content_;
  inventory_content_ = nullptr;

  delete storage_content_;
  storage_content_ = nullptr;

  delete move_left_;
  move_left_ = nullptr;

  delete move_right_;
  move_right_ = nullptr;

  delete accept_;
  accept_ = nullptr;

  delete player_name_;
  player_name_ = nullptr;

  delete storage_name_;
  storage_name_ = nullptr;

  delete mode_;
  mode_ = nullptr;

  delete accept_;
  accept_ = nullptr;

  delete player_cash_;
  player_cash_ = nullptr;

  delete storage_cash_;
  storage_cash_ = nullptr;
}

Warehouse *Warehouse::Create() { return new (std::nothrow) Warehouse(); }

void Warehouse::OnEnter() {
  UI::Dialog::OnEnter();
  inventory_content_->RegisterEvents();
  storage_content_->RegisterEvents();
  mode_->RegisterEvents();
  move_left_->RegisterEvents();
  move_right_->RegisterEvents();

  current_mode_ = 0;
  mode_->SetCurrentIndex(current_mode_);
  temp_cash_ = 0;
  temp_monster_deposit_.clear();
  temp_monster_withdraw_.clear();
  temp_items_.clear();

  UpdateContent();
  OnScreenResize();
}

void Warehouse::OnExit() {
  inventory_content_->UnRegisterEvents();
  storage_content_->UnRegisterEvents();
  mode_->UnRegisterEvents();
  move_left_->UnRegisterEvents();
  move_right_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void Warehouse::OnAcceptClick() {
  auto &info = PlayerInfo::GetInstance()->GetInformation();
  auto player_info = PlayerInfo::GetInstance();

  // Save all change in warehouse
  {
    std::vector<std::pair<uint16_t, uint32_t> > deposit_items;
    std::vector<std::pair<uint16_t, uint32_t> > withdraw_items;
    for (const auto &n : temp_items_) {
      if (n.second < 0)
        deposit_items.push_back(std::make_pair(n.first, -n.second));
      else if (n.second > 0)
        withdraw_items.push_back(std::make_pair(n.first, n.second));
    }
    int deposit_cash = temp_cash_ < 0 ? -temp_cash_ : 0;
    int withdraw_cash = temp_cash_ > 0 ? temp_cash_ : 0;
    connexionManager->client->wareHouseStore(
        withdraw_cash, deposit_cash, withdraw_items, deposit_items,
        temp_monster_withdraw_, temp_monster_deposit_);
  }

  // validate the change here
  if (temp_cash_ != 0) {
    player_info->AddCash(temp_cash_);
    info.warehouse_cash -= temp_cash_;
  }

  {
    for (const auto &n : temp_items_) {
      if (n.second > 0) {
        if (info.items.find(n.first) != info.items.cend()) {
          info.items[n.first] += n.second;
        } else {
          info.items[n.first] = n.second;
        }
        info.warehouse_items[n.first] -= n.second;
        if (info.warehouse_items.at(n.first) == 0)
          info.warehouse_items.erase(n.first);
      }
      if (n.second < 0) {
        info.items[n.first] += n.second;
        if (info.items.at(n.first) == 0) info.items.erase(n.first);
        if (info.warehouse_items.find(n.first) != info.warehouse_items.cend())
          info.warehouse_items[n.first] -= n.second;
        else
          info.warehouse_items[n.first] = -n.second;
      }
    }
  }
  {
    std::vector<CatchChallenger::PlayerMonster> playerMonsterToWithdraw;
    unsigned int index = 0;
    while (index < temp_monster_withdraw_.size()) {
      unsigned int sub_index = 0;
      while (sub_index < info.warehouse_playerMonster.size()) {
        if (sub_index == temp_monster_withdraw_.at(index)) {
          playerMonsterToWithdraw.push_back(
              info.warehouse_playerMonster.at(sub_index));
          info.warehouse_playerMonster.erase(
              info.warehouse_playerMonster.cbegin() + sub_index);
          break;
        }
        sub_index++;
      }
      index++;
    }
    if (temp_monster_withdraw_.size() > 0) {
      connexionManager->client->addPlayerMonster(playerMonsterToWithdraw);
    }
  }
  {
    unsigned int index = 0;
    while (index < temp_monster_deposit_.size()) {
      const std::vector<CatchChallenger::PlayerMonster> &playerMonster =
          connexionManager->client->getPlayerMonster();
      const uint8_t &monsterPosition =
          static_cast<uint8_t>(temp_monster_deposit_.at(index));
      info.warehouse_playerMonster.push_back(playerMonster.at(monsterPosition));
      connexionManager->client->removeMonsterByPosition(monsterPosition);
      index++;
    }
  }
  player_info->Notify();
  Close();
  if (on_transaction_finish_) {
    on_transaction_finish_();
  }
  // showTip(tr("You have correctly withdraw/deposit your
  // goods").toStdString());
}

void Warehouse::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentBoundary();
  qreal content_width = (content.width() / 2) - 40;
  qreal block_1 = content.x();
  qreal block_2 = content.x() + content.width() - content_width;

  player_name_->SetPos(block_1, content.y());
  player_name_->SetWidth(content_width);
  storage_name_->SetPos(block_2, player_name_->Y());
  storage_name_->SetWidth(content_width);

  mode_->SetSize(150, 40);
  mode_->SetPos(content.x() + (content.width() / 2) - (mode_->Width() / 2),
                player_name_->Y());

  inventory_content_->SetPos(block_1, player_name_->Bottom() + 15);
  inventory_content_->SetSize(content_width, content.height() - 25);
  inventory_content_->SetItemSpacing(5);

  player_cash_->SetPos(block_1, player_name_->Bottom() + 15);
  player_cash_->SetWidth(content_width);

  storage_content_->SetPos(block_2, inventory_content_->Y());
  storage_content_->SetSize(content_width, inventory_content_->Height());
  storage_content_->SetItemSpacing(5);

  storage_cash_->SetPos(block_2, player_cash_->Y());
  storage_cash_->SetWidth(content_width);

  move_left_->SetSize(50, 54);
  move_left_->SetPos(
      content.x() + (content.width() / 2) - (move_left_->Width() / 2),
      inventory_content_->Y());

  move_right_->SetSize(50, 54);
  move_right_->SetPos(move_left_->X(),
                      move_left_->Y() + 15 + move_left_->Height());
}

void Warehouse::OnItemClick(Node *node) {
  selected_item_ = node;
  auto items = inventory_content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    static_cast<WarehouseItem *>(item)->SetSelected(false);
  }
}

void Warehouse::SetOnTransactionFinish(std::function<void()> callback) {
  on_transaction_finish_ = callback;
}

void Warehouse::OnModeChange(uint8_t index) {
  current_mode_ = index;
  UpdateContent();
}

void Warehouse::UpdateContent() {
  const CatchChallenger::Player_private_and_public_informations &info =
      connexionManager->client->get_player_informations_ro();

  player_name_->SetText(info.public_informations.pseudo);
  storage_name_->SetText(QString("Storage"));

  if (current_mode_ == 0) {
    inventory_content_->SetVisible(true);
    storage_content_->SetVisible(true);
    player_cash_->SetVisible(false);
    storage_cash_->SetVisible(false);

    // inventory
    {
      inventory_content_->Clear();
      auto i = info.items.begin();
      while (i != info.items.cend()) {
        int32_t quantity = i->second;
        if (temp_items_.find(i->first) != temp_items_.cend()) {
          quantity += temp_items_.at(i->first);
        }
        if (quantity > 0) {
          inventory_content_->AddItem(ItemToItem(i->first, quantity));
        }
        ++i;
      }
      for (const auto &j : temp_items_) {
        if (info.items.find(j.first) == info.items.cend() && j.second > 0)
          inventory_content_->AddItem(ItemToItem(j.first, j.second));
      }
    }

    // inventory warehouse
    {
      storage_content_->Clear();
      auto i = info.warehouse_items.begin();
      while (i != info.warehouse_items.cend()) {
        int32_t quantity = i->second;
        if (temp_items_.find(i->first) != temp_items_.cend()) {
          quantity -= temp_items_.at(i->first);
        }
        if (quantity > 0) {
          storage_content_->AddItem(ItemToItem(i->first, quantity));
        }
        ++i;
      }
      for (const auto &j : temp_items_) {
        if (info.warehouse_items.find(j.first) == info.warehouse_items.cend() &&
            j.second < 0)
          storage_content_->AddItem(ItemToItem(j.first, -j.second));
      }
    }
  } else if (current_mode_ == 1) {
    inventory_content_->SetVisible(true);
    storage_content_->SetVisible(true);
    player_cash_->SetVisible(false);
    storage_cash_->SetVisible(false);
    // monster
    // do before because the dispatch put into random of it
    inventory_content_->Clear();
    storage_content_->Clear();

    // monster
    {
      const std::vector<CatchChallenger::PlayerMonster> &playerMonster =
          connexionManager->client->getPlayerMonster();
      uint8_t index = 0;
      while (index < playerMonster.size()) {
        const CatchChallenger::PlayerMonster &monster = playerMonster.at(index);
        if (CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(
                monster.monster) !=
            CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend()) {
          auto item = ItemToMonster(index, monster.monster, monster.level);
          if (!vectorcontainsAtLeastOne(temp_monster_deposit_, index) ||
              vectorcontainsAtLeastOne(temp_monster_withdraw_, index)) {
            inventory_content_->AddItem(item);
          } else {
            storage_content_->AddItem(item);
          }
        }
        index++;
      }
    }

    // monster warehouse
    {
      uint8_t index = 0;
      while (index < info.warehouse_playerMonster.size()) {
        const CatchChallenger::PlayerMonster &monster =
            info.warehouse_playerMonster.at(index);
        if (CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(
                monster.monster) !=
            CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend()) {
          auto item = ItemToMonster(index, monster.monster, monster.level);
          if (!vectorcontainsAtLeastOne(temp_monster_withdraw_, index) ||
              vectorcontainsAtLeastOne(temp_monster_deposit_, index))
            storage_content_->AddItem(item);
          else
            inventory_content_->AddItem(item);
        }
        index++;
      }
    }
  } else if (current_mode_ == 2) {
    inventory_content_->SetVisible(false);
    storage_content_->SetVisible(false);
    player_cash_->SetVisible(true);
    storage_cash_->SetVisible(true);

    player_cash_->SetText(QString::number(info.cash + temp_cash_));
    storage_cash_->SetText(QString::number(info.warehouse_cash - temp_cash_));
  }
}

WarehouseItem *Warehouse::ItemToItem(const uint16_t &item_id,
                                     const uint32_t &quantity) {
  auto item = WarehouseItem::Create();
  item->SetData(99, item_id);
  item->SetData(98, quantity);
  item->SetOnClick(std::bind(&Warehouse::OnItemClick, this, _1));
  if (QtDatapackClientLoader::GetInstance()->get_itemsExtra().find(item_id) !=
      QtDatapackClientLoader::GetInstance()->get_itemsExtra().cend()) {
    item->SetPixmap(
        QtDatapackClientLoader::GetInstance()->getItemExtra(item_id).image);
    if (quantity > 1) item->SetText(QString::number(quantity));
    item->setToolTip(QString::fromStdString(
        QtDatapackClientLoader::GetInstance()->get_itemsExtra().at(item_id).name));
  } else {
    item->SetPixmap(
        QtDatapackClientLoader::GetInstance()->defaultInventoryImage());
    if (quantity > 1)
      item->SetText(QStringLiteral("id: %1 (x%2)").arg(item_id).arg(quantity));
    else
      item->SetText(QStringLiteral("id: %1").arg(item_id));
  }
  return item;
}

WarehouseItem *Warehouse::ItemToMonster(const uint32_t &monster_id,
                                        const uint16_t &monster,
                                        const uint32_t &quantity) {
  auto item = WarehouseItem::Create();
  item->SetData(99, monster_id);
  item->SetOnClick(std::bind(&Warehouse::OnItemClick, this, _1));
  item->SetPixmap(
      QtDatapackClientLoader::GetInstance()->getMonsterExtra(monster).front);
  item->setToolTip(QString::fromStdString(
      QtDatapackClientLoader::GetInstance()->get_monsterExtra().at(monster).name));
  return item;
}

void Warehouse::OnDeposit() {
  QString value;
  if (current_mode_ == 0) {
    if (selected_item_ == nullptr) {
      return;
    }
    uint32_t quantity = selected_item_->Data(98);

    Globals::GetInputDialog()->ShowInputNumber(
        tr("How many items do you want to deposit?"),
        std::bind(&Warehouse::OnDepositDelayed, this, _1), 1, quantity,
        QString::number(1));
    return;
  } else if (current_mode_ == 1) {
    if (selected_item_ == nullptr) {
      return;
    }
  } else if (current_mode_ == 2) {
    const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
    if ((info.cash + temp_cash_) == 1) {
      value = QString::number(1);
    } else {
      Globals::GetInputDialog()->ShowInputNumber(
          tr("How many cash do you want to deposit?"),
          std::bind(&Warehouse::OnDepositDelayed, this, _1), 1,
          info.cash + temp_cash_, QString::number(1));
    }
  }
  OnDepositDelayed(value);
}

void Warehouse::OnDepositDelayed(const QString &value) {
  if (current_mode_ == 0) {
    uint32_t id = selected_item_->Data(99);

    if (temp_items_.find(id) == temp_items_.cend()) {
      temp_items_[id] = 0;
    }
    temp_items_[id] -= value.toInt();
  } else if (current_mode_ == 1) {
    if (selected_item_ == nullptr) {
      return;
    }
    uint8_t id = selected_item_->Data(99);

    bool remain_valid_monster = false;
    uint8_t index = 0;
    std::vector<CatchChallenger::PlayerMonster> warehouseMonsterOnPlayerList =
        MonstersWithdrawed();
    while (index < warehouseMonsterOnPlayerList.size()) {
      const CatchChallenger::PlayerMonster &monster =
          warehouseMonsterOnPlayerList.at(index);
      if (index != id && monster.egg_step == 0 && monster.hp > 0) {
        remain_valid_monster = true;
        break;
      }
      index++;
    }
    if (!remain_valid_monster) {
      Globals::GetAlertDialog()->Show(
          tr("Error"), tr("You can't deposite your last alive monster!"));
      return;
    }
    vectorremoveOne(temp_monster_withdraw_, id);
    temp_monster_deposit_.push_back(id);
  } else if (current_mode_ == 2) {
    const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
    bool ok = true;
    int i = value.toInt();
    if (temp_cash_ + info.cash < i) {
      Globals::GetAlertDialog()->Show(
          tr("Error"),
          tr("You can't deposit more than you have in your cash!"));
      return;
    }
    if (!ok || i <= 0) return;
    temp_cash_ -= i;
  }
  UpdateContent();
  selected_item_ = nullptr;
}

void Warehouse::OnWithdraw() {
  QString value;

  if (current_mode_ == 0) {
    if (selected_item_ == nullptr) {
      return;
    }
    uint32_t quantity = selected_item_->Data(98);
    Globals::GetInputDialog()->ShowInputNumber(
        tr("How many items do you want to withdraw?"),
        std::bind(&Warehouse::OnWithdrawDelayed, this, _1), 1, quantity,
        QString::number(1));
    return;
  } else if (current_mode_ == 1) {
    if (selected_item_ == nullptr) {
      return;
    }
  } else if (current_mode_ == 2) {
    const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
    bool ok = true;
    int i;
    if ((info.warehouse_cash - temp_cash_) == 1) {
      value = QString::number(1);
    } else {
      Globals::GetInputDialog()->ShowInputNumber(
          tr("How much cash do you want to withdraw?"),
          std::bind(&Warehouse::OnWithdrawDelayed, this, _1), 1,
          info.warehouse_cash - temp_cash_, QString::number(1));
      return;
    }
  }
  OnWithdrawDelayed(value);
}

void Warehouse::OnWithdrawDelayed(const QString &value) {
  if (current_mode_ == 0) {
    uint32_t id = selected_item_->Data(99);
    if (temp_items_.find(id) == temp_items_.cend()) {
      temp_items_[id] = 0;
    }
    temp_items_[id] += value.toInt();
  } else if (current_mode_ == 1) {
    uint8_t id = selected_item_->Data(99);
    std::vector<CatchChallenger::PlayerMonster> list = MonstersWithdrawed();
    if (list.size() >
        CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters) {
      Globals::GetAlertDialog()->Show(tr("Error"),
                                      tr("You can't wear more monster!"));
      return;
    }
    vectorremoveOne(temp_monster_deposit_, id);
    temp_monster_withdraw_.push_back(id);
    UpdateContent();
    selected_item_ = nullptr;
  } else if (current_mode_ == 2) {
    const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
    bool ok = true;
    int i = value.toInt();
    std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] " << i
              << std::endl;
    if (temp_cash_ + info.warehouse_cash < i) {
      Globals::GetAlertDialog()->Show(
          tr("Error"),
          tr("You can't withdraw more than you have in your cash!"));
      return;
    }
    if (!ok || i <= 0) return;
    temp_cash_ += i;
  }
  UpdateContent();
  selected_item_ = nullptr;
}

std::vector<CatchChallenger::PlayerMonster> Warehouse::MonstersWithdrawed()
    const {
  const CatchChallenger::Player_private_and_public_informations &info =
      PlayerInfo::GetInstance()->GetInformationRO();
  std::vector<CatchChallenger::PlayerMonster> warehouseMonsterOnPlayerList;
  {
    const std::vector<CatchChallenger::PlayerMonster> &playerMonster =
        connexionManager->client->getPlayerMonster();
    uint8_t index = 0;
    while (index < playerMonster.size()) {
      const CatchChallenger::PlayerMonster &monster = playerMonster.at(index);
      if (CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(
              monster.monster) !=
              CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend() &&
          !vectorcontainsAtLeastOne(temp_monster_deposit_, index))
        warehouseMonsterOnPlayerList.push_back(monster);
      index++;
    }
  }
  {
    uint8_t index = 0;
    while (index < info.warehouse_playerMonster.size()) {
      const CatchChallenger::PlayerMonster &monster =
          info.warehouse_playerMonster.at(index);
      if (CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(
              monster.monster) !=
              CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend() &&
          vectorcontainsAtLeastOne(temp_monster_withdraw_, index))
        warehouseMonsterOnPlayerList.push_back(monster);
      index++;
    }
  }
  return warehouseMonsterOnPlayerList;
}
